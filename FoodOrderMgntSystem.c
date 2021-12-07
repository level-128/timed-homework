#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>


#define type(x__) ((csv_element * )x__)->var_type
#define __get_node_item(column, x) ((csv_element *)list_get_node(column, x)->value)->value
#define $(list__, index__) list_get_node(list__, index__)->value


const static int DEFAULT_DICT_SIZE = 500;

const static double REHASH_LIMIT = 0.7;


void ERROR(char * message) {
	printf("\nERROR -- %s\n", message);
	printf("\tPress enter to exit. ");
	while (1) {
		if (getchar() == '\n') {
			break;
		}
	}
	exit(1);
}

static uint64_t PRIME = 51539607599;
// this prime is the first prime starting at 1 << 35 + 1 << 34.
// it is computed by using following Python code:
/*

from sympy import sieve
x = sieve.primerange(51539607552)
print(next(x))

 */
// if you want to run this code, it is strongly recommended to use pypy, instead of cpython, as Python interpreter.
// this prime will be used in the dict_hash function as the modular.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// dictionary object
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * memory module of the dictionary:
 *
 * 0        1        2        3         4                                    8 bytes
 * ---------------------------------------------------------------------------
 * |             size                   |         content_count              | <- 0x00
 * ---------------------------------------------------------------------------
 * |                             rand_salt                                   | <- 0x08
 * ---------------------------------------------------------------------------
 *
 *
 *
 *
 *
 */






// the individual trunk in the dictionary.
typedef struct {
	 bool is_used; // true if the chunk is empty, false otherwise.
	 bool is_at_default_location; // true   if the chunk stores itself at the index which it should belong.
	                              // false  if the index which it should belong has been occupied by another chunk.
	 uint32_t hash_value; // the hash value of the key
	 char * ptr_key;
	 void * ptr_value;
} dict_chunk;


typedef struct {
	 uint32_t size; // the size of the dictionary
	 uint32_t content_count; // how many chunk are there in the dictionary
	 uint64_t rand_salt; // the salt of the hash function, generated per dict object.
	 dict_chunk * dict_value;
} dict;

uint32_t dict_hash(const char * key, uint32_t seed);


void dict_store(dict *, char *, void *);



uint32_t dict_hash(const char * key, uint32_t seed) {
	uint64_t result = 0;
	for (uint32_t i = 0;; i++) {
		if (key[i] == 0) {
			break;
		}
		result += (uint64_t) key[i] << 56;
		result %= PRIME;
	}
	return (result % 0xFFFFFFFF) ^ seed;
}


dict * new_dict(size_t size) {
	if (!size) {
		size = DEFAULT_DICT_SIZE;
	}

	dict * self = malloc(sizeof(dict));
	self->dict_value = calloc(size, sizeof(dict_chunk));
	self->size = size;
	self->rand_salt = random();
	return self;
}

void dict_remap(dict * self, size_t new_size) {
	dict_chunk * old_dict = self->dict_value;
	dict_chunk * tmp;
	size_t size = self->size;

	self->dict_value = calloc(sizeof(dict_chunk), new_size);
	self->content_count = 0;
	self->size = new_size;

	for (size_t i = 0; i < size; i++) {
		tmp = old_dict + i;
		if (tmp->is_used) {
			dict_store(self, tmp->ptr_key, tmp->ptr_value);
		}
	}
	free(old_dict);
}

void dict_store(dict * self, char * key, void * value) {
	if (self->content_count > REHASH_LIMIT * self->size) {
		dict_remap(self, self->size + self->size / 2);
	}
	uint32_t hash_value = dict_hash(key, self->rand_salt);
	uint32_t location = hash_value % self->size;
	dict_chunk * self_trunk = self->dict_value + location;
	if (self_trunk->is_used && self_trunk->hash_value != hash_value) {
		location += 1;
		for (location = location % self->size;; location++) {
			self_trunk = self->dict_value + location;
			if (self_trunk->is_at_default_location) {
				continue;
			}
			if (!self_trunk->is_used) {
				break;
			}
			if (self_trunk->hash_value == hash_value) {
				break;
			}
		}
	}
	self_trunk->is_used = true;
	self_trunk->is_at_default_location = (location == hash_value % self->size);
	self_trunk->hash_value = hash_value;
	self_trunk->ptr_key = malloc(sizeof(strlen(key)));
	strcpy(self_trunk->ptr_key, key);
	self_trunk->ptr_value = value;

	self->content_count += 1;
}

dict_chunk * p__dict_find_trunk(dict * self, char * key) {

	uint32_t hash_value = dict_hash(key, self->rand_salt);
	uint32_t location = hash_value % self->size;
	dict_chunk * self_trunk = self->dict_value + location;
	if (!self_trunk->is_used) {
		ERROR("The requested trunk does not exist.");
	}
	if (self_trunk->hash_value == hash_value) {
		return self_trunk;
	}

	location += 1;
	for (location = location % self->size;; location++) {
		self_trunk = self->dict_value + location;
		if (self_trunk->is_at_default_location) {
			continue;
		}
		if (self_trunk->is_used) {
			if (self_trunk->hash_value == hash_value) {
				return self_trunk;
			}
		}
		else {
			ERROR("The requested trunk does not exist.");
		}
	}
}

void * dict_find(dict * self, char * key) {
	return p__dict_find_trunk(self, key)->ptr_value;
}

void dict_del(dict * self, char * key) {
	// delete the record of the key-value pair in dictionary. Does not free the value.
	if ((self->content_count < REHASH_LIMIT * self->size / 2) && self->size > DEFAULT_DICT_SIZE) {
		dict_remap(self, (size_t) (self->size / 1.5));
	}

	dict_chunk * self_trunk = p__dict_find_trunk(self, key);
	memset(self_trunk, 0, sizeof(dict_chunk));
	self->content_count -= 1;
}

void dict_free(dict * self) {
	// free the dictionary.
	dict_chunk * self_trunk;
	for (uint32_t location = 0; location < self->size; location++) {
		self_trunk = self->dict_value + location;
		free(self_trunk->ptr_key);
	}
	free(self);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// link list
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// node in the list.
typedef struct node {
	 void * value; // stored value
	 struct node * next_node;
} node;

// list
typedef struct {
	 size_t len; // the length of the linked list
	 node * last_node; // the addr of the last node
	 node * first_node; // the addr of the first node
	 size_t last_visit_node_index; // record the last visited index from method 'list_get_node'
	 node * last_visit_node; // record the last visited node from method 'list_get_node'
} list;

list * new_list(void);
// create a new list

list * new_list_init(uint32_t param_len, char * params[]);
// create a new list with elements 'params[]' pre initialized in the list.

node * list_get_node(list * self, size_t index);
// return the node in the list at index 'index'.

void list_append(list * self, void * newval);
// append an element 'newval' into the list.

void list_append_int(list * self, int64_t newval);
// append an integer 'newval' into the list.

void list_pop_by_node(list * self, node * pop_node);
// remove the node 'pop_node' from list.

void list_pop_by_index(list * self, size_t index);
// remove the node at index 'index' from list.

void list_free(list * self);
// free the list. only frees every node and struct list;

char * list_to_str(list * self);
// return a str, pretending every element inside the list is a char, by linking each element.



list * new_list(void) {

	list * self = malloc(sizeof(list));
	node * my_element = malloc(sizeof(node));
	self->first_node = my_element;
	self->last_node = my_element;
	self->len = 0;

	self->last_visit_node_index = 0;
	self->last_visit_node = my_element;

	my_element->next_node = self->first_node;
	return self;
}

list * new_list_init(uint32_t param_len, char * params[]) {
	list * self = new_list();
	for (uint32_t i = 0; i < param_len; i++) {
		list_append(self, params[i]);
	}
	return self;
}

node * list_get_node(list * self, size_t index) {
	if (index == 0) {
		return self->first_node;
	}
	if (index >= self->len || index < 0) {
		ERROR("List out of index.");
	}

	size_t start_index;
	if (index >= self->last_visit_node_index) {
		start_index = self->last_visit_node_index;
	}
	else {
		start_index = 0;
		self->last_visit_node = self->first_node;
	}

	for (size_t i = start_index; i < index; i++) {
		self->last_visit_node = self->last_visit_node->next_node;
	}
	self->last_visit_node_index = index;
	return self->last_visit_node;
}

void list_append(list * self, void * newval) {
	node * last_node = self->last_node;
	node * new_element = malloc(sizeof(node));

	new_element->next_node = self->first_node;

	last_node->value = newval;
	last_node->next_node = new_element;

	self->last_node = new_element;
	self->len++;
}

void list_append_int(list * self, int64_t newval) {
	union void_s_to_int {
		 void * void_s;
		 int64_t int_;
	} tmp;
	tmp.int_ = newval;
	list_append(self, tmp.void_s);
}

void list_pop_by_node(list * self, node * pop_node) {
	node * next_node = pop_node->next_node;
	void * tmp_val = pop_node->value;

	pop_node->value = next_node->value;
	next_node->value = tmp_val;

	pop_node->next_node = next_node->next_node;

	if (next_node == self->last_node) {
		self->last_node = pop_node;
	}
	self->len -= 1;
	self->last_visit_node = self->first_node;
	self->last_visit_node_index = 0;
	free(next_node);
}

void list_pop_by_index(list * self, size_t index) {
	list_pop_by_node(self, list_get_node(self, index));
}

void list_free(list * self) {
	node * current_element = self->first_node;
	node * prev_element;
	for (int i = 0; i < self->len; i++) {
		prev_element = current_element;
		current_element = current_element->next_node;
		free(prev_element);
	}
	free(self);
}

char * list_to_str(list * self) {
	char * my_str = malloc(sizeof(char) * (self->len + 1));
	node * current_element = self->first_node;

	union void_s_to_char { // I know this conversion is SAFE and PLEASE DO NOT WARN ME.
		 void * void_s; // The reason which I DISLIKE C is it provided useless warning -- the
		 char char_;    // thing which C should do is provide me a block of memory and shut up; I know what i'm doing.
	} tmp;

	for (size_t i = 0; i < self->len; i++) {
		tmp.void_s = current_element->value;
		my_str[i] = tmp.char_;
		current_element = current_element->next_node;
	}
	my_str[self->len] = '\0';
	return my_str;
}

__attribute__((unused)) void list_print_str(list * self) {
	// this function is used for debug only.
	printf("--list len: %zu, first node: %p, last node %p\n", self->len, self->first_node, self->last_node);
	printf("--last visit node index: %zu last visit node: %p\n", self->last_visit_node_index,
	       self->last_visit_node);
	node * this_node = self->first_node;
	for (size_t i = 0; i < self->len; i++) {
		printf("node: %zu, ", i);
		printf("addr: %p, ", this_node);
		printf("next addr: %p, ", this_node->next_node);
		printf("value: \"%s\"\n", (char *) (this_node->value + 3));
		this_node = this_node->next_node;
	}
	printf("\n");
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSV reading:

#define CSV_INT_ 1
#define CSV_FLOAT_ 2
#define CSV_STRING_ 3

typedef struct {
	 char var_type;
	 union {
		  int64_t int_;
		  double float_;
		  char * string_;
	 } value;
} csv_element;

typedef struct {
	 char * sheet_name;
	 uint32_t element_count;
	 list * sheet_titles;
	 uint32_t sheet_titles_count;
	 list * sheet_element;

	 uint32_t sheet_index_row;
	 dict * sheet_index_dict;

	 struct csv_sheet * father_sheet;
	 struct csv_sheet * successor_sheet;
} csv_sheet;

typedef struct {
	 list * table_list;
	 dict * table_dict;
} csv_table;

typedef struct {
	 char * sheet_name;
	 uint32_t sheet_index_row;
	 list * sheet_titles;
} sheet_properties;


void csv_print_element(csv_element * element_) {
	if (element_->var_type == CSV_FLOAT_) {
		printf("%lf", element_->value.float_);
	}
	else if (element_->var_type == CSV_INT_) {
		printf("%li", element_->value.int_);
	}
	else {
		printf("%s", element_->value.string_);
	}
}

bool csv_compare_element(csv_element * x, csv_element * y) {
	if (x->var_type != y->var_type) {
		return false;
	}
	if (x->var_type != CSV_STRING_) {
		return (x->value.string_ == y->value.string_); // comparing the memory as if they are char *.
	}
	return !strcmp(x->value.string_, y->value.string_);
}

void csv_free_element(csv_element * self) {
	if (self->var_type == CSV_STRING_) {
		free(self->value.string_);
	}
	free(self);
}

void csv_free_elements(list * column) {
	for (uint32_t i = 0; i < column->len; i++) {
		csv_free_element($(column, i));
	}
}

csv_sheet * csv_sheet_create(char * sheet_name, list * sheet_titles, uint32_t sheet_index_row) {
	csv_sheet * self = malloc(sizeof(csv_sheet));
	self->element_count = 0;
	self->sheet_name = malloc(sizeof(char) * (strlen(sheet_name) + 1));
	strcpy(self->sheet_name, sheet_name);
	self->sheet_titles = sheet_titles;
	self->sheet_index_row = sheet_index_row;

	self->sheet_element = new_list();
	self->sheet_index_dict = new_dict(0);
	return self;
}

csv_sheet * csv_sheet_create_from_properties(sheet_properties * my_properties) {
	csv_sheet * ret = csv_sheet_create(my_properties->sheet_name,
	                                   my_properties->sheet_titles, my_properties->sheet_index_row);
	return ret;
}

void csv_sheet_append(csv_sheet * self, list * column) {
	csv_element * index_element = (list_get_node(column, self->sheet_index_row)->value);
	if (index_element->var_type != CSV_STRING_) {
		ERROR("Index element must be string.");
	}
	list_append(self->sheet_element, column);

	dict_store(self->sheet_index_dict, index_element->value.string_, column);
	self->element_count += 1;
}

void csv_sheet_append_by_name(csv_table * self, char * csv_sheet_name, list * column) {
	csv_sheet_append(dict_find(self->table_dict, csv_sheet_name), column);
}

void csv_sheet_pop(csv_sheet * self, uint32_t index) {
	list * my_column = $(self->sheet_element, index);
	char * hash_element = $(my_column, self->sheet_index_row);
	dict_del(self->sheet_index_dict, hash_element);
	csv_free_elements(my_column);
	list_pop_by_index(self->sheet_element, index);
	self->element_count -= 1;

}

list * csv_sheet_get_column_by_name(csv_sheet * self, char * index_name) {
	return dict_find(self->sheet_index_dict, index_name);
}

list * csv_sheet_get_column_by_index(csv_sheet * self, uint32_t index) {
	return list_get_node(self->sheet_element, index)->value;
}

csv_element * p__csv_visit_raw(list * self, uint32_t column, uint32_t row) {
	return list_get_node(list_get_node(self, column)->value, row)->value;
}

csv_table * new_csv_table(void) {
	csv_table * self = malloc(sizeof(csv_table));
	self->table_dict = new_dict(0);
	self->table_list = new_list();
	return self;
}

void csv_table_append_sheet(csv_table * self, csv_sheet * _element) {
	dict_store(self->table_dict, _element->sheet_name, _element);
	list_append(self->table_list, _element);
}

csv_sheet * csv_table_get_sheet_by_name(csv_table * self, char * sheet_name) {
	return dict_find(self->table_dict, sheet_name);
}


list * p__csv_parse_split_line(char * line, char ** buf_con) {
	// input: "hi, 33.3"
	// return: [['h', 'i'], ['3', '3', '.', '3']]

	list * result_list = new_list();
	list * buf = new_list();

	bool is_quote = false;
	bool is_start_element = false;

	size_t i = 0;
	for (;; i++) {

		if (line[i] == '\n' && !is_quote) {
			i += 1;
			break;
		}
		if (line[i] == '\0') {
			break;
		}
		if (line[i] == ' ' && !is_start_element) {
			continue;
		}
		if (line[i] < 0 || line[i] == '\r') {
			continue;
		}
		if (line[i] == '"') {
			is_quote = !is_quote;
			is_start_element = true;
		}
		if (line[i] == ',' && (!is_quote)) {
			list_append(result_list, buf);
			buf = new_list();
			is_start_element = false;
		}
		else {
			is_start_element = true;
			list_append_int(buf, line[i]);
		}
	}
	if (buf->len > 0) {
		list_append(result_list, buf);
	}
	else {
		free(buf);
	}
	* buf_con = line + i;
	return result_list;
}

csv_element * p__csv_parse_element(list * element) {
	// input: ['3', '3', '.', '3']
	// return csv_element{var_type = CSV_FLOAT_, value = 33.3}

	char * element_str = list_to_str(element);
	char * endptr = NULL;

	csv_element * current_element = malloc(sizeof(csv_element));

	current_element->value.int_ = strtoll(element_str, & endptr, 10);
	if (endptr[0] == '\0') {
		current_element->var_type = CSV_INT_;
	}
	else {
		current_element->value.float_ = strtod(element_str, & endptr);
		if (endptr[0] == '\0') {
			current_element->var_type = CSV_FLOAT_;
		}
		else {
			// it is a str
			node * last_node;
			// removing str at the first and the end

			if (element_str[0] == '"' &&
			    element_str[element->len - 1] == '"') {
				list_pop_by_index(element, 0);
				list_pop_by_index(element, element->len - 1);
			}
			// parse "" as "
			node * current_node;
			bool is_quote = false;
			int element_char;
			for (size_t i = 0; i < element->len; i++) {
				current_node = list_get_node(element, i);
				element_char = (int) (uint64_t) current_node->value;
				if (element_char == '"') {
					if (is_quote) {
						list_pop_by_node(element, current_node);
						element->last_visit_node = last_node;
						element->last_visit_node_index = i - 1;
					}
					else {
						last_node = current_node;
					}
					is_quote = !is_quote;
				}
				else {
					is_quote = false;
				}
			}
			current_element->value.string_ = list_to_str(element);
			current_element->var_type = CSV_STRING_;
		}
	}
	free(element);
	return current_element;
}

list * p__csv_parse_line(char * line, char ** buf_con) {
	list * list_line = p__csv_parse_split_line(line, buf_con);
	list * list_result = new_list();
	list * list_element;

	for (int i = 0; i < list_line->len; i++) {
		list_element = list_get_node(list_line, i)->value;
		list_append(list_result, p__csv_parse_element(list_element));
	}
	return list_result;
}

char * p__csv_read_file(char * file_name) {
	struct stat stat_;
	int file_handle = open(file_name, O_RDWR);
	if (file_handle < 0) {
		ERROR("csv csv_table load failed, program terminated.");
	}
	if (stat(file_name, & stat_) != 0) {
		ERROR("csv csv_table load failed: can't determine file's size. program terminated.");
	}
	char * file_ptr = mmap(NULL, stat_.st_size,
	                       PROT_READ | PROT_WRITE, MAP_SHARED,
	                       file_handle, 0);
	return file_ptr;
}

list * p__csv_read_raw_table(char * file_name) {
	char * csv_file = p__csv_read_file(file_name);
	char * buf_con = csv_file;
	list * csv_list = new_list();

	while (true) {
		list_append(csv_list, p__csv_parse_line(buf_con, & buf_con));
		if (buf_con[0] == '\0') {
			break;
		}
	}
	return csv_list;
}

sheet_properties * p__csv_try_read_properties(list * column) {
	sheet_properties * tmp = NULL;
	csv_element * first_item = list_get_node(column, 0)->value;
	if (first_item->var_type == CSV_STRING_) {
		if (first_item->value.string_[0] == '\t') {
			tmp = malloc(sizeof(sheet_properties));
			tmp->sheet_name = malloc(sizeof(char) * strlen(first_item->value.string_ + 1));

			strcpy(tmp->sheet_name, first_item->value.string_ + 1);
			tmp->sheet_index_row = __get_node_item(column, column->len - 1).int_;

			csv_free_element(first_item);
			csv_free_element(list_get_node(column, column->len - 1)->value);

			list_pop_by_index(column, 0);
			list_pop_by_index(column, column->len - 1);

			tmp->sheet_titles = column;
		}
	}
	return tmp;
}

void csv_open(char * file_name, csv_table * table) {
	list * csv_raw_table = p__csv_read_raw_table(file_name);
	list * csv_raw_column = list_get_node(csv_raw_table, 0)->value;
	char * target_sheet_name;
	csv_element * tmp;

	sheet_properties * properties = p__csv_try_read_properties(csv_raw_column);
	csv_sheet * my_sheet = csv_sheet_create_from_properties(properties);
	csv_table_append_sheet(table, my_sheet);

	for (uint32_t i = 1; i < csv_raw_table->len; i++) {
		csv_raw_column = list_get_node(csv_raw_table, i)->value;
		if (!csv_raw_column->len) {
			continue; // empty column
		}
		properties = p__csv_try_read_properties(csv_raw_column);
		if (properties) {
			my_sheet = csv_sheet_create_from_properties(properties);
			csv_table_append_sheet(table, my_sheet);
			continue;
		}
		target_sheet_name = __get_node_item(csv_raw_column, 0).string_;
		tmp = list_get_node(csv_raw_column, 0)->value;
		list_pop_by_index(csv_raw_column, 0);
		csv_sheet_append_by_name(table, target_sheet_name, csv_raw_column);
		csv_free_element(tmp);
	}
}

void csv_print_column(list * column) {
	for (uint32_t i = 0; i < column->len - 1; i++) {
		csv_print_element(list_get_node(column, i)->value);
		printf("\t");
	}
	csv_print_element(list_get_node(column, column->len - 1)->value);
	printf("\n");
}

void csv_print_sheet(csv_table * self, char * sheet_name, char * index_prefix, char * index_name) {
	csv_sheet * self_sheet = csv_table_get_sheet_by_name(self, sheet_name);
	printf("%s\t", index_name);
	csv_print_column(self_sheet->sheet_titles);
	for (uint32_t column_number = 0; column_number < self_sheet->element_count; column_number++) {
		printf(index_prefix, column_number + 1);
		printf("\t");
		csv_print_column(csv_sheet_get_column_by_index(self_sheet, column_number));
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utility functions:


char * get_input(void) {
	list * input_list = new_list();
	char * return_val;
	int c;
	while ((c = getchar()) != '\n') {
		list_append_int(input_list, c);
	}
	return_val = list_to_str(input_list);
	list_free(input_list);
	return return_val;
}

bool input_yn_question(char * message) {
	char * input;
	while (true) {
		printf("%s (y/n)?: ", message);
		input = get_input();
		if (!strcmp(input, "y") || !strcmp(input, "Y")) {
			return true;
		}
		if (!strcmp(input, "n") || !strcmp(input, "N")) {
			return false;
		}
		printf("Invalid input.\n");
	}
}

char * input_option_question(
		char * message, char * error_message, int option_number, char * options[], bool print_options) {
	char * input;
	while (true) {
		printf("%s", message);
		if (print_options) {
			printf(" (%s", options[0]);
			for (int i = 1; i < option_number; i++) {
				printf("/%s", options[i]);
			}
			printf(")?: ");
		}
		input = get_input();
		for (int i = 0; i < option_number; i++) {
			if (!strcmp(input, options[i])) {
				return options[i];
			}
		}
		printf("%s\n", error_message);
	}
}


int main(void) {
	csv_table * my_table = new_csv_table();
	csv_open("test.csv", my_table);
	csv_print_sheet(my_table, "MY_TABLE", "%u", "INDEX");

	list * my_column = csv_sheet_get_column_by_name(csv_table_get_sheet_by_name(my_table, "MY_TABLE"), "2LOf2");
	csv_print_element($(my_column, 1));
}



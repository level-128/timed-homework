//smyww3 20393538 Weizheng Wang
/*
	This is my homework for course COMP 1038 (Programming and Algorithms) in
	University of Nottingham Ningbo.

	I decide to help future students' for handling their assignments by free
	this software after the deadline of this assignment.

	Before 28/Dec/2021 (UTC time), this software remains proprietary, and
	source code will not be released due to the plagiarism policy of
	Nottingham Ningbo. After 28/Dec/2021, this software will be released
	under both GNU Affero General Public License version 3 and Mozilla Public
	License version 2.0 with Exception:

	Copyright Exception:
	Modifying this program without accepting GNU Affero General Public License
	version 3 and Mozilla Public License version 2.0 is allowed only for
	coursework evaluating purpose; Program, which combined with this work, by
   this purpose could remain proprietary or limit rights.

	The reason that I decided to use copyleft license is because providing and
	offering them opportunity to copy part of my code into their work is not what
   I want; If students want to copy it without violating my rights, they
   should include copyright notice to tell teachers that "I'm copying other's
   work. ", which should not be allowed in an individual assignment. Freeing
   this code is to provide students materials to learn from.


									COPYRIGHT INFORMATION:
	Copyright (C) 2021  Weizheng Wang
	Email: weizheng.wang@uconn.edu / smyww3@nottingham.edu.cn

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.



	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0. If a copy of the MPL was not distributed with this file, You can
	obtain one at https://mozilla.org/MPL/2.0/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <wchar.h>
#include <memory.h>
#include <assert.h>

#define $(list__, index__) list_get_node((list *)list__, index__)->value
#define val(list__, index__) (((csv_element *)(list_get_node((list *)list__, index__)->value))->value)

const static int DEFAULT_DICT_SIZE = 500;

const static double REHASH_LIMIT = 0.7;

const static uint64_t PRIME = 51539607599;

/* this prime is the first prime starting at 1 << 35 + 1 << 34.
it is computed by using following Python code:

from sympy import sieve
x = sieve.primerange(1 << 35 + 1 << 34)
print(next(x))

if you want to run this code, it is strongly recommended to use pypy, instead of cpython, as Python interpreter.
this prime will be used in the p__dict_hash function as the modular.

make sure your system has sympy installed. if not, run 'pip install sympy' in your system console.
*/

void ERROR(char *message) {
	printf("\nERROR -- %s\n", message);
	printf("\tPress enter to exit. ");
	while (1) {
		if (getchar() == '\n') {
			break;
		}
	}
	exit(1);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// link list
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// node in the list.
typedef struct node {
	 void *value; // stored value
	 struct node *next_node;
} node;

// list
typedef struct {
	 size_t len; // the length of the linked list
	 node *last_node; // the addr of the last node
	 node *first_node; // the addr of the first node
	 size_t last_visit_node_index; // record the last visited index from method 'list_get_node'
	 node *last_visit_node; // record the last visited node from method 'list_get_node'
} list;

list *new_list(void);
// create a new list

node *list_get_node(list *self, uint32_t index);
// return the node in the list at index 'index'.

void list_append(list *self, void *newval);
// append an element 'newval' into the list.

void list_pop_by_node(list *self, node *pop_node);
// remove the node 'pop_node' from list.

void list_pop_by_index(list *self, size_t index);
// remove the node at index 'index' from list.

void list_free(list *self);
// free the list. only frees every node and struct list;



list *new_list(void) {

	list *self = malloc(sizeof(list));
	node *my_element = malloc(sizeof(node));
	self->first_node = my_element;
	self->last_node = my_element;
	self->len = 0;

	self->last_visit_node_index = 0;
	self->last_visit_node = my_element;

	my_element->next_node = self->first_node;
	return self;
}


node *list_get_node(list *self, uint32_t index) {
	if (index == 0) {
		return self->first_node;
	}
	if (index >= self->len || index < 0) {
		ERROR("List out of index.");
	}

	size_t start_index;
	if (index >= self->last_visit_node_index) {
		start_index = self->last_visit_node_index;
	} else {
		start_index = 0;
		self->last_visit_node = self->first_node;
	}

	for (size_t i = start_index; i < index; i++) {
		self->last_visit_node = self->last_visit_node->next_node;
	}
	self->last_visit_node_index = index;
	return self->last_visit_node;
}

void list_append(list *self, void *newval) {
	node *last_node = self->last_node;
	node *new_element = malloc(sizeof(node));

	new_element->next_node = self->first_node;

	last_node->value = newval;
	last_node->next_node = new_element;

	self->last_node = new_element;
	self->len++;
}

void list_insert_by_index(list *self, void *newval, uint32_t index) {
	node *tmp_node = malloc(sizeof(node));
	tmp_node->value = newval;

	if (index == 0) {
		tmp_node->next_node = self->first_node;
		self->first_node = tmp_node;
	} else {
		node *list_prev_node = list_get_node(self, index - 1);
		node *list_aft_node = list_prev_node->next_node;
		list_prev_node->next_node = tmp_node;
		tmp_node->next_node = list_aft_node;
	}
	self->len += 1;
}

void list_pop_by_node(list *self, node *pop_node) {
	node *next_node = pop_node->next_node;
	void *tmp_val = pop_node->value;

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

void list_pop_by_index(list *self, size_t index) {
	list_pop_by_node(self, list_get_node(self, index));
}

void list_free(list *self) {
	node *current_element = self->first_node;
	node *prev_element;
	for (int i = 0; i < self->len; i++) {
		prev_element = current_element;
		current_element = current_element->next_node;
		free(prev_element);
	}
	free(self);
}

__attribute__((unused)) void list_print_str(list *self) {
	// this function is used for debug only.
	printf("--list len: %zu, first node: %p, last node %p\n", self->len, self->first_node, self->last_node);
	printf("--last visit node index: %zu last visit node: %p\n", self->last_visit_node_index,
	       self->last_visit_node);
	node *this_node = self->first_node;
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
// Unicode str;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
	 list str_object;
	 uint32_t ref_count;
} string;

union void_s_to_wchar {
	 void *void_s;
	 wchar_t wchar;
} str_conv;

typedef node character;

string *new_string(wchar_t str[]);

void str_append(string *self, int64_t newval);

string *new_string(wchar_t str[]) {
	string *self = malloc(sizeof(string));
	character *my_element = malloc(sizeof(node));
	self->ref_count = 1;

	self->str_object.first_node = my_element;
	self->str_object.last_node = my_element;
	self->str_object.len = 0;

	self->str_object.last_visit_node_index = 0;
	self->str_object.last_visit_node = my_element;

	my_element->next_node = self->str_object.first_node;

	if (str) {
		for (uint32_t i = 0; str[i] != '\0'; i++) {
			str_append(self, str[i]);
		}
	}
	return self;
}

void str_append(string *self, int64_t newval) {

	str_conv.wchar = newval;
	list_append(&self->str_object, str_conv.void_s);
}

int32_t str_search(string *self, uint32_t from, const wchar_t target[]) {
	int j = 0;
	void *cache_node; // set the cache node at the first match wchar.

	for (uint32_t i = from; i < self->str_object.len;) {
		str_conv.void_s = $(&self->str_object, i + j);
		if (!target[j]) {
			self->str_object.last_visit_node_index = i;
			self->str_object.last_visit_node = cache_node;
			return i;
			// implication-defined behaviour: narrowing conversion from unsigned int to int is implication-defined.
		}
		if (str_conv.wchar != target[j]) {
			i++;
			j = 0;
			continue;
		} else {
			if (j == 0) {
				cache_node = list_get_node(&self->str_object, i);
			}
			j++;
		}
	}
	return -1;
}

void str_insert_by_index(string *self, wchar_t *str, uint32_t index) {
	size_t str_len = wcslen(str);
	for (uint32_t i = 0; i < str_len; i++) {
		str_conv.wchar = str[i];
		list_insert_by_index(&self->str_object, str_conv.void_s, index + i);
	}
}

void str_replace(string *self, wchar_t *from, wchar_t *to, uint32_t max_replace_time) {
	if (!wcscmp(to, L"")) {
		ERROR("str_replace function tries to replace to an empty string.");
	}
	uint32_t index = 0;
	uint32_t char_to_len = wcslen(to);
	uint32_t char_from_len = wcslen(from);
	for (int _ = 0; _ < max_replace_time;) {
		index = str_search(self, index, from);
		if (index == -1) {
			break;
		}

		for (int i = 0; i < char_from_len; i++) {
			list_pop_by_node(&self->str_object, list_get_node(&self->str_object, index));
		}
		str_insert_by_index(self, to, index);
		index += char_to_len - char_from_len;
	}
}

void str_free(string *self) {
	if (self->ref_count == 1) {
		list_free((list *) self);
	} else {
		self->ref_count -= 1;
	}
}

bool str_cmpw(string *self, wchar_t *target) {
	for (uint32_t i = 0; i < self->str_object.len; i++) {
		str_conv.void_s = $(self, i);
		if (str_conv.wchar != target[i]) {
			return false;
		}
	}
	return (bool) (!target[self->str_object.len]);
}

bool str_cmp(string *self, string *target) {
	if (self->str_object.len != target->str_object.len) {
		return false;
	}
	for (uint32_t i = 0; i < self->str_object.len; i++) {
		if ($(self, i) != $(target, i)) {
			return false;
		}
	}
	return true;
}

void str_print(string *self) {
	for (uint32_t i = 0; i < self->str_object.len; i++) {
		str_conv.void_s = ($(&self->str_object, i));
		putwchar(str_conv.wchar);
	}

}

string *get_input(void) {
	string *input_list = new_string(NULL);
	int c;
	while ((c = getchar()) != '\n') {
		str_append(input_list, c);
	}
	return input_list;
}

string *str_cpy(string *self) {
	self->ref_count += 1;
	return self;
}

wchar_t *str_out(string *self) {
	wchar_t *my_str = malloc(sizeof(wchar_t) * (self->str_object.len + 1));
	node *current_element = self->str_object.first_node;

	union void_s_to_char { // I know this conversion is SAFE and PLEASE DO NOT WARN ME.
		 void *void_s; // The reason which I DISLIKE C is it provided useless warning -- the
		 char char_;    // thing which C should do is provide me a block of memory and shut up; I know what i'm doing.
	} tmp;

	for (size_t i = 0; i < self->str_object.len; i++) {
		tmp.void_s = current_element->value;
		my_str[i] = (unsigned char)tmp.char_;
		current_element = current_element->next_node;
	}
	my_str[self->str_object.len] = '\0';
	return my_str;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSV data structures:
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define CSV_INT_ 1
#define CSV_FLOAT_ 2
#define CSV_STRING_ 3
#define CSV_HEADER 4

// equivalent Haskell pseudo code:

/* data csv_element = csv_element {
 *    var_type :: char
 *    , value :: {int_ :: int64_t  |
 *                float_ :: double |
 *                string_ :: string}
 *    } deriving (Show, Eq)
 */

typedef struct {
	 char var_type;
	 union {
		  int64_t int_;
		  double float_;
		  string *string_;
	 } value;
} csv_element;

csv_element *csv_new_int(int64_t int_) {
	csv_element *self = malloc(sizeof(csv_element));
	self->value.int_ = int_;
	self->var_type = CSV_INT_;
	return self;
}

csv_element *csv_new_float(double float_) {
	csv_element *self = malloc(sizeof(csv_element));
	self->value.float_ = float_;
	self->var_type = CSV_FLOAT_;
	return self;
}

csv_element *csv_new_str_from_wchar(wchar_t *str_) {
	csv_element *self = malloc(sizeof(csv_element));
	string *string_ = new_string(str_);
	self->value.string_ = string_;
	self->var_type = CSV_STRING_;
	return self;
}

csv_element *csv_new_str_from_str(string *str_) {
	csv_element *self = malloc(sizeof(csv_element));
	self->value.string_ = str_cpy(str_);
	self->var_type = CSV_STRING_;
	return self;
}

csv_element *csv_new_header_from_str(string *str_){
	assert($(str_, 0) == (void *)'\t');
	list_pop_by_index(&str_->str_object, 0);
	csv_element *self = malloc(sizeof(csv_element));
	self->value.string_ = str_cpy(str_);
	self->var_type = CSV_HEADER;
	return self;
}

bool csv_cmp(csv_element *x, csv_element *y) {
	if (x->var_type != y->var_type) {
		return false;
	}
	if (x->value.int_ == y->value.int_) {
		return true; // comparing the memory as if they are int.
	}
	if (x->var_type == CSV_STRING_) {
		return str_cmp(x->value.string_, y->value.string_);
	} else {
		return false;
	}

}

void csv_element_print(csv_element *element_) {
	if (element_->var_type == CSV_FLOAT_) {
		printf("%lf", element_->value.float_);
	} else if (element_->var_type == CSV_INT_) {
		printf("%li", element_->value.int_);
	} else {
		str_print(element_->value.string_);
	}
}

void csv_element_free(csv_element *self) {
	if (self->var_type == CSV_STRING_) {
		str_free(self->value.string_);
	}
	free(self);
}


//void csv_element_delete_from_list(list *target_list, uint32_t index) {
//   node *tmp_ = list_get_node(target_list, index);
//   csv_element_free(tmp_->value);
//   list_pop_by_node(target_list, tmp_);
//}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//csv column
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
	 list column_object;
	 node *index_node;
} column;

column *new_column(void) {
	column *self = malloc(sizeof(column));

	character *my_element = malloc(sizeof(node));

	self->column_object.first_node = my_element;
	self->column_object.last_node = my_element;
	self->column_object.len = 0;

	self->column_object.last_visit_node_index = 0;
	self->column_object.last_visit_node = my_element;

	my_element->next_node = self->column_object.first_node;
	return self;
}

void col_free(column *self) {
	for (uint32_t i = 0; i < self->column_object.len; i++) {
		csv_element_free($(&self->column_object, i));
	}
	list_free((list *) self);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// dictionary object
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// the individual trunk in the dictionary.
typedef struct {
	 bool is_used; // true if the chunk is empty, false otherwise.
	 bool is_at_default_location;
	 // true   if the chunk stores itself at the index which it should belong.
	 // false  if the index which it should belong has been occupied by another chunk.
	 csv_element *key;
	 column *value;
} dict_chunk;


typedef struct {
	 uint32_t size; // the size of the dictionary
	 uint32_t content_count; // how many chunk are there in the dictionary
	 uint64_t rand_salt; // the salt of the hash function, generated per dict object.
	 dict_chunk *dict_value;
} dict;

dict *new_dict(size_t size);
// create a new dictionary.

uint32_t p__dict_hash(const csv_element *key, uint32_t seed);

void p__dict_remap(dict *self, size_t new_size);

dict_chunk *p__dict_find_trunk(dict *self, csv_element *key);

void dict_store(dict *self, csv_element *key, void *value);
// store the value

void *dict_find(dict *self, csv_element *key);
// find the value from key

void dict_pop(dict *self, csv_element *key);
// delete the record of the key-value pair in dictionary. Does not free the value.

void dict_free(dict *self);
// delete the dictionary


uint32_t p__dict_hash(const csv_element *key, uint32_t seed) {
	if (key->var_type == CSV_STRING_) {

		wchar_t *string_ = str_out(key->value.string_);
		uint64_t result = 0;
		for (uint32_t i = 0;; i++) {
			if (string_[i] == 0) {
				break;
			}
			result += (uint64_t) string_[i] << 56;
			result %= PRIME;
		}
		return (result % 0xFFFFFFFF) ^ seed;
	}
	int64_t key_ = key->value.int_;
	// hashing the address of the csv_element
	return (intptr_t) key_ * ((int64_t) seed + (0L << 31)) % 1 << 32;
	// this code uses undefined behaviour (integer overflow) because of performance consideration, and the fact that in
	// 64-bit systems long integer overflow will cause wraparound (because the register size are 64 bit max).


}

dict *new_dict(size_t size) {
	if (!size) {
		size = DEFAULT_DICT_SIZE;
	}

	dict *self = malloc(sizeof(dict));
	self->dict_value = calloc(size, sizeof(dict_chunk));
	self->size = size;
	self->rand_salt = random();
	return self;
}

void p__dict_remap(dict *self, size_t new_size) {
	dict_chunk *old_dict = self->dict_value;
	dict_chunk *tmp;
	size_t size = self->size;

	self->dict_value = calloc(sizeof(dict_chunk), new_size);
	self->content_count = 0;
	self->size = new_size;

	for (size_t i = 0; i < size; i++) {
		tmp = old_dict + i;
		if (tmp->is_used) {
			dict_store(self, tmp->key, tmp->value);
		}
	}
	free(old_dict);
}

void dict_store(dict *self, csv_element *key, void *value) {
	if (self->content_count > REHASH_LIMIT * self->size) {
		// rehash the dict
		p__dict_remap(self, self->size + self->size / 2);
	}

	uint32_t hash_value = p__dict_hash(key, self->rand_salt);
	uint32_t location = hash_value % self->size;
	dict_chunk *self_trunk = self->dict_value + location;
	if (self_trunk->is_used && self_trunk->key != key) {
		location += 1;
		for (location = location % self->size;; location++) {
			self_trunk = self->dict_value + location;
			if (self_trunk->is_at_default_location == true) {
				continue;
			}
			if (self_trunk->is_used == false) {
				break;
			}
			if (csv_cmp(key, self_trunk->key)) {
				break;
			}
		}
	}
	self_trunk->is_used = true;
	self_trunk->is_at_default_location = (location == hash_value % self->size);
	self_trunk->key = key;
	self_trunk->value = value;

	self->content_count += 1;
}

dict_chunk *p__dict_find_trunk(dict *self, csv_element *key) {

	uint32_t hash_value = p__dict_hash(key, self->rand_salt);
	uint32_t location = hash_value % self->size;
	dict_chunk *self_trunk = self->dict_value + location;
	if (!self_trunk->is_used) {
		return NULL;
	}
	if (csv_cmp(self_trunk->key, key)) {
		return self_trunk;
	}

	location += 1;
	for (location = location % self->size;; location++) {
		self_trunk = self->dict_value + location;
		if (self_trunk->is_at_default_location) {
			continue;
		}
		if (self_trunk->is_used) {
			if (csv_cmp(self_trunk->key, key)) {
				return self_trunk;
			}
		} else {
			return NULL;
		}
	}
}

void *dict_find(dict *self, csv_element *key) {
	return p__dict_find_trunk(self, key)->value;
}

void dict_pop(dict *self, csv_element *key) {

	if ((self->content_count < REHASH_LIMIT * self->size / 2) && self->size > DEFAULT_DICT_SIZE) {
		p__dict_remap(self, (size_t) (self->size / 1.5));
	}

	dict_chunk *self_trunk = p__dict_find_trunk(self, key);
	memset(self_trunk, 0, sizeof(dict_chunk));
	self->content_count -= 1;
}

void dict_free(dict *self) {
	// free the dictionary.
	free(self);
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


typedef struct {
	 string *sheet_name;
	 uint32_t element_count;
	 column *sheet_titles;
	 list *sheet_element; // List[columns]
	 // expend to
	 // sheet_element: List[columns[csv_elements[int | float | string[w_char] ] ] ]
	 uint32_t sheet_index_row;
	 dict *sheet_index_dict;
} csv_sheet;


typedef struct {
	 string *sheet_name;
	 uint32_t sheet_index_row;
	 column *sheet_titles;
} sheet_properties;


csv_sheet *csv_sheet_create(string *sheet_name, column *sheet_titles, uint32_t sheet_index_row) {
	csv_sheet *self = malloc(sizeof(csv_sheet));

	self->element_count = 0;
	self->sheet_name = str_cpy(sheet_name);
	self->sheet_titles = sheet_titles;
	self->sheet_index_row = sheet_index_row;

	self->sheet_element = new_list();
	self->sheet_index_dict = new_dict(0);
	return self;
}

csv_sheet *csv_sheet_create_from_header(column *my_properties) {
	string * sheet_name = val(my_properties, 0).string_;
	uint32_t sheet_index_row = val(my_properties, my_properties->column_object.len - 1).int_;
	list_pop_by_index(&my_properties->column_object, 0);
	list_pop_by_index(&my_properties->column_object, my_properties->column_object.len - 1);
	csv_sheet *ret = csv_sheet_create(sheet_name,my_properties, sheet_index_row);
	return ret;
}

void csv_sheet_append(csv_sheet *self, column *column) {
	list_append(self->sheet_element, column);
	if (self->sheet_index_row != -1) {
		dict_store(self->sheet_index_dict, $(column, self->sheet_index_row), column);
	}
	self->element_count += 1;
}

column *csv_sheet_get_column_by_name(csv_sheet *self, csv_element *index) {
	return dict_find(self->sheet_index_dict, index);
}

column *csv_sheet_get_column_by_index(csv_sheet *self, uint32_t index) {
	return $(self->sheet_element, index);
}

csv_element *csv_sheet_get_element_by_index(csv_sheet *self, uint32_t column, uint32_t row) {
	return $($(self->sheet_element, column), row);
}

void csv_sheet_pop_column_by_index(csv_sheet *self, uint32_t index) {
	column *my_column = csv_sheet_get_column_by_index(self, index);
	csv_element *hash_element = $(my_column, self->sheet_index_row);
	dict_pop(self->sheet_index_dict, hash_element);
	col_free(my_column);
	list_pop_by_index(self->sheet_element, index);
	self->element_count -= 1;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
	 list *table_list;
	 dict *table_dict;
} csv_table;


csv_table *new_csv_table(void) {
	csv_table *self = malloc(sizeof(csv_table));
	self->table_dict = new_dict(0);
	self->table_list = new_list();
	return self;
}

void csv_table_append_sheet(csv_table *self, csv_sheet *_element) {
	dict_store(self->table_dict, csv_new_str_from_str(_element->sheet_name), _element);
	list_append(self->table_list, _element);
}

csv_sheet *csv_table_get_sheet_by_key(csv_table *self, csv_element *key) {
	return dict_find(self->table_dict, key);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
//csv_element *p__csv_visit_raw(list *self, uint32_t column, uint32_t row) {
//   return list_get_node(list_get_node(self, column)->value, row)->value;
//}


void csv_sheet_append_by_key(csv_table *self, csv_element *key, column *column_) {
	csv_sheet_append(csv_table_get_sheet_by_key(self, key), column_);
}


list *p__csv_parse_split_line(char *line, char **buf_con) {
	// input: "hi, 33.3"
	// return: [['h', 'i'], ['3', '3', '.', '3']]

	list *result_list = new_list();
	string *buf = new_string(NULL);

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
			buf = new_string(NULL);
			is_start_element = false;
		} else {
			is_start_element = true;
			str_append(buf, line[i]);
		}
	}
	if (buf->str_object.len > 0) {
		list_append(result_list, buf);
	} else {
		str_free(buf);
	}
	*buf_con = line + i;
	return result_list;
}

csv_element *p__csv_parse_element(string *element) {
	// input: ['3', '3', '.', '3']
	// return csv_element{var_type = CSV_FLOAT_, value = 33.3}

	wchar_t *element_str = str_out(element);
	wchar_t *endptr = NULL;

	int64_t int_ = wcstoll(element_str, &endptr, 10);
	if (endptr[0] == '\0') {
		str_free(element);
		return csv_new_int(int_);
	}

	double float_ = wcstod(element_str, &endptr);
	if (endptr[0] == '\0') {
		str_free(element);
		return csv_new_float(float_);
	}

	bool is_header = element_str[0] == '\t';

	if (element_str[0 + is_header] == '"' && element_str[element->str_object.len - 1] == '"') {
		list_pop_by_index(&element->str_object, 0 + is_header);
		list_pop_by_index(&element->str_object, element->str_object.len - 1);
	}

	str_replace(element, L"\"\"", L"\"", UINT32_MAX);
	if (is_header){
		return csv_new_header_from_str(element);
	}
	else{
		return csv_new_str_from_str(element);
	}

}


column *p__csv_parse_line(char *line, char **buf_con) {
	// parse a line from *line to *buf_con, and return the parsed element in a list.
	// return type: List[csv_elements{}]
	list *list_line = p__csv_parse_split_line(line, buf_con);
	column *list_result = new_column();
	string *list_element;

	for (int i = 0; i < list_line->len; i++) {
		list_element = list_get_node(list_line, i)->value;
		list_append(&list_result->column_object, p__csv_parse_element(list_element));
	}
	return list_result;
}

char *p__csv_read_file(char *file_name) {
	struct stat stat_;
	int file_handle = open(file_name, O_RDWR | O_CREAT);
	if (file_handle < 0) {
		ERROR("csv csv_table load failed, program terminated.");
	}
	if (stat(file_name, &stat_) != 0) {
		ERROR("csv csv_table load failed: can't determine file's size. program terminated.");
	}
	char *file_ptr = mmap(NULL, stat_.st_size,
	                      PROT_READ, MAP_SHARED,
	                      file_handle, 0);
	close(file_handle); // close the file handle DOES NOT unmap the file.
	return file_ptr;
}

void csv_open(char *file_name, csv_table *table) {
	char *csv_file = p__csv_read_file(file_name);
	char *buf_con = csv_file;
	column * my_column;

	while (true) {
		my_column = p__csv_parse_line(buf_con, &buf_con);
		if (buf_con[0] == '\0') {
			break;
		}
		csv_element * first_element = $(my_column, 0);
		if (first_element->var_type == CSV_HEADER){
			csv_sheet_create_from_header(my_column);
		}
		else{
			list_pop_by_index(&my_column->column_object, 0);
			csv_sheet_append_by_key(table, first_element, my_column);
		}
		csv_element_free(first_element);
	}
	shm_unlink(csv_file);
}

void *csv_save_sheets(char *file_name, char *sheets) {

}

void csv_print_column(column *column) {
	for (uint32_t i = 0; i < column->column_object.len - 1; i++) {
		csv_element_print($(column, i));
		printf("\t");
	}
	csv_element_print($(column, column->column_object.len - 1));
	printf("\n");
}

void csv_print_sheet(csv_sheet *self_sheet, char *index_prefix, char *index_name) {
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

bool input_yn_question(char *message) {
	string *input;
	while (true) {
		printf("%s (y/n)?: ", message);
		input = get_input();
		if (!str_cmpw(input, L"y") || !str_cmpw(input, L"Y")) {
			free(input);
			return true;
		}
		if (!str_cmpw(input, L"n") || !str_cmpw(input, L"N")) {
			free(input);
			return false;
		}
		free(input);
		printf("Invalid input.\n");
	}
}

wchar_t *input_option_question(char *message, char *error_message, int option_number, wchar_t *options[], bool print_options) {
	string *input;
	while (true) {
		printf("%s", message);
		if (print_options) {
			printf(" (%ls", options[0]);
			for (int i = 1; i < option_number; i++) {
				printf("/%ls", options[i]);
			}
			printf(")?: ");
		}
		input = get_input();
		for (int i = 0; i < option_number; i++) {
			if (!str_cmpw(input, options[i])) {
				free(input);
				return options[i];
			}
		}
		free(input);
		printf("%s\n", error_message);
	}
}

int64_t input_integer_question(char *message, char *error_message, int64_t min_value, int64_t max_value) {
	string *input;
	wchar_t *begin_ptr = NULL;
	wchar_t *end_ptr = NULL;
	int64_t result;


	while (true) {
		printf("%s", message);
		input = get_input();
		begin_ptr = str_out(input);
		result = wcstoll(begin_ptr, &end_ptr, 10);
		if (*end_ptr != '\0' || result > max_value || result < min_value || *begin_ptr == '\0') {
			printf("%s\n", error_message);
			str_free(input);
			free(begin_ptr);
			continue;
		}
		free(input);
		return result;
	}
}


void order_food(csv_table *my_table) {
	int table_number = input_integer_question("Enter the table number: ", "Invalid table number.", 1, 100);
	csv_sheet *category_sheet = csv_table_get_sheet_by_key(my_table, "CATEGORY");
	csv_print_sheet(category_sheet, "%u", "Category no.");

	CATEGORY:
	{
		int category_number = input_integer_question("Enter the category number: ", "Invalid category number.", 1,
		                                             category_sheet->element_count);
		csv_element *category_name = csv_sheet_get_element_by_index(category_sheet, category_number - 1, 0);
		csv_sheet *food_sheet = csv_table_get_sheet_by_key(my_table, category_name);
		if (food_sheet->element_count == 0) {
			printf("Empty food list.\n");
			goto CATEGORY;
		}
		MENU:
		{
			csv_print_sheet(food_sheet, "%u", "Category no.");
			int food_number = input_integer_question("Enter the food number: ", "Invalid food number.", 1,
			                                         food_sheet->element_count);
		};


	}

}


void admin_section(csv_table *my_table) {
	printf("admin");
}

void main_menu(csv_table *my_table) {
	while (1) {
		printf("1)  Order food\n2)  Admin section\n3)  Exit\n");
		int64_t option = input_integer_question("Option: ", "Unknown option.", 0, 3);
		switch (option) {
			case 1:
				order_food(my_table);
			case 2:
				admin_section(my_table);
			case 3:
				exit(0);
		}
	}
}


int main(void) {
   csv_table *my_table = new_csv_table();
   csv_open("food.csv", my_table);

   main_menu(my_table);

//	string *my_str = new_string(L"f");
//	str_print(my_str);
//	printf("%i", str_cmpw(my_str, L"我"));




//   list *my_column = csv_sheet_get_column_by_name(csv_table_get_sheet_by_key(my_table, "MY_TABLE"), "2LOf2");
//   csv_element_print($(my_column, 1));
}



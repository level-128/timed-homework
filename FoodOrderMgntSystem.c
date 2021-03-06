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

// tested compile params:
// gcc -O3 -Wall -g -fsanitize=address -fsanitize=undefined -fsanitize-address-use-after-scope -fsanitize=leak FoodOrderMgntSystem.c

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
#include <termios.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>


#define $(list__, index__) list_get_node((list *)(list__), index__)->value
#define val(list__, index__) (((element *)(list_get_node((list *)(list__), index__)->value))->value)
#define type(x__) x__->type
#define len(x__) x__->list_object.len
#define ERROR(msg, ...) do {\
                        printf("\n\033[0;31mERROR -- at file: %s, line: %d\n\t" msg\
                        "\033[0m\n", __FILE__, __LINE__, ##__VA_ARGS__);           \
                        printf("program exits with error code %i\n", errno);       \
                        exit(errno);                                               \
                           } while (0)
#define NL putchar('\n');
#define RETURN goto RETURN_
#define printf_l(x__) printf("\n%s\n", x__)

#define TINT_ 1
#define TFLOAT_ 2
#define TSTRING_ 3
#define THEADER_ 4

char *MENU_FILE_NAME = "menu.csv";
char *TRANSITION_HISTORY_FILE_NAME = "transaction.csv";
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


// node in the list.
typedef struct node {
	 void *value; // stored value
	 struct node *next_node;
} node;

typedef node character;

typedef struct list {
	 size_t len; // the length of the linked list
	 node *last_node; // the addr of the last node
	 node *first_node; // the addr of the first node
	 size_t last_visit_node_index; // record the last visited index from method 'list_get_node'
	 node *last_visit_node; // record the last visited node from method 'list_get_node'
} list;

typedef struct string {
	 list list_object; // extend from list.
	 uint32_t ref_count; // reference count object.
} string;

typedef struct element {
	 char type; // the type of the element
	 union {
		  int64_t int_;
		  double float_;
		  string *string_;
	 } value;
} element;

typedef struct column {
	 list list_object;
	 node *father_node;
} column;

// the individual trunk in the dictionary.
typedef struct dict_chunk {
	 bool is_used; // true if the chunk is empty, false otherwise.
	 bool is_at_default_location;
	 // true   if the chunk stores itself at the index which it should belong.
	 // false  if the index which it should belong has been occupied by another chunk.
	 element *key;
	 column *value;
} dict_chunk;

typedef struct dict {
	 uint32_t size; // the size of the dictionary
	 uint32_t content_count; // how many chunk are there in the dictionary
	 uint64_t rand_salt; // the salt of the hash function, generated per dict object, for averaging the hash table distribution.
	 dict_chunk *dict_value;
} dict;

typedef struct sheet {
	 string *name;
	 uint32_t len;
	 column *titles; // Column[elements[String]]]
	 list *sheet_element; // List[columns]
	 // expend to
	 // sheet_element: List[columns[elements[int | float | string] ] ]
	 uint32_t sheet_index_row;
	 dict *sheet_index_dict;
} sheet;

typedef struct table {
	 list *table_list; // List[Sheet]
	 dict *table_dict;
} table;

union void_s_to_wchar {
	 void *void_s;
	 wchar_t wchar;
} str_conv;

union void_s_to_char {
	 void *void_s;
	 char char_;
} void_s_to_char;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// link list
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// create a new list
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

// return the node of the list at index i.
// to get the value of the list (aka List[i] in Python), use $(List, i).
node *list_get_node(list *self, uint32_t index) {
	if (index == 0) {
		return self->first_node;
	}
	if (index >= self->len || index < 0) {
		ERROR("list index out of range, list at %p has length %zu, but tries to index %u", self, self->len, index);
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

// append value into the list
void list_append(void *self, void *newval) {

	node *last_node = ((list *) self)->last_node;
	node *new_element = malloc(sizeof(node));

	new_element->next_node = ((list *) self)->first_node;

	last_node->value = newval;
	last_node->next_node = new_element;
	((list *) self)->last_node = new_element;
	((list *) self)->len++;
}

// insert newval before the index.
void list_insert_by_index(list *self, void *newval, uint32_t index) {
	node *new_node = malloc(sizeof(node));
	new_node->value = newval;
	self->len += 1;
	if (index == 0) {
		new_node->next_node = self->first_node;
		self->first_node = new_node;
		self->last_node->next_node = new_node;
	} else {
		node *list_prev_node = list_get_node(self, index - 1);
		node *list_aft_node = list_prev_node->next_node;
		list_prev_node->next_node = new_node;
		new_node->next_node = list_aft_node;
	}
	if (self->last_visit_node_index >= index) {
		self->last_visit_node_index += 1;
	}
}

// remove node node * pop_node in the list.
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

// pop the node at the index.
void list_pop_by_index(list *self, size_t index) {
	list_pop_by_node(self, list_get_node(self, index));
}
// free the list itself.
void list_free(list *self) {
	node *current_element = self->first_node;
	node *prev_element;
	for (int i = 0; i <= self->len; i++) { // the malloc node count is one more than the length
		// there is an empty node after the next node.
		prev_element = current_element;
		current_element = current_element->next_node;
		free(prev_element);
	}
	free(self);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// wide char str
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// append newval into string.
void str_append(string *self, wchar_t newval) {
	str_conv.wchar = newval;
	list_append(&self->list_object, str_conv.void_s);
}

// convert str[] into string. to create empty string, use new_str(NULL) or new_str(L"")
string *new_str(wchar_t str[]) {
	string *self = malloc(sizeof(string));
	character *my_element = malloc(sizeof(node));
	self->ref_count = 1;

	self->list_object.first_node = my_element;
	self->list_object.last_node = my_element;
	len(self) = 0;

	self->list_object.last_visit_node_index = 0;
	self->list_object.last_visit_node = my_element;

	my_element->next_node = self->list_object.first_node;

	if (str) {
		for (uint32_t i = 0; str[i] != '\0'; i++) {
			str_append(self, str[i]);
		}
	}
	return self;
}

// create a new string by copying all the characters in string_.
string *new_str_from_str(string *string_) {
	string *self = new_str(NULL);
	for (uint32_t i = 0; i < len(string_); i++) {
		list_append(self, $(string_, i));
	}
	return self;
}

// search wchar_t target[] in string, return the first occurrence, otherwise -1.
int32_t str_search(string *self, uint32_t index, const wchar_t target[]) {
	uint32_t target_str_len = wcslen(target);
	uint32_t fit_len = 0;
	for (; index < len(self); index++) {
		if (fit_len == target_str_len) {
			return (int32_t) (index - fit_len);
		}
		if ((intptr_t) ($(self, index)) == target[fit_len]) {
			fit_len += 1;
		} else {
			fit_len = 0;
		}
	}
	if (fit_len == target_str_len) {
		return (int32_t) (index - fit_len);
	} else {
		return -1;
	}
}

// insert the wchar_t *str before index.
// ex: "hello", str = L"#", index = 1, return: "h#ello".
void str_insert_by_index(string *self, wchar_t *str, uint32_t index) {
	size_t str_len = wcslen(str);
	for (uint32_t i = 0; i < str_len; i++) {
		str_conv.wchar = str[i];
		list_insert_by_index(&self->list_object, str_conv.void_s, index + i);
	}
}

// replace wchar_t *from to wchar_t *to, replace at most max_replace_time times.
// ex: "hello" replace "l" to "!!" return "he!!!!o"
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
			list_pop_by_node(&self->list_object, list_get_node(&self->list_object, index));
		}
		str_insert_by_index(self, to, index);
		index += char_to_len - char_from_len + 1;
	}
}

// decrease the reference count of the string. if the reference count reachs zero, free it.
void str_free(string *self) {
	if (self->ref_count == 1) {
		list_free((list *) self);
	} else {
		self->ref_count -= 1;
	}
}

// compare self and target
bool str_cmpw(string *self, wchar_t *target) {
	for (uint32_t i = 0; i < len(self); i++) {
		str_conv.void_s = $(self, i);
		if (str_conv.wchar != target[i]) {
			return false;
		}
	}
	return (bool) (!target[len(self)]);
}

// compare self and target
bool str_cmp(string *self, string *target) {
	if (len(self) != len(target)) {
		return false;
	}
	for (uint32_t i = 0; i < len(self); i++) {
		if ($(self, i) != $(target, i)) {
			return false;
		}
	}
	return true;
}

// print the string in terminal
void str_printf(string *self) {
	for (uint32_t i = 0; i < len(self); i++) {
		str_conv.void_s = ($(&self->list_object, i));
		putwchar(str_conv.wchar);
	}
}

// return a user input string.
string *str_input(char *prompt) {
	if (*prompt) {
		NL
		printf("%s", prompt);
	}
	string *input_list = new_str(NULL);
	int c;
	while ((c = getchar()) != '\n') {
		str_append(input_list, c);
	}
	return input_list;
}

// add the reference count by 1
string *str_cpy(string *self) {
	self->ref_count += 1;
	return self;
}

// convert string into wchar_t *
wchar_t *str_out(string *self) {
	wchar_t *my_str = malloc(sizeof(wchar_t) * (len(self) + 1));
	node *current_element = self->list_object.first_node;

	for (size_t i = 0; i < len(self); i++) {
		void_s_to_char.void_s = current_element->value;
		my_str[i] = (unsigned char) void_s_to_char.char_;
		current_element = current_element->next_node;
	}
	my_str[len(self)] = '\0';
	return my_str;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSV data structures:
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// create a new element with type int
element *ele_new_int(int64_t int_) {
	element *self = malloc(sizeof(element));
	self->value.int_ = int_;
	type(self) = TINT_;
	return self;
}

// create a new element with type float
element *ele_new_float(double float_) {
	element *self = malloc(sizeof(element));
	self->value.float_ = float_;
	type(self) = TFLOAT_;
	return self;
}

// create a new element with type string
element *ele_new_str(wchar_t *str_) {
	element *self = malloc(sizeof(element));
	string *string_ = new_str(str_);
	self->value.string_ = string_;
	type(self) = TSTRING_;
	return self;
}

// create a new element which points to the string *str_.
element *ele_new_str_from_str(string *str_) {
	element *self = malloc(sizeof(element));
	self->value.string_ = str_;
	type(self) = TSTRING_;
	return self;
}

// create a new header
element *ele_new_head_from_str(string *str_) {
//	assert($(str_, 0) == (void *) '\t');
	list_pop_by_index(&str_->list_object, 0); // remove leading tab
	element *self = malloc(sizeof(element));
	self->value.string_ = str_;
	type(self) = THEADER_;
	return self;
}

// compare two elements
bool ele_cmp(element *x, element *y) {
	if (type(x) != type(y)) {
		return false;
	}
	if (x->value.int_ == y->value.int_) {
		return true; // comparing the memory as if they are int.
	}
	if (type(x) == TSTRING_) {
		return str_cmp(x->value.string_, y->value.string_);
	} else {
		return false;
	}
}

// add elements, return x + y.
void ele_add(element *x, element *y, element *result) {
	if (type(x) == TINT_) {
		if (type(y) == TINT_) {
			result->value.int_ = x->value.int_ + y->value.int_;
			type(result) = TINT_;
			return;
		} else if (type(y) == TFLOAT_) {
			result->value.float_ = (double) x->value.int_ + y->value.float_;
			type(result) = TFLOAT_;
			return;
		}
	} else if (type(x) == TFLOAT_) {
		if (type(y) == TINT_) {
			result->value.float_ = x->value.float_ + (double) y->value.int_;
			type(result) = TINT_;
			return;
		} else if (type(y) == TFLOAT_) {
			result->value.float_ = x->value.float_ + (double) y->value.float_;
			type(result) = TFLOAT_;
			return;
		}
	}
	ERROR("Type Error: ele_add could only add int and float object");
}

// multiply elements, return x * y.
void ele_mul(element *x, element *y, element *result) {
	if (type(x) == TINT_) {
		if (type(y) == TINT_) {
			result->value.int_ = x->value.int_ * y->value.int_;
			type(result) = TINT_;
			return;
		} else if (type(y) == TFLOAT_) {
			result->value.float_ = (double) x->value.int_ * y->value.float_;
			type(result) = TFLOAT_;
			return;
		}
	} else if (type(x) == TFLOAT_) {
		if (type(y) == TINT_) {
			result->value.float_ = x->value.float_ * (double) y->value.int_;
			type(result) = TINT_;
			return;
		} else if (type(y) == TFLOAT_) {
			result->value.float_ = x->value.float_ * (double) y->value.float_;
			type(result) = TFLOAT_;
			return;
		}
	}
	ERROR("Type Error: ele_mul could only multiply int and float object");
}

// copy the element
element *ele_cpy(element *self) {
	element *new_element = malloc(sizeof(element));
	memcpy(new_element, self, sizeof(element));
	if (type(self) == TSTRING_ || type(self) == THEADER_) {
		str_cpy(self->value.string_);
	}
	return new_element;
}

void ele_printf(element *element_) {
	if (type(element_) == TFLOAT_) {
		printf("%.2lf", element_->value.float_);
	} else if (type(element_) == TINT_) {
		printf("%li", element_->value.int_);
	} else {
		str_printf(element_->value.string_);
	}
}

void ele_write(element *element_, FILE *stream) {
	if (type(element_) == TFLOAT_) {
		fprintf(stream, "%lf,", element_->value.float_);
	} else if (type(element_) == TINT_) {
		fprintf(stream, "%li,", element_->value.int_);
	} else {
		string *tmp_str = new_str_from_str(element_->value.string_);

		str_replace(tmp_str, L"\"", L"\"\"", UINT32_MAX);
		str_insert_by_index(tmp_str, L"\"", 0);
		str_append(tmp_str, L'\"');
		wchar_t *tmp_ = str_out(tmp_str);
		str_free(tmp_str);
		if (type(element_) == THEADER_) {
			fprintf(stream, "\t%ls,", tmp_);
		} else {
			fprintf(stream, "%ls,", tmp_);
		}
		free(tmp_);
	}
}

void ele_free(element *self) {
	if (type(self) == TSTRING_) {
		str_free(self->value.string_);
	}
	free(self);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//csv column
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

column *new_column(void) {
	column *self = malloc(sizeof(column));

	character *my_element = malloc(sizeof(node));

	self->list_object.first_node = my_element;
	self->list_object.last_node = my_element;
	len(self) = 0;

	self->list_object.last_visit_node_index = 0;
	self->list_object.last_visit_node = my_element;

	my_element->next_node = self->list_object.first_node;
	return self;
}

// append items into column. for example:
// col_appends(my_column, 3, TINT_, 213, TFLOAT_, 213.213, TSTRING_, L"hello");
// will append 3 items with type int, float and string.
column *col_appends(column *self, int arg_count, ...) {
	va_list args;
	int var_type;
	va_start(args, arg_count);
	for (uint32_t i = 0; i < arg_count; i++) {
		var_type = va_arg(args, long);
		if (var_type == TINT_) {
			list_append(self, ele_new_int(va_arg(args, int64_t)));
		} else if (var_type == TFLOAT_) {
			list_append(self, ele_new_float(va_arg(args, double)));
		} else if (var_type == TSTRING_) {
			list_append(self, ele_new_str(va_arg(args, wchar_t*)));
		} else if (var_type == 0) {
			list_append(self, va_arg(args, element*));
		}
	}
	return self;
}

column *column_copy(column *self) {
	column *self_new = new_column();
	for (uint32_t i = 0; i < len(self); i++) {
		list_append(&self_new->list_object, ele_cpy($(self, i)));
	}
	return self_new;
}

void col_free(column *self) {
	for (uint32_t i = 0; i < len(self); i++) {
		ele_free($(&self->list_object, i));
	}
	list_free((list *) self);
}

void col_printf(column *column) {
	for (uint32_t i = 0; i < len(column) - 1; i++) {
		ele_printf($(column, i));
		printf("\t");
	}
	ele_printf($(column, len(column) - 1));
	NL
}

void col_write_out(column *self, FILE *stream) {
	for (uint32_t i = 0; i < len(self); i++) {
		ele_write($(self, i), stream);
	}
	fprintf(stream, "\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// dictionary object
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void p__dict_remap(dict *self, size_t new_size);

dict_chunk *p__dict_find_trunk(dict *self, element *key);

void dict_store(dict *self, element *key, void *value);


uint32_t p__dict_hash(const element *key, uint32_t seed) {
	if (type(key) == TSTRING_) {
		wchar_t *string_ = str_out(key->value.string_);
		uint64_t result = 0;
		for (uint32_t i = 0;; i++) {
			if (string_[i] == 0) {
				break;
			}
			result += (uint64_t) string_[i] << 56;
			result %= PRIME;
		}
		free(string_);
		return (result % 0xFFFFFFFF) ^ seed;
	}
	int64_t key_ = key->value.int_;
	// hashing the address of the element
	return (intptr_t) key_ * ((int64_t) seed + (0L << 31)) % 1 << 32;
	// this code uses undefined behaviour (integer overflow) because of performance consideration, and the fact that in
	// 64-bit systems long integer overflow will cause wraparound (because the register size are 64 bit max).
}

// create a new dictionary object
dict *new_dict(size_t size) {
	if (!size) {
		size = DEFAULT_DICT_SIZE;
	}

	dict *self = malloc(sizeof(dict));
	self->dict_value = calloc(size, sizeof(dict_chunk));
	self->size = size;
	self->rand_salt = rand();
	return self;
}

void p__dict_remap(dict *self, size_t new_size) {
	dict_chunk *old_dict = self->dict_value;
	dict_chunk *tmp_;
	size_t size = self->size;

	self->dict_value = calloc(sizeof(dict_chunk), new_size);
	self->content_count = 0;
	self->size = new_size;

	for (size_t i = 0; i < size; i++) {
		tmp_ = old_dict + i;
		if (tmp_->is_used) {
			dict_store(self, tmp_->key, tmp_->value);
		}
	}
	free(old_dict);
}

// store the value
void dict_store(dict *self, element *key, void *value) {
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
			if (ele_cmp(key, self_trunk->key)) {
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

dict_chunk *p__dict_find_trunk(dict *self, element *key) {
	uint32_t hash_value = p__dict_hash(key, self->rand_salt);
	uint32_t location = hash_value % self->size;
	dict_chunk *self_trunk = self->dict_value + location;
	if (!self_trunk->is_used) {
		return NULL;
	}
	if (ele_cmp(self_trunk->key, key)) {
		return self_trunk;
	}
	location += 1;
	for (location = location % self->size;; location++) {
		self_trunk = self->dict_value + location;
		if (self_trunk->is_at_default_location) {
			continue;
		}
		if (self_trunk->is_used) {
			if (ele_cmp(self_trunk->key, key)) {
				return self_trunk;
			}
		} else {
			return NULL;
		}
	}
}

// find the value from key
void *dict_find(dict *self, element *key) {
	dict_chunk *tmp_ = p__dict_find_trunk(self, key);
	if (tmp_) {
		return tmp_->value;
	}
	return NULL;
}

// delete the record of the key-value pair in dictionary. Does not free the value.
void dict_pop(dict *self, element *key) {
	if ((self->content_count < REHASH_LIMIT * self->size / 2) && self->size > DEFAULT_DICT_SIZE) {
		p__dict_remap(self, (size_t) (self->size / 1.5));
	}

	dict_chunk *self_trunk = p__dict_find_trunk(self, key);
	memset(self_trunk, 0, sizeof(dict_chunk));
	self->content_count -= 1;
}

// free the dictionary.
void dict_free(dict *self) {
	free(self->dict_value);
	free(self);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

sheet *sheet_new(string *sheet_name, column *sheet_titles, uint32_t sheet_index_row) {
	sheet *self = malloc(sizeof(sheet));

	self->len = 0;
	self->name = sheet_name;
	self->titles = sheet_titles;
	self->sheet_index_row = sheet_index_row;

	self->sheet_element = new_list();
	self->sheet_index_dict = new_dict(0);
	return self;
}

sheet *sheet_new_from_header(column *my_properties) {
	string *sheet_name = str_cpy(val(my_properties, 0).string_);
	uint32_t sheet_index_row = val(my_properties, len(my_properties) - 1).int_;
	ele_free($(my_properties, 0));
	ele_free($(my_properties, len(my_properties) - 1));
	list_pop_by_index(&my_properties->list_object, 0);
	list_pop_by_index(&my_properties->list_object, len(my_properties) - 1);
	sheet *ret = sheet_new(sheet_name, my_properties, sheet_index_row);
	return ret;
}

void sheet_append(sheet *self, column *column) {
	column->father_node = self->sheet_element->last_node;
	list_append(self->sheet_element, column);

	if (self->sheet_index_row != -1) {
		dict_store(self->sheet_index_dict, $(column, self->sheet_index_row), column);
	}
	self->len += 1;
}

column *sheet_get_col_by_name(sheet *self, element *index) {
	return dict_find(self->sheet_index_dict, index);
}

column *sheet_get_col_by_index(sheet *self, uint32_t index) {
	return $(self->sheet_element, index);
}

element *sheet_get_ele_by_index(sheet *self, uint32_t column, uint32_t row) {
	return $($(self->sheet_element, column), row);
}

void sheet_pop(sheet *self, column *my_column) {
	element *hash_element = $(my_column, self->sheet_index_row);
	dict_pop(self->sheet_index_dict, hash_element);
	// because pop the list need to swap this node and the next node, assign the next column's father node as this node.
	if (my_column->father_node->next_node != self->sheet_element->last_node) { // the last node's next node is an empty node
		((column *) my_column->father_node->next_node->value)->father_node = my_column->father_node;
	}
	list_pop_by_node(self->sheet_element, my_column->father_node); // pop the column which stores in self->sheet_element
	col_free(my_column);
	self->len -= 1;
}

void sheet_pop_by_index(sheet *self, uint32_t index) {
	column *my_column = sheet_get_col_by_index(self, index);
	sheet_pop(self, my_column);
}

__attribute__((unused)) void sheet_pop_by_name(sheet *self, element *index) {
	column *my_column = sheet_get_col_by_name(self, index);
	sheet_pop(self, my_column);
}

void sheet_printf(sheet *self_sheet, char *index_prefix, char *index_name) {
	if (self_sheet->len == 0) {
		printf_l("Empty category list");
		return;
	}
	NL
	bool is_print_index = strlen(index_prefix) && strlen(index_name);
	if (is_print_index) {
		printf("%s\t", index_name);
	}
	col_printf(self_sheet->titles);
	NL
	for (uint32_t column_number = 0; column_number < self_sheet->len; column_number++) {
		if (is_print_index) {
			printf(index_prefix, column_number + 1);
			printf("\t");
		}
		col_printf(sheet_get_col_by_index(self_sheet, column_number));
	}
}

void sheet_write_out(sheet *self, FILE *stream) {
	element *sheet_name = ele_new_str_from_str(str_cpy(self->name));
	// the self->name is a single string, and for printing the name, we need to create an element string
	// contains the name, and use it to print, then free it.
	type(sheet_name) = THEADER_;
	ele_write(sheet_name, stream);
	type(sheet_name) = TSTRING_;

	// write titles by adding index row at the end of the column.
	element *tmp_ = ele_new_int(self->sheet_index_row);
	list_append(&self->titles->list_object, tmp_);
	col_write_out(self->titles, stream);
	ele_free(tmp_);
	list_pop_by_index(&self->titles->list_object, len(self->titles) - 1);

	for (uint32_t i = 0; i < self->len; i++) {
		ele_write(sheet_name, stream); // write the column title;
		col_write_out($(self->sheet_element, i), stream);
	}
	ele_free(sheet_name);
}

void sheet_free(sheet *self) {
	str_free(self->name);
	col_free(self->titles);
	for (uint32_t i = 0; i < self->len; i++) {
		col_free(sheet_get_col_by_index(self, i));
	}
	list_free(self->sheet_element);
	dict_free(self->sheet_index_dict);
	free(self);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

table *new_table(void) {
	table *self = malloc(sizeof(table));
	self->table_dict = new_dict(0);
	self->table_list = new_list();
	return self;
}

void table_append_sheet(table *self, sheet *_element) {
	dict_store(self->table_dict, ele_new_str_from_str(_element->name), _element);
	list_append(self->table_list, _element);
}

sheet *table_get_sheet_by_key(table *self, element *key) {
	return dict_find(self->table_dict, key);
}

sheet *table_get_sheet_by_name(table *self, wchar_t *name) {
	element *tmp_ = ele_new_str(name);
	sheet *tmp_sheet = table_get_sheet_by_key(self, tmp_);
	ele_free(tmp_);
	return tmp_sheet;
}

void sheet_append_by_key(table *self, element *key, column *column_) {
	sheet_append(table_get_sheet_by_key(self, key), column_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// parser
list *p__csv_parse_split_line(char *line, char **buf_con) {
	// input: "hi, 33.3"
	// return: [['h', 'i'], ['3', '3', '.', '3']]

	list *result_list = new_list();
	string *buf = new_str(NULL);

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
			buf = new_str(NULL);
			is_start_element = false;
		} else {
			is_start_element = true;
			str_append(buf, line[i]);
		}
	}
	if (len(buf) > 0) {
		list_append(result_list, buf);
	} else {
		str_free(buf);
	}
	*buf_con = line + i;

	return result_list;
}

element *p__csv_parse_element(string *element) {
	// input: ['3', '3', '.', '3']
	// return element{type = TFLOAT_, value = 33.3}

	wchar_t *element_str = str_out(element);
	wchar_t *endptr = NULL;

	int64_t int_ = wcstoll(element_str, &endptr, 10);
	if (endptr[0] == '\0') {
		str_free(element);
		free(element_str);
		return ele_new_int(int_);
	}

	double float_ = wcstod(element_str, &endptr);
	if (endptr[0] == '\0') {
		str_free(element);
		free(element_str);
		return ele_new_float(float_);
	}

	bool is_header = element_str[0] == '\t';
	if (element_str[0 + is_header] == '"' && element_str[len(element) - 1] == '"') {
		list_pop_by_index(&element->list_object, 0 + is_header);
		list_pop_by_index(&element->list_object, len(element) - 1);
	}

	str_replace(element, L"\"\"", L"\"", UINT32_MAX);
	if (is_header) {
		free(element_str);

		return ele_new_head_from_str(element);
	} else {
		free(element_str);
		return ele_new_str_from_str(element);
	}

}

column *p__csv_parse_line(char *line, char **buf_con) {
	// parse a line from *line to *buf_con, and return the parsed element in a list.
	// return type: List[csv_elements{}]
	list *list_line = p__csv_parse_split_line(line, buf_con);
	column *list_result = new_column();
	string *list_element;

	for (int i = 0; i < list_line->len; i++) {
		list_element = $(list_line, i);
		list_append(list_result, p__csv_parse_element(list_element));
	}
	list_free(list_line);
	return list_result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//file IO

bool p__check_file(const char *file_name) {
	if (access(file_name, F_OK | R_OK | W_OK) != 0) {
		return false;
	}
	return true;
}

void p__create_menu_file(const char *file_name) {
	FILE *file_handle = fopen(file_name, "w");
	column *sheet_titles = new_column();

	list_append(&sheet_titles->list_object, ele_new_str(L"Name"));
	sheet *menu = sheet_new(
			  new_str(L"MENU"),
			  sheet_titles,
			  0
	);
	sheet_write_out(menu, file_handle);
	sheet_free(menu);
	fclose(file_handle);
}

void p__create_order_file(const char *file_name) {
	FILE *file_handle = fopen(file_name, "w");

	column *transaction_history_sheet_titles = new_column();
	col_appends(transaction_history_sheet_titles, 7,
	            TSTRING_, L"Table no.",
	            TSTRING_, L"Payment method",
	            TSTRING_, L"Card no.",
	            TSTRING_, L"Card holder's name",
	            TSTRING_, L"Amt.",
	            TSTRING_, L"Date",
	            TSTRING_, L"Time");
	sheet *transaction_history = sheet_new(
			  new_str(L"TRANSACTION_HISTORY"),
			  transaction_history_sheet_titles,
			  0
	);
	sheet_write_out(transaction_history, file_handle);
	sheet_free(transaction_history);

	sheet *password_sheet = sheet_new(
			  new_str(L"PASSWORD"),
			  new_column(),
			  0
	);
	column *password = new_column();
	list_append(password, ele_new_str(L"admin"));
	sheet_append(password_sheet, password);
	sheet_write_out(password_sheet, file_handle);
	sheet_free(password_sheet);
	fclose(file_handle);
}

void csv_open_file(const char *file_name, table *table) {
	struct stat stat_;
	if (!p__check_file(file_name)) {
		p__create_menu_file(file_name);
	}
	int file_handle = open(file_name, O_RDWR | O_CREAT);
	if (file_handle < 0) {
		ERROR("csv table load failed, reason: %s", strerror(errno));
	}
	if (stat(file_name, &stat_) == -1) {
		ERROR("can't read the status of the file: %s, reason %s", file_name, strerror(errno));
	}
	if (stat_.st_size == 0) {
		ERROR("the file: %s is empty. ", file_name);
	}

	char *file_ptr = mmap(NULL, stat_.st_size, PROT_READ, MAP_PRIVATE, file_handle, 0);
	close(file_handle); // close the file handle DOES NOT unmap the file.

	if (file_ptr == (char *) 0xffffffffffffffff) {
		ERROR("MMIO failed, with reason: %s"
		      ". make sure the file is not located at the remote machine or access across VMs (for example: visiting file outside WSL).",
		      strerror(errno));
	}

	char *buf_con = file_ptr;
	column *my_column;

	while (true) {
		my_column = p__csv_parse_line(buf_con, &buf_con);
		if (len(my_column) == 0) {
			col_free(my_column);
			continue;
		}
		element *first_element = $(my_column, 0);
		if (type(first_element) == THEADER_) {
			table_append_sheet(table, sheet_new_from_header(my_column));
		} else {
			list_pop_by_index(&my_column->list_object, 0);
			sheet_append_by_key(table, first_element, my_column);
			ele_free(first_element);
		}
		// end of file
		if (buf_con[0] == '\0') {
			break;
		}
	}
	munmap(file_ptr, stat_.st_size);
}

void csv_open(table *self) {
	if (!p__check_file(MENU_FILE_NAME)) {
		p__create_menu_file(MENU_FILE_NAME);
	}
	if (!p__check_file(TRANSITION_HISTORY_FILE_NAME)) {
		p__create_order_file(TRANSITION_HISTORY_FILE_NAME);
	}
	csv_open_file(MENU_FILE_NAME, self);
	csv_open_file(TRANSITION_HISTORY_FILE_NAME, self);
}

bool flush_file(table *self, char *succeed_prompt, const char *failed_prompt) {
	NL
	FILE *menu_file = fopen(MENU_FILE_NAME, "w");
	if (menu_file == NULL) {
		if (!*failed_prompt) {
			ERROR("failed when tries to write file '%s' reason: %s", MENU_FILE_NAME, strerror(errno));
		} else {
			printf("%s\n", failed_prompt);
			return false;
		}
	}
	sheet *category_sheet = table_get_sheet_by_name(self, L"MENU");
	sheet_write_out(category_sheet, menu_file);
	// write sub-sheets under sheet "MENU"
	for (uint32_t i = 0; i < category_sheet->len; i++) {
		wchar_t *sheet_name = str_out(sheet_get_ele_by_index(category_sheet, i, 0)->value.string_);
		sheet_write_out(table_get_sheet_by_name(self, sheet_name), menu_file);
		free(sheet_name);
	}
	fclose(menu_file);

	FILE *trans_hist_file = fopen(TRANSITION_HISTORY_FILE_NAME, "w");
	if (menu_file == NULL) {
		if (*failed_prompt) {
			ERROR("failed when tries to write file '%s' reason: %s", TRANSITION_HISTORY_FILE_NAME, strerror(errno));
		} else {
			printf("%s\n", failed_prompt);
			return false;
		}
	}
	sheet *trans_hist_sheet = table_get_sheet_by_name(self, L"TRANSACTION_HISTORY");
	sheet *password_sheet = table_get_sheet_by_name(self, L"PASSWORD");
	sheet_write_out(trans_hist_sheet, trans_hist_file);
	sheet_write_out(password_sheet, trans_hist_file);
	fclose(trans_hist_file);
	printf("%s\n", succeed_prompt);
	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utility functions:

bool input_yn_question(char *message) {
	string *input;
	while (true) {
		NL
		printf("%s (y/n)?: ", message);
		input = str_input("");
		if (str_cmpw(input, L"y") || str_cmpw(input, L"Y")) {
			str_free(input);
			return true;
		}
		if (str_cmpw(input, L"n") || str_cmpw(input, L"N")) {
			str_free(input);
			return false;
		}
		str_free(input);
		printf_l("Invalid input.");
	}
}

int64_t input_integer_question(char *message, char *error_message, int64_t min_value, int64_t max_value) {
	string *input;
	wchar_t *begin_ptr = NULL;
	wchar_t *end_ptr = NULL;
	int64_t result;


	while (true) {
		input = str_input(message);
		begin_ptr = str_out(input);
		result = wcstoll(begin_ptr, &end_ptr, 10);
		if (*end_ptr != '\0' || result > max_value || result < min_value || *begin_ptr == '\0') {
			printf_l(error_message);
			str_free(input);
			free(begin_ptr);
			continue;
		}
		str_free(input);
		free(begin_ptr);
		return result;
	}
}

element *input_number_question(char *message, char *error_message) {
	while (true) {
		string *input = str_input(message);
		wchar_t *input_str = str_out(input);
		str_free(input);

		wchar_t *end_ptr = L"";

		int64_t result_int = wcstoll(input_str, &end_ptr, 10);
		if (end_ptr[0] == '\0') {
			free(input_str);
			return ele_new_int(result_int);
		}
		double result_float = wcstod(input_str, &end_ptr);
		if (end_ptr[0] == '\0') {
			free(input_str);
			return ele_new_float(result_float);
		}
		free(input_str);
		printf_l(error_message);
	}
}

string *p__input_password(char *msg) {
	if (*msg) {
		NL
		printf("%s", msg);
	}
	struct termios old_cfg, new_cfg;
	string *password = new_str(L"");
	int c;

	tcgetattr(0, &old_cfg);
	new_cfg = old_cfg;
	new_cfg.c_lflag &= ~ICANON & ~ECHO;
	tcsetattr(0, TCSANOW, &new_cfg);

	while ((c = getchar()) != '\n') {
		if (c == 127 && len(password) != 0) {
			putchar(0x8);
			putchar(' ');
			putchar(0x8);
			list_pop_by_index(&password->list_object, len(password) - 1);
		} else if (c != 127) {
			putchar('*');
			str_append(password, c);
		}
	}
	// restore terminal settings.
	tcsetattr(0, TCSANOW, &old_cfg);

	putchar('\n');
	return password;
}

bool p__is_valid_date(uint32_t year, uint8_t month, uint8_t day) {
	year += 2000;
	if (month > 12 || month < 1) {
		return false;
	}
	bool is_leap_year = false;
	if (year % 4 == 0 && year % 100 == 0 && year % 400 == 0) {
		is_leap_year = true;
	} else if (year % 4 == 0 && year % 100 != 0) {
		is_leap_year = true;
	}
	int date_for_each_month[] = {31, is_leap_year ? 28 : 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (day > date_for_each_month[month - 1]) {
		return false;
	}
	return true;
}

bool p__parse_date(string *raw_date, uint8_t *year, uint8_t *month, uint8_t *day) {
	if (len(raw_date) != 8) {
		return false;
	}
	wchar_t *raw_input_str = str_out(raw_date);
	if (raw_input_str[2] != L'/' || raw_input_str[5] != L'/') {
		free(raw_input_str);
		return false;
	}
	raw_input_str[2] = L'\0';
	raw_input_str[5] = L'\0';
	wchar_t *ptr;

	*year = wcstoul(raw_input_str, &ptr, 10);
	if (*ptr) {
		free(raw_input_str);
		return false;
	}
	*month = wcstoul(raw_input_str + 3, &ptr, 10);
	if (*ptr) {
		free(raw_input_str);
		return false;
	}
	*day = wcstoul(raw_input_str + 6, &ptr, 10);
	if (*ptr) {
		free(raw_input_str);
		return false;
	}
	free(raw_input_str);
	return true;
}

void p__input_date(uint8_t *year, uint8_t *month, uint8_t *day, char *msg) {
	while (true) {
		string *raw_input = str_input(msg);

		if (!p__parse_date(raw_input, year, month, day) || !p__is_valid_date(*year, *month, *day)) {
			printf_l("Invalid date.");
			str_free(raw_input);
			continue;
		}
		str_free(raw_input);
		return;
	}
}

int p__compare_date(uint8_t year1, uint8_t month1, uint8_t day1, uint8_t year2, uint8_t month2, uint8_t day2) {
	if (year1 != year2) {
		return (year1 > year2) * 2 - 1;
	}
	if (month1 != month2) {
		return (month1 > month2) * 2 - 1;
	}
	if (day1 != day2) {
		return (day1 > day2) * 2 - 1;
	}
	return 0;
}

bool check_password(table *self, string * input_password) {
	string *password =
			  sheet_get_ele_by_index(table_get_sheet_by_name(self, L"PASSWORD"), 0, 0)->value.string_;
	if (!str_cmp(password, input_password)) {
		printf_l("Invalid password.");
		str_free(input_password);
		return false;
	}
	str_free(input_password);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// admin options:
void change_password(table *self) {
	INPUT_PW:{}
	if (!check_password(self, p__input_password("Enter the old password: "))) {
		if (input_yn_question("Do you want to change the password")) {
			goto INPUT_PW;
		} else {
			return;
		}
	}
	ENTER_NEW_PW:
	{};
	string *new_password = p__input_password("Enter new password: ");
	if (len(new_password) < 5 || len(new_password) > 15) {
		printf_l("Password can???t be less than 5 characters and more than 15 characters.");
		str_free(new_password);
		goto ENTER_NEW_PW;
	}
	string *new_password2 = p__input_password("Enter the new password again: ");
	if (!str_cmp(new_password, new_password2)) {
		printf_l("Re-entered new password and new password don???t match.");
		str_free(new_password);
		str_free(new_password2);
		goto ENTER_NEW_PW;
	}
	str_free(new_password2);
	element *password = sheet_get_ele_by_index(table_get_sheet_by_name(self, L"PASSWORD"), 0, 0);
	str_free(password->value.string_);
	password->value.string_ = new_password;
	printf_l("Password successfully changed.");
}

void add_category(table *self) {
	sheet *menu_sheet = table_get_sheet_by_name(self, L"MENU");
	ADD_CATEGORY:{};

	element *category_name = ele_new_str_from_str(str_input("Enter a category name: "));
	if (len(category_name->value.string_) == 0) {
		printf("Invalid input.\n");
		ele_free(category_name);
		goto ADD_CATEGORY;
	}
	if (len(category_name->value.string_) > 50) {
		printf("Category name can???t be more than 50 characters.\n");
		ele_free(category_name);
		goto ADD_CATEGORY;
	}
	if (sheet_get_col_by_name(menu_sheet, category_name)) {
		printf("The category you entered is already on the list.\n");
		ele_free(category_name);
		goto ADD_CATEGORY;
	} else {
		column *new_col = new_column();
		list_append(new_col, category_name);
		sheet_append(menu_sheet, new_col);

		column *food_sheet_column = new_column();
		col_appends(food_sheet_column, 4,
		            TSTRING_, L"Name",
		            TSTRING_, L"Price",
		            TSTRING_, L"Availability",
		            TSTRING_, L"Description");
		sheet *food_sheet = sheet_new(
				  str_cpy(category_name->value.string_),
				  food_sheet_column,
				  0
		);
		table_append_sheet(self, food_sheet);
		flush_file(self, "Successfully created category.", "Category creation unsuccessful.");
	}
	if (input_yn_question("Do you want to add more categories")) {
		goto ADD_CATEGORY;
	}
}

void delete_category(table *self) {
	sheet *menu_sheet = table_get_sheet_by_name(self, L"MENU");
	if (menu_sheet->len == 0) {
		printf_l("Empty category list.");
		return;
	}
	while (true) {
		int64_t index = input_integer_question("Enter a category number: ", "Invalid category number.", 1,
		                                       menu_sheet->len);
		sheet_pop_by_index(menu_sheet, index - 1);

		flush_file(self, "Successfully deleted.", "Unsuccessful deletion.");
		if (!input_yn_question("Do you want to delete more categories")) {
			break;
		}
	}
}

void view_category(table *self) {
	sheet *menu_sheet = table_get_sheet_by_name(self, L"MENU");
	sheet_printf(menu_sheet, "%u", "Category no.");
}

void add_food_item(table *self) {
	sheet *menu_sheet = table_get_sheet_by_name(self, L"MENU");
	if (menu_sheet->len == 0) {
		printf_l("Empty category list.");
		return;
	}
	while (true) {
		int64_t category_number = input_integer_question("Enter a category number:", "Invalid category number.", 1,
		                                                 menu_sheet->len);
		sheet *food_sheet = table_get_sheet_by_key(self, sheet_get_ele_by_index(menu_sheet, category_number - 1, 0));

		element *food_name = ele_new_str_from_str(str_input("Enter food name: "));

		if (sheet_get_col_by_name(food_sheet, food_name)) {
			printf("The food item already exists in the food list in the same category.\n");
			ele_free(food_name);
		} else {
			element *price = input_number_question("Enter price: ", "Invalid input");
			int64_t availability = input_integer_question("Enter availability: ", "Invalid input.", 0, 999999999);
			string *description = str_input("Enter a description: ");

			column *food_column = new_column();
			list_append(food_column, food_name);
			list_append(food_column, price);
			list_append(food_column, ele_new_int(availability));
			list_append(food_column, ele_new_str_from_str(description));

			sheet_append(food_sheet, food_column);
			flush_file(self, "Successfully created food record.", "Food record creation unsuccessful.");
		}
		if (!input_yn_question("Do you want to add more food records")) {
			break;
		}
	}
}

void delete_food_item(table *self) {
	while (true) {
		sheet *menu_sheet = table_get_sheet_by_name(self, L"MENU");
		if (menu_sheet->len == 0) {
			printf_l("Empty category list.");
			return;
		}
		int64_t category_number = input_integer_question("Enter a category number:", "Invalid input.", 1, menu_sheet->len);
		sheet *food_sheet = table_get_sheet_by_key(self, sheet_get_ele_by_index(menu_sheet, category_number - 1, 0));

		if (food_sheet->len == 0) {
			printf_l("Empty food list.");
			return;
		}
		int64_t food = input_integer_question("Enter a food number:", "Invalid input.", 1, food_sheet->len);

		sheet_pop_by_index(food_sheet, food - 1);
		flush_file(self, "Successfully deleted.", "Unsuccessful deletion.");
		if (!input_yn_question("Do you want to delete more food items")) {
			break;
		}
	}
}

void view_food_items(table *self) {
	sheet *menu_sheet = table_get_sheet_by_name(self, L"MENU");
	if (menu_sheet->len == 0) {
		printf_l("Empty category list.");
		return;
	}
	int64_t category_number = input_integer_question("Enter a category number:", "Invalid input.", 1, menu_sheet->len);
	element *category_name = sheet_get_ele_by_index(menu_sheet, category_number - 1, 0);

	wchar_t *temp = str_out(category_name->value.string_);
	printf("Category name: %ls\n\n", temp);
	free(temp);
	sheet_printf(table_get_sheet_by_key(self, category_name), "%i", "Food no.");
}

void show_transit_history(table *self) {
	uint8_t year_from, year_to, month_from, month_to, day_from, day_to;
	JMP1:
	p__input_date(&year_from, &month_from, &day_from, "Enter start date: ");
	p__input_date(&year_to, &month_to, &day_to, "Enter end date: ");
	if (p__compare_date(year_from, month_from, day_from, year_to, month_to, day_to) == 1) {
		printf_l("Start date must be on or before the end date.");
		goto JMP1;
	}
	NL
	uint8_t year, month, day;
	element *total = ele_new_float(0.0);
	sheet *transit_sheet = table_get_sheet_by_name(self, L"TRANSACTION_HISTORY");
	col_printf(transit_sheet->titles);
	NL
	for (uint32_t column_number = 0; column_number < transit_sheet->len; column_number++) {
		p__parse_date(val(sheet_get_col_by_index(transit_sheet, column_number), 5).string_,
		              &year, &month, &day);
		if (p__compare_date(year, month, day, year_from, month_from, day_from) != -1) {
			if (p__compare_date(year, month, day, year_to, year_to, day_to) != 1) {
				col_printf(sheet_get_col_by_index(transit_sheet, column_number));
				ele_add($(sheet_get_col_by_index(transit_sheet, column_number), 4), total, total);
			}
		}
	}
	NL
	printf("Total amount: ");
	ele_printf(total);
	NL
	ele_free(total);
}

void admin_section(table *self) {
	string *input_password = p__input_password("Enter the password: ");
	if (!check_password(self, input_password)) {
		return;
	}
	while (true) {
		NL
		printf("1. Add a category\n"
		       "2. Delete a category\n"
		       "3. View categories\n"
		       "4. Add a food item\n"
		       "5. Delete a food item\n"
		       "6. View food items\n"
		       "7. Show transaction history\n"
		       "8. Change password\n"
		       "9. Exit\n");
		switch (input_integer_question("Option: ", "Unknown option.", 1, 9)) {
			case 1:
				add_category(self);
				break;
			case 2:
				delete_category(self);
				break;
			case 3:
				view_category(self);
				break;
			case 4:
				add_food_item(self);
				break;
			case 5:
				delete_food_item(self);
				break;
			case 6:
				view_food_items(self);
				break;
			case 7:
				show_transit_history(self);
				break;
			case 8:
				change_password(self);
				break;
			default:
//				flush_file(self, "", "");
				// not save tolerant, if failed to flush the file, prompt error message and then press enter to crash the program.
				// un-commit this line of code to activate this feature.
				return;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// order functions:
sheet *p__ordered_item_sheet_create(void) {
	column *ordered_items_title = new_column();
	col_appends(ordered_items_title, 5,
	            TSTRING_, L"Food no.",
	            TSTRING_, L"Name",
	            TSTRING_, L"Price",
	            TSTRING_, L"Qty",
	            TSTRING_, L"Description");

	return sheet_new(new_str(L"ORDERED_ITEMS"), ordered_items_title, 0);
}

void p__ordered_item_sheet_append(sheet *self, column *food_column, int category_number, int food_number, int qty) {
	wchar_t food_no[20] = L"";
	swprintf(food_no, 20, L"%i-%i", category_number, food_number);
	column *ordered_items_column = column_copy(food_column);
	list_insert_by_index(&ordered_items_column->list_object, ele_new_str(food_no), 0);
	val(ordered_items_column, 3).int_ = qty;
	sheet_append(self, ordered_items_column);
	// deduce the qty
	val(food_column, 2).int_ -= qty;
}

void p__ordered_item_sheet_del(table *self, sheet *order_sheet, element *food_no) {
	GOTO1:
	if (food_no == NULL) {
		food_no = ele_new_str_from_str(str_input("Enter the food number you want to remove from the selected list: "));
		if (sheet_get_col_by_name(order_sheet, food_no) == NULL) {
			printf_l("Invalid food number.");
			ele_free(food_no);
			food_no = NULL;
			goto GOTO1;
		}
	}
	column *food_column = sheet_get_col_by_name(order_sheet, food_no);
	int qty = val(food_column, 3).int_;
	int category_number, food_number;
	wchar_t *tmp_ = str_out(food_no->value.string_);
	swscanf(tmp_, L"%i-%i", &category_number, &food_number);
	free(tmp_);

	sheet_pop(order_sheet, food_column);
	sheet *menu_sheet = table_get_sheet_by_name(self, L"MENU");
	element *food_sheet_name = sheet_get_ele_by_index(menu_sheet, category_number - 1, 0);
	sheet *food_sheet = table_get_sheet_by_key(self, food_sheet_name);
	sheet_get_ele_by_index(food_sheet, food_number - 1, 2)->value.int_ += qty;
	ele_free(food_no);
}

element *p__ordered_item_print(sheet *self) {
	sheet_printf(self, "", "");

	element *sum = ele_new_float(0.0);
	element *tmp1 = ele_new_int(0);
	for (uint32_t i = 0; i < self->len; i++) {
		ele_mul($(sheet_get_col_by_index(self, i), 3), $(sheet_get_col_by_index(self, i), 2), tmp1);
		ele_add(tmp1, sum, sum);
	}
	ele_free(tmp1);
	NL
	printf("Total: ");
	ele_printf(sum);
	NL
	return sum;
}

bool p__check_is_ordered(sheet *self, int category_number, int food_number) {
	wchar_t food_no[20] = L"";
	bool result = false;
	swprintf(food_no, 20, L"%i-%i", category_number, food_number);
	element *tmp_ = ele_new_str(food_no);
	if (sheet_get_col_by_name(self, tmp_)) {
		result = true;
	}
	ele_free(tmp_);
	return result;
}

void p__payment(table *my_table, int table_number, element *amount) {
	bool is_cash = input_integer_question("1. Card 2. Cash (1/2)?: ", "Invalid input.", 1, 2) - 1;
	string *card_number = NULL;
	string *card_holder_name = NULL;
	wchar_t *payment_method = is_cash ? L"Cash" : L"Card";

	if (!is_cash) {
		wchar_t tmp_[10] = L"";
		swprintf(tmp_, 10, L"%li",
		         input_integer_question("Enter card number: ", "Invalid card number.", 100000000, 999999999));
		card_number = new_str(tmp_);

		JMP1:
		card_holder_name = str_input("Enter the card holder???s name: ");
		if (len(card_holder_name) == 0) {
			printf_l("Invalid input.");
			str_free(card_holder_name);
			goto JMP1;
		}
		if (len(card_holder_name) > 50) {
			printf_l("Input name is more than 50 characters.");
			str_free(card_holder_name);
			goto JMP1;
		}
	} else {
		card_number = new_str(L"");
		card_holder_name = new_str(L"");
	}

	time_t unix_time;
	struct tm *time_;
	time(&unix_time);
	time_ = localtime(&unix_time);
	wchar_t local_date[9] = L"";
	wchar_t local_time[9] = L"";
	swprintf(local_date, 9, L"%d/%d/%d", (time_->tm_year + 1900) % 100, time_->tm_mon + 1, time_->tm_mday);
	swprintf(local_time, 9, L"%d/%d/%d", time_->tm_hour, time_->tm_mon + 1, time_->tm_mday);

	column *pay_record_column = new_column();

	col_appends(pay_record_column, 7,
	            TINT_, table_number,
	            TSTRING_, payment_method,
	            NULL, ele_new_str_from_str(card_number),
	            NULL, ele_new_str_from_str(card_holder_name),
	            NULL, amount,
	            TSTRING_, local_date,
	            TSTRING_, local_time);
	sheet *payment_record_sheet = table_get_sheet_by_name(my_table, L"TRANSACTION_HISTORY");
	sheet_append(payment_record_sheet, pay_record_column);
}

void order_food(table *my_table) {
	element *amount;
	sheet *ordered_sheet = p__ordered_item_sheet_create();
	sheet *category_sheet = table_get_sheet_by_name(my_table, L"MENU");
	if (category_sheet->len == 0) {
		printf_l("Empty category list");
		RETURN;
	}
	int table_number = (int) input_integer_question("Enter the table number: ", "Invalid table number.", 1, 100);
	sheet_printf(category_sheet, "%i", "Category no.");

	CATEGORY:
	{};
	int category_number = (int) input_integer_question("Enter the category number: ", "Invalid category number.", 1, category_sheet->len);
	element *category_name = sheet_get_ele_by_index(category_sheet, category_number - 1, 0);
	sheet *food_sheet = table_get_sheet_by_key(my_table, category_name);
	if (food_sheet->len == 0) {
		printf_l("Empty food list.");
		goto CATEGORY;
	}
	sheet_printf(food_sheet, "%i", "Food no.");

	FOOD_MENU:
	{}; // order food
	int food_number = (int) input_integer_question("Enter the food number: ", "Invalid food number.", 1, food_sheet->len);
	int food_availability = (int) sheet_get_ele_by_index(food_sheet, food_number - 1, 2)->value.int_;
	if (food_availability == 0) {
		printf_l("Invalid food number.");
		goto FOOD_MENU;
	}
	if (p__check_is_ordered(ordered_sheet, category_number, food_number)) {
		printf_l("You have already selected the food");
	} else {
		int food_quantity = (int) input_integer_question("Enter the food item???s quantity: ", "Invalid quantity.", 1, food_availability);
		food_availability -= food_quantity;
		// append the ordered items
		p__ordered_item_sheet_append(ordered_sheet, sheet_get_col_by_index(food_sheet, food_number - 1), category_number, food_number, food_quantity);
	}
	if (food_availability && input_yn_question("Do you want to order more food")) {
		goto FOOD_MENU;
	}
	sheet_printf(category_sheet, "%i", "Category no.");
	if (input_yn_question("Do you want to explore more categories")) {
		goto CATEGORY;
	}

	CHECK_OUT:
	{};
	amount = p__ordered_item_print(ordered_sheet);
	if (!input_yn_question("Do you want to check out")) {
		if (input_yn_question("Do you want to add to the selected food list")) {
			sheet_printf(category_sheet, "%i", "Category no.");
			ele_free(amount);
			goto CATEGORY;
		}
		if (input_yn_question("Do you want to remove from the selected food list")) {
			p__ordered_item_sheet_del(my_table, ordered_sheet, NULL);
			ele_free(amount);
			goto CHECK_OUT;
		}
	}

	PAYMENT:
	{};
	p__payment(my_table, table_number, amount); // do not free this amount since it stores in the transition.
	bool is_succeed = flush_file(my_table, "Payment successful.", "Payment unsuccessful.");
	if (is_succeed == false) {
		if (input_integer_question("1. Cancel order 2. Change payment method (1/2)?:", "Invalid input.", 1, 2) == 2) {
			goto PAYMENT;
		} else {
			for (uint32_t i = 0; i < ordered_sheet->len; i++) {
				p__ordered_item_sheet_del(my_table, ordered_sheet, ele_new_str_from_str(str_cpy(
						  sheet_get_ele_by_index(ordered_sheet, i, 0)->value.string_)));
			}
		}
	}
	RETURN_:
	sheet_free(ordered_sheet);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void main_menu(table *my_table) {
	while (1) {
		NL
		printf("1)  Order food\n2)  Admin section\n3)  Exit\n");
		int64_t option = input_integer_question("Option: ", "Unknown option.", 0, 3);
		switch (option) {
			case 1:
				order_food(my_table);
				break;
			case 2:
				admin_section(my_table);
				break;
			default:
				exit(0);
		}
	}
}

void sig_handler(int sig) {
	errno = SIGINT;
	ERROR("Received SIGINT signal, quiting the program...");
}

int main(int argc, char *argv[]) {
	if (argc == 3){
		MENU_FILE_NAME = strcmp(argv[1], "DEFAULT") ? argv[1] : MENU_FILE_NAME;
		TRANSITION_HISTORY_FILE_NAME = strcmp(argv[2], "DEFAULT") ? argv[2] : TRANSITION_HISTORY_FILE_NAME;
	}

	signal(SIGINT, sig_handler);
	setbuf(stdout, 0); // workaround for Clion debugger when running in WSL.
	table *my_table = new_table();
	csv_open(my_table);
	main_menu(my_table);
}
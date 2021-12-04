#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define print if(DEBUG)printf
#define REHASH_LIMIT 0.7
#define DEFAULT_DICT_SIZE 500


bool DEBUG = false;

static uint64_t PRIME = 51539607599;
// this prime is the first prime starting at 1 << 35 + 1 << 34.
// it is computed by using following Python code:
/*

from sympy import sieve
x = sieve.primerange(51539607552)
print(next(x))

 */
// if you want to run this code, it is strongly recommended to use pypy, instead of cpython, as Python interpreter.
// this prime will be used in the hash function as the modular.

typedef struct{
    bool is_used;
    bool is_at_default_location;
    uint32_t hash_value;
    char * ptr_key;
    void * ptr_value;
} dict_chunk;

typedef struct{
    uint32_t size;
    uint32_t content_count;
    uint64_t rand_salt;
    dict_chunk * dict_value;
} dict;

void dict_store(dict *, char *, void *);

uint32_t hash(const char * key, uint32_t seed){
   uint64_t result = 0;
   for (uint32_t i = 0; ; i++){
      if (key[i] == 0){
         break;
      }
      result += (uint64_t) key[i] << 56;
      result %= PRIME;
   }
   return (result % 0xFFFFFFFF) ^ seed;
}


dict * new_dict(size_t size){
   if (! size){
      size = DEFAULT_DICT_SIZE;
   }

   dict * self = malloc(sizeof(dict));
   self->dict_value = calloc(size, sizeof(dict_chunk));
   self->size = size;
   self->rand_salt = random();
   return self;
}

void dict_remap(dict * self, size_t new_size){
   dict_chunk * old_dict = self->dict_value;
   dict_chunk * tmp;
   size_t size = self->size;

   self->dict_value = calloc(sizeof(dict_chunk), new_size);
   self->content_count = 0;
   self->size = new_size;

   for (size_t i = 0; i < size; i++){
      tmp = old_dict + i;
      if (tmp->is_used){
         dict_store(self, tmp->ptr_key, tmp->ptr_value);
      }
   }
   free(old_dict);
}

void dict_store(dict * self, char * key, void * value){
   if (self->content_count > REHASH_LIMIT * self->size){
      dict_remap(self, self->size + self->size / 2);
   }

   uint32_t hash_value = hash(key, self->rand_salt);
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



dict_chunk * _dict_find_trunk(dict * self, char * key) {
   uint32_t hash_value = hash(key, self->rand_salt);
   uint32_t location = hash_value % self->size;
   dict_chunk *self_trunk = self->dict_value + location;
   if (!self_trunk->is_used){
      return NULL;
   }
   if (self_trunk->hash_value == hash_value){
      return self_trunk;
   }

   location += 1;
   for (location = location % self->size ; ; location++){
      self_trunk = self->dict_value + location;
      if (self_trunk->is_at_default_location){
         continue;
      }
      if (self_trunk->is_used){
         if (self_trunk->hash_value == hash_value){
            return self_trunk;
         }
      }
      else{
         return NULL;
      }
   }
}

void * dict_find(dict * self, char * key) {
   dict_chunk * tmp = _dict_find_trunk(self, key);
   if (tmp){
      return tmp->ptr_value;
   }
   return NULL;
}

// return the value and delete the key, return nu
void dict_del(dict * self, char * key) {
   if ((self->content_count < REHASH_LIMIT * self->size / 2) && self->size > DEFAULT_DICT_SIZE) {
      dict_remap(self, self->size / 1.5);
   }

   dict_chunk *self_trunk = _dict_find_trunk(self, key);
   memset(self_trunk, 0, sizeof(dict_chunk));
   self->content_count -= 1;
}


// define a link list

typedef struct node {
    void * value;
    struct node * next_node;
} node;

typedef struct {
    size_t len;
    node * last_node;
    node * first_node;
    size_t last_visit_node_index;
    node * last_visit_node;
} list;


node * list_get_node(list * self, size_t index){
    if (index == 0){
        return self->first_node;
    }
    if (index >= self->len || index < 0){
        return NULL;
    }

    size_t start_index;
    if (index >= self->last_visit_node_index){
        start_index = self->last_visit_node_index;
    }
    else{
        start_index = 0;
        self->last_visit_node =self->first_node;
    }

    for (size_t i = start_index; i < index; i++){
        self->last_visit_node = self->last_visit_node->next_node;
    }
    self->last_visit_node_index = index;
    print("--list_get index: %zu, node: %p\n\n", index, self->last_visit_node);
    return self->last_visit_node;
}

void list_append(list * self, void * newval){
    node * last_node = self->last_node;
    node * new_element = malloc(sizeof(node));

    new_element->next_node = self->first_node;

    last_node->value = newval;
    last_node->next_node = new_element;

    self->last_node = new_element;
    self->len ++;
}

void list_append_int(list * self, int64_t newval){
    union void_s_to_int{
        void * void_s;
        int64_t int_;
    } tmp;
    tmp.int_ = newval;
    list_append(self, tmp.void_s);
}

void list_pop_node(list * self, node * pop_node){
    node * next_node = pop_node->next_node;
    void * tmp_val = pop_node->value;

    pop_node->value = next_node->value;
    next_node->value = tmp_val;

    pop_node->next_node = next_node->next_node;

    if (next_node == self->last_node){
        self->last_node = pop_node;
    }
    self->len -= 1;
    self->last_visit_node = self->first_node;
    self->last_visit_node_index = 0;
    free(next_node);

    print("--list_pop this node: %p, deleted node: %p, next node addr: %p\n\n", pop_node, next_node,
           pop_node->next_node);

}

void list_pop(list * self, size_t index){
    list_pop_node(self, list_get_node(self, index));
}

list * new_list(void){
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

void list_free(list * self){
    node * current_element = self->first_node;
    node * prev_element;
    for (int i = 0; i < self->len; i++){
        prev_element = current_element;
        current_element = current_element->next_node;
        free(prev_element);
    }
    free(self);
}

char * list_to_str(list * self){
    char * my_str = malloc(sizeof(char) * (self->len + 1));
    node * current_element = self->first_node;

    union void_s_to_char{ // I know this convertion is SAFE and PLEASE DO NOT WARN ME.
        void * void_s; // The reasion which I DISLIKE C is it provided useless warning -- the 
        char char_;    // thing which C should do is provide me a block of memory and shut up; I know what i'm doing.
    } tmp;
    
    for (size_t i = 0; i < self->len; i++){
        tmp.void_s = current_element->value;
        my_str[i] = tmp.char_;
        current_element = current_element->next_node;
    }
    my_str[self->len] = '\0'; 
    return my_str;
}

void __attribute__((unused)) list_print_str(list * self){
    // this function is used for debug only.
    if (DEBUG){
        printf("--list len: %zu, first node: %p, last node %p\n", self->len, self->first_node, self->last_node);
        printf("--last visit node index: %zu last visit node: %p\n", self->last_visit_node_index,
               self->last_visit_node);
        node *this_node = self->first_node;
        for (size_t i = 0; i < self->len; i++) {
            printf("node: %zu, ", i);
            printf("addr: %p, ", this_node);
            printf("next addr: %p, ", this_node->next_node);
            printf("value: \"%s\"\n", (char *) this_node->value);
            this_node = this_node->next_node;
        }
        printf("\n");
    }
}

char * get_input(void){
    list * input_list = new_list();
    char * return_val;
    int c;
    while( (c=getchar()) != '\n'){

        list_append_int(input_list, c);
    }
    return_val = list_to_str(input_list);
    list_free(input_list);
    return return_val;
}


// CSV reading:

#define CSV_INT_ 1
#define CSV_FLOAT_ 2
#define CSV_STRING_ 3

typedef struct{
    char var_type;
    union{
        int64_t int_;
        double float_;
        char * string_;
        } value;
} csv_element;


void print_csv_element(csv_element * element_){
    char * name[] = {"number", "decimal", "string"};
    printf("%s: ", name[element_->var_type - 1]);
    if (element_->var_type == CSV_FLOAT_){
        printf("%lf", element_->value.float_);
    }
    else if (element_->var_type == CSV_INT_){
        printf("%li", element_->value.int_);
    }
    else{
        printf("%s", element_->value.string_);
    }
}

list * split_csv_line(char * line, char ** buf_con){
    // input: "hi, 33.3"
    // return: [['h', 'i'], ['3', '3', '.', '3']]

    list * result_list = new_list();
    list * buf = new_list();

    bool is_quote = false;
    bool is_start_element = false;

    size_t i = 0;
    for (; ; i++){

        if (line[i] == '\n' && !is_quote){
            i += 1;
            break;
        }
        if (line[i] == '\0'){
            break;
        }
        if (line[i] == ' ' && !is_start_element){
            continue;
        }
        if (line[i] < 0){
            continue;
        }
        if (line[i] == '"'){
            is_quote = ! is_quote;
            is_start_element = true;
        }
        if (line[i] == ',' && (! is_quote)){
            list_append(result_list, buf);
            buf = new_list();
            is_start_element = false;
        }
        else{
            is_start_element = true;
            list_append_int(buf, line[i]);
        }
    }
    if (buf->len > 0){
        list_append(result_list, buf);
    }
    else{
        free(buf);
    }
    *buf_con = line + i;
    return result_list;
}

csv_element * parse_csv_element(list * element){
    // input: ['3', '3', '.', '3']
    // return csv_element{var_type = CSV_FLOAT_, value = 33.3}

    char * element_str = list_to_str(element);
    char * endptr = NULL;

    csv_element * current_element = malloc(sizeof(csv_element));

    current_element->value.int_ = strtoll(element_str, &endptr, 10);
    if (endptr[0] == '\0'){
        current_element->var_type = CSV_INT_;
    }

    current_element->value.float_ = strtod(element_str, &endptr);
    if (endptr[0] == '\0'){
        current_element->var_type = CSV_FLOAT_;
    }

    else {
        // it is a str
        node *last_node;
        // removing str at the first and the end
        if ((uintptr_t) list_get_node(element, 0)->value == '"' &&
            (uintptr_t) (last_node = list_get_node(element, element->len - 1)) == '"') {
            list_pop(element, 0);
            list_pop_node(element, last_node);
        }
        // parse "" as "
        node *current_node;
        bool is_quote = false;
        int element_char;
        for (size_t i = 0; i < element->len; i++) {
            current_node = list_get_node(element, i);
            element_char = (int) (uint64_t) current_node->value;
            if (element_char == '"') {
                if (is_quote) {
                    list_pop_node(element, current_node);
                    element->last_visit_node = last_node;
                    element->last_visit_node_index = i - 1;
                } else {
                    last_node = current_node;
                }
                is_quote = !is_quote;
            }
            else{
                is_quote = false;
            }
        }
        current_element->value.string_ = list_to_str(element);
        current_element->var_type = CSV_STRING_;
    }
    free(element);
    return current_element;
}

list * parse_csv_line(char * line, char ** buf_con){
    list * list_line = split_csv_line(line, buf_con);
    list * list_result = new_list();
    list * list_element;

    for (int i = 0; i < list_line->len; i++){
        list_element = list_get_node(list_line, i)->value;
        list_append(list_result, parse_csv_element(list_element));
    }
    return list_result;
}

char * _open_csv(char * file_name){
    struct stat stat_;
    int file_handle = open(file_name, O_RDWR);
    if (file_handle == 0){
        printf("csv load failed, program terminated.");
        exit(1);
    }
    if (stat(file_name, &stat_) != 0){
        printf("can't determine file's size. Program terminated.");
        exit(1);
    }
    char * file_ptr = mmap(NULL,stat_.st_size,
                        PROT_READ|PROT_WRITE,MAP_SHARED,
                        file_handle ,0);
    return file_ptr;
}

list * load_csv(char * file_name){
    char * csv_file = _open_csv(file_name);
    char * buf_con = csv_file;
    list * csv_list = new_list();

    while (true){
        list_append(csv_list, parse_csv_line(buf_con, &buf_con));
        if(buf_con[0] == '\0'){
            break;
        }
    }
    return csv_list;
}

int main(void){
    list * my_list;
    my_list = load_csv("Book1.csv");

    int element[2] = {1, 2};
    print_csv_element(list_get_node(list_get_node(my_list, element[0])->value, element[1])->value);

//    list * my_list = parse_csv_line("\"hell,o\",hi,12234455,\"33.3\"", 36);
//
//    list * x = new_list();
//
//    list_append(x, "hello");
//    list_append(x, "hi");
//    list_append(x, "world");
//    list_append(x, "world2");
//    list_append(x, "world3");
//    list_append(x, "world4");
//    list_append(x, "world5");
//    list_append(x, "world6");
//    list_append(x, "world7");

//    list_print_str(x);
//    list_pop(x, x->len - 1);
//    list_pop(x, 3);
//    list_print_str(x);

}

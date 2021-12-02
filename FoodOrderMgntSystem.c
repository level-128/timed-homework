#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <sys/mman.h>

#define print if(DEBUG)printf

bool DEBUG = false;

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

void list_print_str(list * self){
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


list * split_csv_line(char * line, size_t len){
    // input: "hi, 33.3"
    // return: [['h', 'i'], ['3', '3', '.', '3']]

    list * result_list = new_list();
    list * buf = new_list();

    bool is_str = false;

    for (size_t i = 0; i < len ; i++){
        if (line[i] == '\n'){
            continue;
        }
        if (line[i] == ' ' && !is_str){
            continue;
        }
        if (line[i] == '"'){
            is_str = ! is_str;
        }
        if (line[i] == ',' && (! is_str)){
            list_append(result_list, buf);
            buf = new_list();
        }
        else{
            list_append_int(buf, line[i]);
        }
    }
    if (buf->len > 0){
        list_append(result_list, buf);
    }
    else{
        free(buf);
    }
    return result_list;
}

csv_element * parse_csv_element(list * element){
    // input: ['3', '3', '.', '3']
    // return csv_element{var_type = CSV_FLOAT_, value = 33.3}

    char * element_str = list_to_str(element);
    char * endptr;

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
        }
        current_element->value.string_ = list_to_str(element);
        current_element->var_type = CSV_STRING_;
    }
    free(element);
    return current_element;
}

list * parse_csv_line(char * line, size_t size){
    list * list_line = split_csv_line(line, size);
    list * list_result = new_list();
    list * list_element;
    for (int i = 0; i < list_line->len; i++){
        list_element = list_get_node(list_line, i)->value;
        list_append(list_result, parse_csv_element(list_element));
    }
    return list_result;
}

list * open_csv(char * file){

}


int main(void){
    open_csv("test.csv");

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
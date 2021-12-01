#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define PRINT_LOG true

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


node * list_get_node(list * self, int64_t index){
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
    printf("--list_get index: %li, node: %p\n\n", index, self->last_visit_node);
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

void list_pop(list * self, size_t index){
    node * this_node = list_get_node(self, index);
    node * next_node = this_node->next_node;

    void * tmp_val = this_node->value;
    this_node->value = next_node->value;
    next_node->value = tmp_val;

    this_node->next_node = next_node->next_node;
    free(next_node);
    
    if (index == self->len - 1){
        self->last_node = this_node;
    }

    printf("--list_pop this node: %p, deleted node: %p, next node addr: %p\n\n", this_node, next_node, this_node->next_node);
    self->len -= 1;
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
    printf("--list len: %li, first node: %p, last node %p\n", self->len, self->first_node, self->last_node);
    printf("--last visit node index: %li last visit node: %p\n", self->last_visit_node_index, self->last_visit_node);
    node * this_node = self->first_node;
    for (size_t i = 0; i <= self->len; i++){
        printf("node: %li, ", i);
        printf("addr: %p, ", this_node);
        printf("next addr: %p, ", this_node->next_node);
        printf("value: \"%s\"\n", (char *)this_node->value);
        this_node = this_node->next_node;
    }
    printf("\n");
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
    list * name;
    size_t length;
    void * table_content;
} csv_table;

typedef struct{
    char var_type;
    union{
        int64_t int_;
        double float_;
        char * string_;
        } value;
} csv_element;

void not_use(void){}


list * split_csv_line(char * line, size_t len){
    list * result_list = new_list();
    list * buf = new_list();

    for (size_t i = 0; i < len ; i++){
        if (line[i] == '\n'){
            continue;
        }  
        if (line[i] == ','){
            list_append(result_list, list_to_str(buf)); // TODO: place buf directly into the linked list
            list_free(buf);
            buf = new_list();
        }
        else{
            list_append_int(buf, line[i]);
        }
    }
    list_append(result_list, list_to_str(buf));
    list_free(buf);
    return result_list;
}

list * parse_csv_line(list * line){
    node * current_node;
    char * str_value;
    for (size_t i = 0; i < line->len; i++){
        current_node = list_get_node(line, i);
        str_value = current_node->value;
    }
}

int main(void){
    // list * my_list = split_csv_line("hello,hi,12234455,\"33.3\"", 36);
    // for (int i = 0; i < my_list->len; i++){
    //     printf("%s ", list_get_node(my_list, i)->value);
    // }

    list * x = new_list();

    list_append(x, "hello");
    list_append(x, "hi");
    list_append(x, "world");
    list_append(x, "world2");
    list_append(x, "world3");
    list_append(x, "world4");
    list_append(x, "world5");
    list_append(x, "world6");
    list_append(x, "world7");

    list_print_str(x);
    list_pop(x, x->len - 1);
    list_pop(x, 3);
    list_print_str(x);

}
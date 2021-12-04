#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define REHASH_THERESHOLD 0.7

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


#define DEFAULT_DICT_SIZE 500

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
   if (self->content_count > REHASH_THERESHOLD * self->size){
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
   if ((self->content_count < REHASH_THERESHOLD * self->size / 2) && self->size > DEFAULT_DICT_SIZE) {
      dict_remap(self, self->size / 1.5);
   }

   dict_chunk *self_trunk = _dict_find_trunk(self, key);
   memset(self_trunk, 0, sizeof(dict_chunk));
   self->content_count -= 1;
}


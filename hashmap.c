#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <stdint.h>
#include <string.h>

#define REHASH_THERESHOLD 0.7

char* itoa(int num)
{
   char * str = malloc(3);
   int radix = 10;
    char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";//索引表
    unsigned unum;//存放要转换的整数的绝对值,转换的整数可能是负数
    int i=0,j,k;//i用来指示设置字符串相应位，转换之后i其实就是字符串的长度；转换后顺序是逆序的，有正负的情况，k用来指示调整顺序的开始位置;j用来指示调整顺序时的交换。
 
    //获取要转换的整数的绝对值
    if(radix==10&&num<0)//要转换成十进制数并且是负数
    {
        unum=(unsigned)-num;//将num的绝对值赋给unum
        str[i++]='-';//在字符串最前面设置为'-'号，并且索引加1
    }
    else unum=(unsigned)num;//若是num为正，直接赋值给unum
 
    //转换部分，注意转换后是逆序的
    do
    {
        str[i++]=index[unum%(unsigned)radix];//取unum的最后一位，并设置为str对应位，指示索引加1
        unum/=radix;//unum去掉最后一位
 
    }while(unum);//直至unum为0退出循环
 
    str[i]='\0';//在字符串最后添加'\0'字符，c语言字符串以'\0'结束。
 
    //将顺序调整过来
    if(str[0]=='-') k=1;//如果是负数，符号不用调整，从符号后面开始调整
    else k=0;//不是负数，全部都要调整
 
    char temp;//临时变量，交换两个值时用到
    for(j=k;j<=(i-1)/2;j++)//头尾一一对称交换，i其实就是字符串的长度，索引最大值比长度少1
    {
        temp=str[j];//头部赋值给临时变量
        str[j]=str[i-1+k-j];//尾部赋值给头部
        str[i-1+k-j]=temp;//将临时变量的值(其实就是之前的头部值)赋给尾部
    }
 
    return str;//返回转换后的字符串
 
}


#define DEFAULT_DICT_SIZE 500

typedef struct{
   char is_used;
   char key[3];
   uint32_t hash_value;
   void * ptr_key;
   void * ptr_value;
} dict_chunk;

typedef struct{
    size_t size;
    size_t content_count;
    uint64_t rand_salt;
    dict_chunk * dict_value;
} dict;

void dict_store(dict * self, char * key, void * value);

// Using MurmurHash hash functions, this is internet resource,
// because of course I can't implement a hash function by my self,
// otherwise I should quit this school and become a PhD in number theory.
// the reason which I use MurmurHash is linear modular based hash
// functions could not pass chi-square tests, and even a random
// seed were given, it still faces hash collision DOS under malicious input.
//
// source: https://en.wikipedia.org/wiki/MurmurHash
static inline uint32_t murmur_32_scramble(uint32_t k) {
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    return k;
}
uint32_t hash(const char* key, uint32_t seed)
{
  uint32_t h = seed;
  uint32_t k;
  uint32_t len = strlen(key);
  /* Read in groups of 4. */
  for (size_t i = len >> 2; i; i--) {
    // Here is a source of differing results across endianness.
    // A swap here has no effects on hash properties though.
    memcpy(&k, key, sizeof(uint32_t));
    key += sizeof(uint32_t);
    h ^= murmur_32_scramble(k);
    h = (h << 13) | (h >> 19);
    h = h * 5 + 0xe6546b64;
  }
  /* Read the rest. */
  k = 0;
  for (size_t i = len & 3; i; i--) {
    k <<= 8;
    k |= key[i - 1];
  }
  // A swap is *not* necessary here because the preceding loop already
  // places the low bytes in the low places according to whatever endianness
  // we use. Swaps only apply when the memory is copied in a chunk.
  h ^= murmur_32_scramble(k);
  /* Finalize. */
  h ^= len;
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

char is_right_key(dict_chunk * self_chunk, char * key, dict * self){
//   if (! memcmp(self_chunk->key, key, 3)){ // TODO
      if (self_chunk->hash_value == hash(key, self->rand_salt)){
         return 1;
      }
//   }
   return 0;
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
        tmp = (dict_chunk *)((int64_t)old_dict + sizeof(dict_chunk) * i);
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
    size_t location = hash_value % self->size;
    dict_chunk * self_trunk;

   for (location = location % self->size ; ; location++){
      self_trunk = (dict_chunk *)((int64_t)self->dict_value + sizeof(dict_chunk) * location);
      if (! self_trunk->is_used){
         break;
      }
      if (is_right_key(self_trunk, key, self)){
         break;
      }
   }

   self_trunk->is_used = 1;
   memcpy(&self_trunk->key, key, 3);
   self_trunk->hash_value = hash_value;
   self_trunk->ptr_key = key;
   self_trunk->ptr_value = value;

   self->content_count += 1;
}

void * dict_find(dict * self, char * key){
  uint32_t hash_value = hash(key, self->rand_salt);
  size_t location = hash_value % self->size;
  dict_chunk * self_trunk;

  for (location = location % self->size ; ; location++){
     self_trunk = (dict_chunk *)((int64_t)self->dict_value + sizeof(dict_chunk) * location);
     if (self_trunk->is_used){
        if (is_right_key(self_trunk, key, self)){
           break;
        }
     }
     else{
       return NULL;
     }

  }
  return self_trunk->ptr_value;
}





int main(void){
   dict * mydict = new_dict(0);

   for (int i = 0; i < 400; i++){
      dict_store(mydict, itoa(i), itoa(i));
      printf("store: %i\n", i);
   }
   for (int i = 0; i < 400; i++){
      printf("%s\n", (char *)dict_find(mydict, itoa(i)));
   }

}
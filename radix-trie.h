#ifndef RADIX_TRIE_H
#define RADIX_TRIE_H


#include <stdint.h>






typedef struct node nod;




extern nod* radix_trie_insert(nod *r, uint32_t key, int length, void *value);

extern void radix_trie_walk(nod *root, void (*fn)(uint32_t key, int bit, void *v));


extern int radix_trie_find(nod *root, uint32_t key, int len, void **val);

extern int radix_trie_delete(nod *n, uint32_t key, int len);

extern void radix_trie_delete_all(nod *root);



// Helpers



static void
trie_node_print(uint32_t key, int bit, void *v)
{
    int i = 0;

    printf("key = 0x");
    for (i = 4; i <= bit; i += 4)
    {
        printf("%01X", (key >> (32 - i)) & 0xf);
    }
    printf(", value = %p\n", v);

}


#endif

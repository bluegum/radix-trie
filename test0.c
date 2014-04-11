#include <stdio.h>
#include <stdlib.h>
#include "radix-trie.h"


struct pair
{
    uint32_t k;
    int  len;
    int  v;
};


static struct pair mymap[]=
{
    {0x10ff0000, 32, 0x01},
    {0x10ff0001, 32, 0x02},
    {0x10ff2000, 32, 0x20},
    {0x10ff0002, 32, 0x90},
    {0x10ffe002, 32, 0x901},
    {0x10ffd002, 32, 0x9001},
    {0x00, 8, 0x20},
    {0x01, 8, 0x21},
    {0x71, 8, 0x30},
    {0x72, 8, 0x31},
    {0x7171, 16, 0x71},
    {0x7172, 16, 0x72},
    {0x828282, 24, 0x82},
};

int
main(int argc, char **argv)
{

    nod *trie = 0;
    void *val;
    uint32_t key;

    int i;

    for (i = 0; i < sizeof(mymap)/sizeof(struct pair); i++)
    {
        printf("key = %08X\n", mymap[i].k);
        trie = radix_trie_insert(trie, mymap[i].k, mymap[i].len, (void*)mymap[i].v);

    }


    printf("%s", "\n\n\nWalking\n\n");
    radix_trie_walk(trie, trie_node_print);

    printf("%s", "\n\n\nLooking-up...exiting entries\n\n");

    for (i = 0; i < sizeof(mymap)/sizeof(struct pair); i++)
    {
        if (radix_trie_find(trie, mymap[i].k, mymap[i].len, &val))
        {
            printf("%08X Found, of Value = 0x%x\n", mymap[i].k, (int)val);
        }
        else
        {
            printf("%08X Not Found\n", mymap[i].k);
        }

    }

    printf("%s", "\n\n\nLooking-up...non-exit entries\n\n");
    key = 0x10ff2002;
    if (radix_trie_find(trie, key, 32, &val))
    {
        printf("%08X Found, of Value = %x\n", key, (int)val);
    }
    else
    {
        printf("%08X Not Found\n", key);
    }

    // replace an entry

    key = 0x10ff0002;
    printf("\n\n\nREplace value of %08X with %p\n", key, 0xffffffff);
    trie = radix_trie_insert(trie, key, 32, (void*)0xffffffff);
    key = 0x71;
    printf("\n\n\nREplace value of %08X with %p\n", key, 0xff);
    trie = radix_trie_insert(trie, key, 8, (void*)0xff);

    printf("%s", "\n\n\nWalking\n\n");
    radix_trie_walk(trie, trie_node_print);


    // delete an entry

    key = 0x10ff0002;
    printf ("\n\n\nDeleting entry, key = %08X\n\n\n", key);
    if (radix_trie_delete(trie, key, 32))
    {
        printf ("%08X entry is deleted\n", key);
    }
    else
    {
        printf("%08X Not Found\n", key);
    }

    

    // check this entry
    if (radix_trie_find(trie, key, 32, &val))
    {
        printf("%08X Found, of Value = %x\n", key, (int)val);
    }
    else
    {
        printf("%08X Not Found\n", key);
    }

    // delete all entries
    printf ("\n\n\n\n");
    for (i = 0; i < sizeof(mymap)/sizeof(struct pair); i++)
    {
        if (radix_trie_delete(trie, mymap[i].k, mymap[i].len))
        {
            printf("key = %08X is deleted\n", mymap[i].k);
        }
        else
        {
            printf("key = %08X is not found\n", mymap[i].k);
        }
    }



    // delete the whole tree
    radix_trie_delete_all(trie);

    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include "radix-trie.h"

int
main(int argc, char **argv)
{

    nod *trie = 0;
    void *val;
    uint key;

    int i, size = 1024;

    for (i = 0; i < size; i++)
    {
        trie = radix_trie_insert(trie, i, 32, (void*)i);
    }


    printf("%s", "\n\n\nWalking\n\n");
    radix_trie_walk(trie, trie_node_print);


    // delete all entries
    printf ("\nDeleting\n\n\n\n");
    for (i = 0; i < size; i++)
    {
        if (radix_trie_delete(trie, i, 32))
        {
        }
        else
        {
            printf("key = %08X is not found\n", i);
        }
    }



    // delete the whole tree
    radix_trie_delete_all(trie);

    return 0;
}


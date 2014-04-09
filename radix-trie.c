#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "radix-trie.h"


/*
  This implementation is inspired by crit_bit tree paper by D. J. Bernstein, and the implementation of radix-tree.c in Linux kernel lib.

  Instead of using the ubiquitous implementation of binary patricia tree, this implementation
  uses a pre-configured fan-out factor, maxing out at 32, can be
  configured as 2, 4, 8, 16, and 32, installed in each internal node
  as well as external node. There are two bitmapped "long" type integers combined used to tag each of the fan'd slot,
  signal the reference type of each slot, containing internal-node
  only, external node only, or internal-node plus external node, or empty.
  The external type is a void type reference points to user data.

  The wide fan-out patricia tree should have less memory overhead when
  most of the keys are clustered, or more densely pouplated.

  The choosing of maximum of 32 slot is keep the implementation
  simple, otherwise, the tag bitmap need to be in two array type instead
  of two 32 bit integer, like those in linux-kernel.

  Note: the integertype uint32_t is used in the structure could be
  able to be replaced by  uint64_t for mapping key with length upto 64
  bit.

  Note: I choose radix_trie instead of radix_tree is to avoid
  confusion with the version in linux kernel.
 */

#define RADIX_ORDER 4

#if (RADIX_ORDER % 2)
  #define BIT_CHUNK 1
#else
  #define BIT_CHUNK RADIX_ORDER
#endif

#define MAP_SIZE (1<<RADIX_ORDER)

typedef enum
{
    n_empty,
    n_external,
    n_internal,
    n_composite
} nodetype;

#if BIT_CHUNK > 5
#error Maximum fan factor is 32
#endif





/* tag and tag1 bit */
/*
  tag      tag1
   0        0      empty
   1        0      external
   0        1      internal
   1        1      internal + external
 */


/* node are internal ONLY */
struct node
{
    uint32_t key;
    uint32_t tag; /* bitfield, 0 for internal node, 1 for external data */
    uint32_t tag1; /* bitfield, 0 is undefined, 1 for internal + external(value) */
    int  crit_bit;
    int  order;    /* Normally order == RADIX_ORDER, when 32 % RADIX_ORDER != 0, it can be less than RADIX_ORDER */
    void *value;
    struct node *fan[MAP_SIZE];
};


/*
 * key: are 32 bit integer
 * crit_bit: the last bit of common prefix
 * order: the exponent over 2 on span
 */
static inline
int
radix_trie_find_slot(uint32_t key, int order, int crit_bit)
{
    int len = 32;

    uint32_t n_crit_bit = crit_bit + order;

    uint32_t n = 0;

    uint32_t msk = ~0UL >> (32 - order);

    n =  ~0UL >> (32 - n_crit_bit);

    n &= key >> (32 - n_crit_bit);

    n &= msk;

    return (int)n;
}



static inline
void
radix_trie_set_tag(nod *node, int offset)
{

    node->tag |= (1<<offset);

}

static inline
int
radix_trie_get_tag(nod *n, int offset)
{
    return (n->tag & (1<<offset));
}


static inline
void
radix_trie_clear_tag(nod *node, int offset)
{

    node->tag &= (~(1<<offset));

}

static inline
void
radix_trie_clear_tag1(nod *node, int offset)
{

    node->tag1 &= (~(1<<offset));

}

static inline
void
radix_trie_set_tag1(nod *node, int offset)
{

    node->tag1 |= (1<<offset);

}

static inline
int
radix_trie_get_tag1(nod *n, int offset)
{
    return (n->tag1 & (1<<offset));
}

static inline
nodetype
radix_trie_get_nodetype(nod *n, int offset)
{
    int a, b;

    a = radix_trie_get_tag(n, offset);
    b = radix_trie_get_tag1(n, offset);

    if (a == 0 && b == 0)
    {
        return n_empty;
    }
    else if (a != 0 && b == 0)
    {
        return n_external;
    }
    else if (a == 0 && b != 0)
    {
        return n_internal;
    }
    else /* a != 0 && b != 0*/
    {
        return n_composite;
    }
    return n_empty;
}

static inline
void
radix_trie_set_nodetype(nod *n, nodetype nt, int offset)
{

    if (!n)
        return;

    radix_trie_clear_tag(n, offset);
    radix_trie_clear_tag1(n, offset);

    switch (nt)
    {
        case n_internal:
            radix_trie_set_tag1(n, offset);
            break;

        case n_external:
            radix_trie_set_tag(n, offset);
            break;

        case n_composite:
            radix_trie_set_tag(n, offset);
            radix_trie_set_tag1(n, offset);
            break;

        default:
            break;
    }
}


static
int
radix_trie_find_prefix(uint32_t k0, uint32_t k1)
{
    uint32_t x = k0 ^ k1;
    uint32_t msk = ~(~0UL >> 1);

    int i = 0;

    if (x == 0)
        return 32;
    while ( (x & msk) == 0)
    {
        i++;
        msk >>= 1;

    }
    return i;
}



static
nod*
radix_trie_new(uint32_t key, int len, int prefix, void *value, int external)
{
    nod *n;
    int i;
    uint32_t _key;
    int crit_bit;
    int first_order;

    if (len < 32)
    {
        _key = key << (32 - len);
    }
    else
    {
        _key = key;
    }


    n = (nod*)malloc(sizeof(nod));
    memset(n, 0, sizeof(nod));
    n->key = key;
    n->value = value;

    if (external)
    {
        crit_bit = prefix - RADIX_ORDER;
    }
    else
    {
        crit_bit = 32 - prefix;
        crit_bit = (crit_bit + RADIX_ORDER - 1) / RADIX_ORDER * RADIX_ORDER;
        crit_bit = 32 - crit_bit;
    }

    first_order = 32 % RADIX_ORDER;
    if (prefix < first_order)
    {
        n->crit_bit = 0;
    }
    else
    {
        n->crit_bit = crit_bit;
    }

    if (n->crit_bit == 0)
    {
        n->order = 32 % RADIX_ORDER;
        if (n->order == 0)
            n->order = RADIX_ORDER;

        if (n->order < prefix)
        {
            // must ajust crit_bit
            n->crit_bit = crit_bit;
            n->order = RADIX_ORDER;

        }
    }
    else
    {
        n->order = RADIX_ORDER;
    }

    i = radix_trie_find_slot(_key, n->order, n->crit_bit);

    if (external)
    {
        radix_trie_set_nodetype(n, n_external, i);
    }
    n->fan[i] = value;


    return n;
}

nod*
radix_trie_insert(nod *r, uint32_t key, int length, void *value)
{
    int  match = 0;
    int  len = length;
    nod *_r = r;
    nod *last_r;
    int  i, prefix;
    uint32_t _key;

    if (length < 32)
    {
        _key = key << (32 - length);
    }
    else
    {
        _key = key;
    }

    if (!r)
    {
        /* length of key is crit_bit */
        r = radix_trie_new(_key, len, length, value, 1);
        return r;
    }

    last_r = r;

    /* trace prefix, at BIT_CHUNK a time */

    while (len)
    {
        int crit_bit = r->crit_bit;

        prefix = radix_trie_find_prefix(_key, r->key);

        if (prefix >= r->crit_bit)
        {
            // iterate down
            int i = radix_trie_find_slot(_key, r->order, r->crit_bit);
            nodetype nt = radix_trie_get_nodetype(r, i);
            if (nt == n_empty || nt == n_external)
            {
                match = r->crit_bit;
                break;
            }
            else if (nt == n_composite)
            {
                if (length == r->crit_bit + r->order)
                {
                    match = r->crit_bit;
                    break;
                }
                else
                {
                    last_r = r;
                    r = r->fan[i];
                }
            }
            else
            {
                last_r = r;
                r = r->fan[i];
            }
        }
        else
        {
            match = 32 - prefix;
            match = (match + RADIX_ORDER - 1) / RADIX_ORDER * RADIX_ORDER;
            match = 32 - match;

            if (match <= 0)
                match = 0;

            break;
        }
    }


    if (match == r->crit_bit)
    {

        int i = radix_trie_find_slot(_key, r->order, r->crit_bit);
        nodetype nt = radix_trie_get_nodetype(r, i);


        if (nt == n_empty)
        {
            if (match + RADIX_ORDER >= length)
            {
                // simply insert value and set tag bit
                r->fan[i] = value;
                radix_trie_set_nodetype(r, n_external, i);
            }
            else
            {
                // insert new node
                nod *n_child;
                n_child = radix_trie_new(_key, 32, length, value, 1); /* crit_bit is the last bit */
                r->fan[i] = n_child;
                radix_trie_set_nodetype(r, n_internal, i);
            }
            return _r;
        }
        else if (nt == n_external)
        {
            // Take a leaf and read ...
            if (r->crit_bit + r->order == length)
            {
                // The reasoning is:
                // we matched the whole key, note: (match == r->crit_bit),
                // and the existing holder being external, means the keys must match
                // So, let's Replace the exiting value
                r->fan[i] = value;
            }
            else if (r->crit_bit + r->order > length)
            {
                /* r->crit_bit + r->order should always less than 32 */
                r->fan[i] = value;
            }
            else
            {
                /* The slot is already taken by an external entry, the following steps to be done */
                /* 1. we are going to make a new internal node to fit the current entry */
                /* 2. and assigned the "value" field in the new node with existing value */
                /* 3. clear bit in tag */
                /* 4. set bit in tag1 to signal the new entry are compound entry */
                /* 5. insert the new node to the slot */

                nod *n_child =
                    radix_trie_new(_key, 32, length, value, 1); /* crit_bit is the last bit, and its an external */

                n_child->value = r->fan[i];
                radix_trie_set_nodetype(r, n_composite, i);
                r->fan[i] = n_child;
            }
        }
        else if (nt == n_composite)
        {
            // replace
            r->fan[i]->value = value;
            return _r;
        }
        else if (nt == n_internal)
        {
            r->fan[i]->value = value;
            radix_trie_set_nodetype(r, n_composite, i);
            return _r;
        }
    }
    else if (match < r->crit_bit)
    {
        // a new node is needed with shorter crit_bit
        // with "match" bits of common prefix
        // with new child and old_child, as is "r"


        /* split r */
        int i = radix_trie_find_slot(_key, r->order, match);
        uint32_t n_mask = 0xffffffff >> match;
        nod *n_nod, *n_child;
        int slot_old;

        /* new child node for new key 
         * crit_bit is set to the key_length subtracted by fanning factor, 
         * to INDICATE this node contains only externals, and it will stay this way.
         */
        n_child = radix_trie_new(_key, 32, length, value, 1);

        /* new root for sub-tree */


        n_nod = radix_trie_new(_key, 32, prefix, r->value, 0);
        /* connect the old root to the 1st slot */
        slot_old = radix_trie_find_slot(r->key, n_nod->order, n_nod->crit_bit);
        n_nod->fan[slot_old] = r;

        /* connect the new child to new root */
        i = radix_trie_find_slot(_key, n_nod->order, match);
        n_nod->fan[i] = n_child;

        /* set tags */
        radix_trie_set_nodetype(n_nod, n_internal, slot_old);
        radix_trie_set_nodetype(n_nod, n_internal, i);

        if (r == _r)
        {
            return n_nod;
        }
        else
        {
            i = radix_trie_find_slot(_key, last_r->order, last_r->crit_bit);
            last_r->fan[i] = n_nod;
            return _r;
        }

    }
    else if (match > r->crit_bit)
    {
        printf("Something weird happened\n");
    }

    return _r;
}

void
radix_trie_walk(nod *root, void (*fn)(uint32_t key, int bit, void *v))
{
    int i;
    uint32_t k;


    if (!root)
        return;
    for (i = 0; i < MAP_SIZE; i++)
    {

        nodetype nt = radix_trie_get_nodetype(root, i);

        k = (root->key) & (0xffffffff << (32 - root->crit_bit));
        k += i << (32 - root->crit_bit - root->order);

        if (nt == n_external)
        {
            fn(k, root->crit_bit + root->order, root->fan[i]);
        }
        else if (nt == n_internal)
        {
            radix_trie_walk(root->fan[i], fn);
        }
        else if (nt == n_composite)
        {
            fn(k, root->crit_bit + root->order, root->fan[i]->value);
            radix_trie_walk(root->fan[i], fn);
        }
    }

}


/*
 * return:
 *  0 for not found
 *  1 for found, value stored in val
 */
int
radix_trie_find(nod *root, uint32_t key, int len, void **val)
{

    int i, j;
    uint32_t k;
    nodetype nt;
    int length = len;

    if (!root)
        return 0;

    if (len < 32)
        k = key << (32 - len);
    else
        k = key;

    j = 28;
    if (root->crit_bit == 0)
    {
    
        i = k >> (32 - BIT_CHUNK);

        nt = radix_trie_get_nodetype(root, i);
        if (nt == n_internal)
        {
            // slot is external data
            if (len == BIT_CHUNK)
            {
                // found
                *val = root->fan[i];
                return 1;
            }
            // fall through
        }
        else if (nt == n_composite)
        {
            if (len == BIT_CHUNK)
            {
                // found
                *val = root->fan[i]->value;
                return 1;
            }
            // fall through
        }
        else if (nt == n_external)
        {
            if (len == BIT_CHUNK)
            {
                // found
                *val = root->fan[i];
                return 1;
            }
            // fall through
        }
        else
        {
            return 0;
        }
    }

    if (!root)
        return 0;

    while (len > root->crit_bit)
    {
        int prefix = radix_trie_find_prefix(k, root->key);

        i = (k >> (32 - root->crit_bit - BIT_CHUNK)) & (0xff >> (8 - BIT_CHUNK));
        nt = radix_trie_get_nodetype(root, i);

        if (prefix > root->crit_bit + root->order)
        {
            switch (nt)
            {
                case n_internal:
                case n_composite:
                    root = root->fan[i];
                    break;
                case n_external:
                    return 0;
                    break;
                default:
                    return 0;
                    break;
            }
        }
        else
        {
            if (length == root->crit_bit + root->order)
            {
                switch (nt)
                {
                    case n_composite:
                        *val = root->fan[i]->value;
                        return 1;
                        break;
                    case n_external:
                        *val = root->fan[i];
                        return 1;
                        break;
                    case n_internal:
                    default:
                        return 0;
                        break;
                }
                
            }
            else
            {
                switch (nt)
                {
                    case n_internal:
                    case n_composite:
                        root = root->fan[i];
                        break;
                    case n_external:
                        return 0;
                        break;
                    default:
                        return 0;
                        break;
                }
            }
        }

        len -= BIT_CHUNK;
        j -= BIT_CHUNK;
    }

    if (len + BIT_CHUNK == root->crit_bit)
    {
        i = (k >> (32 - root->crit_bit - BIT_CHUNK)) & (0xff >> (8 - BIT_CHUNK));
        if (radix_trie_get_nodetype(root, i) == n_external)
        {
            *val = root->fan[i];
            return 1;
        }

    }

    return 0;

}

static
int
radix_trie_is_empty(nod* n)
{
    return (n->tag == 0 &&
            n->tag1 == 0);
}
/*
 * radix_trie_delete:
 *  Given a key, remove the entry or node associated with key
 *  within the tree if the key is found, otherwise, do nothing
 *
 * Return:
 *  0 : fail
 *  1 : success
 */
int
radix_trie_delete(nod *n, uint32_t key, int len)
{
    int  r = 0;
    int  i;
    uint32_t k;

    if (!n)
        return r;

    if (len < 32)
        k = key << (32 - len);
    else
        k = key;


    // search for key entry
    if (n->crit_bit < len)
    {
        nodetype nt;

        // need to look up the node using partial key
        i = radix_trie_find_slot(k, n->order, n->crit_bit);
        nt = radix_trie_get_nodetype(n, i);
        switch (nt)
        {
            case n_internal:
                r = radix_trie_delete(n->fan[i], key, len);
                if (radix_trie_is_empty(n->fan[i]))
                {
                    free(n->fan[i]);
                    radix_trie_set_nodetype(n, n_empty, i);
                    // for debuging
                    n->fan[i] = 0;
                }
                return r;
            case n_composite:
                if (n->crit_bit + n->order == len)
                {
                    radix_trie_set_nodetype(n, n_internal, i);
                    r = 1;
                }
                else
                {
                    r = radix_trie_delete(n->fan[i], key, len);


                    if (radix_trie_is_empty(n->fan[i]))
                    {
                        void *x = n->fan[i]->value;
                        free(n->fan[i]);
                        radix_trie_set_nodetype(n, n_external, i);
                        n->fan[i] = x;
                    }
                }
                break;
            case n_external:
                // varify key is same
                // they should be the same
                // ..
                // unset tag
                radix_trie_set_nodetype(n, n_empty, i);
                r = 1;
                break;
            default:
                printf("%08X is not in the set\n", key);
                break;
        }
    }
    else if (n->crit_bit == len)
    {
        // just look up in the node
        printf("%08X should be in this node\n", k);
    }
    else
    {
        printf ("error in %s: crit_bit = %d > len = %d\n", __FUNCTION__, n->crit_bit, len);
        return 0;
    }
    return r;
}

void
radix_trie_delete_all(nod *root)
{

    int i, n;

    if (!root)
        return;

    n = 1 << BIT_CHUNK;
    for (i = 0; i < n; i++)
    {

        nodetype nt = radix_trie_get_nodetype (root, i);

        switch (nt)
        {

            case n_internal:
                radix_trie_delete_all(root->fan[i]);
                break;

            case n_composite:
                radix_trie_delete_all(root->fan[i]);
                break;

            default:
                break;
        }
    }
    free(root);
}


/*
 * radix_trie_destroy:
 *
 *  Use user provided destructor to delete all leaf objects
 *  Then, the tree is free'd
 *
 */
void
radix_trie_destroy(nod *r, void (*fn)(uint32_t key, int bit, void *v))
{

    radix_trie_walk(r, fn);

    radix_trie_delete_all(r);

}
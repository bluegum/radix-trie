#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "pat.h"


/*
  This implementation is inspired by implementation of radix-tree.c in Linux kernel lib.

  Instead of using the ubiquitous implementation of binary patricia tree, this implementation
  uses a pre-configured fan-out factor, maxing out at 32, installed in each internal node
  as well as external node. There is a bitmap'd "long" used to tag each of the fan'd slot,
  signal the reference containing internal-node only, or internal-node plus external node.
  Here the external node is a void type reference points to user data.

  The wide fan-out patricia tree should have less memory overhead when there are large concentration
  patterns in the tails of fixed length key.

  This implementation limits the key length to 4 bytes maximum. The rational to this constraint is that
  simplifies implementation of 32 bit architecture with 32 bit key. Another factor is this data structure is
  aimed to be used by CMap charactor mapping, where assuming the character keys are all bound
  by unicode code-space (0 .. 0x10FFFF).

 */

#define MASK_CHUNK 4
#define FAN_FACTOR (1<<MASK_CHUNK)
#define MASK_PREFIX (0xFFFFFFFF<<(32-MASK_CHUNK))

typedef enum
{
    n_empty,
    n_external,
    n_internal,
    n_composite
} nodetype;

#if MASK_CHUNK > 4
#error Maximum fan factor is 16
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
    uint key;
    uint tag; /* bitfield, 0 for internal node, 1 for external data */
    uint tag1; /* bitfield, 0 is undefined, 1 for internal + external(value) */
    uint mask;
    int  crit_bit;


    void *value;
    struct node *fan[FAN_FACTOR];
};



static inline
int
pat_find_slot(uint key, int len, int crit_bit)
{
    uint n_crit_bit = crit_bit/MASK_CHUNK*MASK_CHUNK;

    uint n = 0;

    uint prefix_mask = 0x0f;

    int shift = len / MASK_CHUNK * MASK_CHUNK;
    
    prefix_mask <<= (len - MASK_CHUNK);

    //    printf("New node, mask=%08X, new_mask=%08X\n", MASK_PREFIX, (MASK_PREFIX) >> n_crit_bit);
    n =  prefix_mask >> n_crit_bit;
    n &= key;
    n >>= (len - n_crit_bit - 4);
    return (int)n;
}



static inline
void
pat_set_tag(nod *node, int offset)
{

    node->tag |= (1<<offset);

}

static inline
int
pat_get_tag(nod *n, int offset)
{
    return (n->tag & (1<<offset));
}


static inline
void
pat_clear_tag(nod *node, int offset)
{

    node->tag &= (~(1<<offset));

}

static inline
void
pat_clear_tag1(nod *node, int offset)
{

    node->tag1 &= (~(1<<offset));

}

static inline
void
pat_set_tag1(nod *node, int offset)
{

    node->tag1 |= (1<<offset);

}

static inline
int
pat_get_tag1(nod *n, int offset)
{
    return (n->tag1 & (1<<offset));
}

static inline
nodetype
pat_get_nodetype(nod *n, int offset)
{
    int a, b;

    a = pat_get_tag(n, offset);
    b = pat_get_tag1(n, offset);

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
pat_set_nodetype(nod *n, nodetype nt, int offset)
{

    if (!n)
        return;

    pat_clear_tag(n, offset);
    pat_clear_tag1(n, offset);

    switch (nt)
    {
        case n_internal:
            pat_set_tag1(n, offset);
            break;

        case n_external:
            pat_set_tag(n, offset);
            break;

        case n_composite:
            pat_set_tag(n, offset);
            pat_set_tag1(n, offset);
            break;

        default:
            break;
    }
}

static
nod*
pat_new(uint key, int len, int crit_bit, uint mask, void *value, int external)
{
    nod *n;
    int i;


    n = (nod*)malloc(sizeof(nod));
    memset(n, 0, sizeof(nod));
    n->key = key;
    n->mask = mask;
    n->value = value;
    n->crit_bit = crit_bit;

    i = pat_find_slot(key, len, crit_bit);
    if (external)
    {
        pat_set_nodetype(n, n_external, i);
    }
    n->fan[i] = value;


    return n;
}

nod*
pat_insert(nod *r, uint key, int length,  uint mask, void *value)
{
    uint prefix_mask = 0xf0000000;// << (length - MASK_CHUNK);
    int  match = 0;
    int  len = length;
    nod *_r = r;
    nod *last_r;

    uint _key;

    if (length < 32)
    {
        _key = key << (32 - length);
    }
    else
    {
        _key = key;
    }

//    fprintf(stdout, "key=%08X, adjusted_key=%08X\n", key, _key);
    if (!r)
    {
        /* length of key is crit_bit */
        r = pat_new(_key, len, len - MASK_CHUNK, mask, value, 1);
        return r;
    }

    if ((r->mask & r->key)
        == (key & mask))
    {
        /* replace value; */
        r->value = value;
        return r;
    }

    // with a shorter key, a new parent node is created
    if (length < r->crit_bit)
    {
        int bit = 0;
        uint _mask;
        nod *_n;

        int slot0;
        int slot1;

        while (length)
        {
            if (((key << (32 - length)) ^
                 (r->key << (bit))) &
                (0xf0000000 >> bit))
            {
                break;
            }
            bit += MASK_CHUNK;
            length -= MASK_CHUNK;
        }

        _mask = 0xf0000000 >> bit;

        slot0 = (_key & _mask) >> (32 - MASK_CHUNK - bit);
        slot1 = (r->key & _mask) >> (32 - MASK_CHUNK - bit);

        _n = pat_new(0, length, bit, mask, value, 0);

        if (slot0 == slot1)
        {
            printf("Error!!!, SHOULD NOT BE HERE!!!\n");

            /* set flag to internal + external */
            pat_set_nodetype(_n, n_composite, slot0);
            // store value in child
            r->value = value;
            _n->fan[slot1] = r;
        }
        else
        {
            if (length == bit)
            {
                // insert external node
                pat_set_nodetype(_n, n_internal, slot1);
                pat_set_nodetype(_n, n_external, slot0);
                _n->fan[slot0] = value;
                _n->fan[slot1] = r;
            }
            else
            {
                // insert internal node
                nod *n_child = pat_new(_key, length, length - MASK_CHUNK, mask, value, 1);
                /* set flag to external */
                pat_set_nodetype(_n, n_internal, slot0);
                pat_set_nodetype(_n, n_internal, slot1);
                _n->fan[slot0] = n_child;
                _n->fan[slot1] = r;
            }
        }
        return _n;
    }


    /* trace prefix, at MASK_CHUNK a time */

    while (len)
    {
        if (match == r->crit_bit)
        {
            // tripping down to sub tree
            int i = pat_find_slot(_key, 32, match + MASK_CHUNK);
            if (pat_get_nodetype(r, i) == n_internal)
            {
                r = r->fan[i];
            }
            else
            {
                break;
            }
        }
        if ((prefix_mask & _key) ^ ((prefix_mask) & r->key))
        {
            break;
        }
        prefix_mask >>= MASK_CHUNK;
        len -= MASK_CHUNK;
        match += MASK_CHUNK;
    }

    last_r = r;

    while (len)
    {
        int i = pat_find_slot(key, length, match);
        if (match == r->crit_bit)
        {

            nodetype nt = pat_get_nodetype(r, i);


            if (nt == n_empty)
            {
                if (match + MASK_CHUNK == length)
                {
                    // simply insert value and set tag bit
                    r->fan[i] = value;
                    pat_set_tag(r, i);
                }
                else
                {
                    // insert new node
                    nod *n_child;
                    n_child = pat_new(_key, 32, length - MASK_CHUNK, mask, value, 1); /* crit_bit is the last bit */
                    r->fan[i] = n_child;
                    pat_set_nodetype(r, n_internal, i);
                }
                return _r;
            }
            else if (nt == n_external)
            {
                /* The slot is already taken by an external entry, the following steps to be done */
                /* 1. we are going to make a new internal node to fit the current entry */
                /* 2. and assigned the "value" field in the new node with existing value */
                /* 3. clear bit in tag */
                /* 4. set bit in tag1 to signal the new entry are compound entry */
                /* 5. insert the new node to the slot */
                nod *n_child =
                    pat_new(_key, 32, length - MASK_CHUNK, mask, value, 1); /* crit_bit is the last bit, and its an external */

                n_child->value = r->fan[i];
                //pat_set_tag1(r, i);
                //pat_clear_tag(r, i);
                pat_set_nodetype(r, n_composite, i);
                r->fan[i] = n_child;

            }
            else if (nt == n_internal)
            {
                r = r->fan[i];
            }
            else // composite node
            {
                /* The slot contains an internal node */
                /* search sub tree */
                last_r = r;
                r = r->fan[i];
                //printf ("Going in Sub-tree %d\n", i);

                prefix_mask >>= MASK_CHUNK;
                len -= MASK_CHUNK;
                match += MASK_CHUNK;
                continue;
            }

        }
        else if (match < r->crit_bit)
//        else if ((prefix_mask & r->key) ^
//                 (prefix_mask & _key))
        {
#if 0
            int slot;
            printf("match=%d, crit_bit=%d\n", match, r->crit_bit);

            if (match == 0)
            {
                // new top node

                nod *n_root = 
                    pat_new(0, length, match, mask, 0, 0); /* crit_bit is the last bit */
                nod *n_child =
                    pat_new(_key, length, length - MASK_CHUNK, mask, value, 1); /* crit_bit is the last bit */

                int i =  pat_find_slot(key, length, match);

                //pat_set_tag(n_child, i);
                n_root->fan[i] = n_child;
                i =  pat_find_slot(r->key, length, r->crit_bit);
                n_root->fan[i] = r;
                return n_root;
            }
            else if (match > r->crit_bit)
            {
                // 
            }
            else if (match == r->crit_bit)
            {
                /* find slot */
                int bitt = r->crit_bit;
                int i = pat_find_slot(key, length, match);


                if (bitt + MASK_CHUNK >= length)
                {
                    /* insert leaf/value into slot, and set TAG accordingly */
                    r->fan[i] = value;
                    pat_set_tag(r, i);
                }
                else
                {
                    /* make a new internal node, and set TAG accordingly */
                    printf("TODO: Confounded\n");
                    return _r;
                }

            }
            else  /* match  < r->crit_bit */
#endif
            {
                if (match + MASK_CHUNK >= length)
                {
                    /* insert leaf/value into slot, and set TAG accordingly */
                    int i = pat_find_slot(key, length, match);
                    r->fan[i] = value;
                    pat_set_tag(r, i);
                    break;
                }
                else
                {
                    /* split r */
                    int i = pat_find_slot(key, length, match);
                    uint n_mask = 0xffffffff >> match;
                    nod *n_nod, *n_child;
                    int slot_old;

                    /* new node for new key */
                    printf("slot for new entry = %d\n", i);
                    n_child = pat_new(_key, 32, match, n_mask, value, 1); /* crit_bit is the last bit */

                    /* new root for sub-tree */
                    n_nod = pat_new(_key, 32, match, r->mask, r->value, 0);
                    /* connect the old root to the 1st slot */
                    slot_old = pat_find_slot(r->key, 32, match);
                    n_nod->fan[slot_old] = r;

                    /* connect the new child to new root */
                    n_nod->fan[i] = n_child;

                    /* set tags */
                    pat_set_nodetype(n_nod, n_internal, slot_old);
                    pat_set_nodetype(n_nod, n_internal, i);

                    if (r == _r)
                        return n_nod;
                    else
                    {
                        i = pat_find_slot(key, length, match - MASK_CHUNK);
                        last_r->fan[i] = n_nod;
                        return _r;
                    }
                }
            }


            printf("prefix_mask = %08X, match = %d\n", prefix_mask, match);
            break;
        }
        else if (match > r->crit_bit)
        {
            printf("Something weird happened\n");
        }


        prefix_mask >>= MASK_CHUNK;
        len -= MASK_CHUNK;
        match += MASK_CHUNK;
        if (match > r->crit_bit)
        {
            break;
        }
    }

    return _r;
}

void
pat_walk(nod *root, void (*fn)(uint key, int bit, void *v))
{
    int i;
    uint k;


    if (!root)
        return;
    for (i = 0; i < FAN_FACTOR; i++)
    {

        nodetype nt = pat_get_nodetype(root, i);

        k = (root->key) & (0xffffffff << (32 - root->crit_bit));
        k += i << (32 - root->crit_bit - MASK_CHUNK);

        if (nt == n_external)
        {
            fn(k, root->crit_bit + MASK_CHUNK, root->fan[i]);
        }
        else if (nt == n_internal)
        {
            pat_walk(root->fan[i], fn);
        }
        else if (nt == n_composite)
        {
            fn(k, root->crit_bit + MASK_CHUNK, root->fan[i]->value);
            pat_walk(root->fan[i], fn);
        }
    }

}


/*
 * return:
 *  0 for not found
 *  1 for found, value stored in val
 */
int
pat_find(nod *root, uint key, int len, void **val)
{

    int i, j;
    uint k;

    if (!root)
        return 0;

    if (len < 32)
        k = key << (32 - len);
    else
        k = key;

    j = 28;
    if (root->crit_bit == 0)
    {
        i = k >> (32 - MASK_CHUNK);

        if (pat_get_tag(root, i))
        {
            // slot is external data
            if (len == MASK_CHUNK)
            {
                // found
                *val = root->fan[i];
                return 1;
            }
            else
            {
                return 0;
            }

        }
        else
        {
            if (pat_get_tag1(root, i))
            {
                // composite node
                if (len == MASK_CHUNK)
                {
                    // found
                    *val = root->fan[i]->value;
                    return 1;
                }
            }

            // otherwise, drill down to sub-tree
            root = root->fan[i];
            len -= MASK_CHUNK;
            j -= MASK_CHUNK;
        }

    }

    if (!root)
        return 0;

    while (len > root->crit_bit)
    {
        nodetype nt;


        i = (k >> (32 - root->crit_bit - MASK_CHUNK)) & 0xf;

        nt = pat_get_nodetype(root, i);

        switch (nt)
        {
            case n_internal:
            case n_composite:
                root = root->fan[i];
                break;
            default:
                break;
        }
        len -= MASK_CHUNK;
        j -= MASK_CHUNK;
    }

    if (len == root->crit_bit)
    {
        i = (k >> (32 - root->crit_bit - MASK_CHUNK)) & 0xf;
        if (pat_get_nodetype(root, i) == n_external)
        {
            *val = root->fan[i];
            return 1;
        }

    }

    return 0;

}

int
find_slot(uint k, int bit)
{
    if (bit == 0)
    {
        return k>>28;
    }
    else if (bit == 32)
    {
        return k & 0xf;
    }
    else
    {
        return ((k >> (32 - bit - MASK_CHUNK)) & 0x0f);
    }
}

static
int
pat_is_empty(nod* n)
{
    return (n->tag == 0 &&
            n->tag1 == 0);
}
/*
 * pat_delete:
 *  Given a key, remove the entry or node associated with key
 *  within the tree if the key is found, otherwise, do nothing
 *
 * Return:
 *  0 : fail
 *  1 : success
 */
int
pat_delete(nod *n, uint key, int len)
{
    int  r = 0;
    int  i;
    uint k;

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
        uint mask;

        if (n->crit_bit == 0)
            mask = 0xf0000000;
        else if (n->crit_bit == 32)
            mask = n->key;
        else
            mask = 0xffffffff << (32 - n->crit_bit);

        // need to look up the node using partial key
        i = find_slot(k, n->crit_bit);
        nt = pat_get_nodetype(n, i);
        switch (nt)
        {
            case n_internal:
                r = pat_delete(n->fan[i], key, len);
                if (pat_is_empty(n->fan[i]))
                {
                    free(n->fan[i]);
                    pat_set_nodetype(n, n_empty, i);
                    // for debuging
                    n->fan[i] = 0;
                }
                return r;
            case n_composite:
                if (n->crit_bit + MASK_CHUNK == len)
                {
                    pat_set_nodetype(n, n_internal, i);
                    r = 1;
                }
                else
                {
                    r = pat_delete(n->fan[i], key, len);


                    if (pat_is_empty(n->fan[i]))
                    {
                        void *x = n->fan[i]->value;
                        free(n->fan[i]);
                        pat_set_nodetype(n, n_external, i);
                        n->fan[i] = x;
                    }
                }
                break;
            case n_external:
                // varify key is same
                // they should be the same
                // ..
                // unset tag
                pat_set_nodetype(n, n_empty, i);
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
pat_delete_all(nod *root)
{

    int i, n;

    if (!root)
        return;

    n = 1 << MASK_CHUNK;
    for (i = 0; i < n; i++)
    {

        nodetype nt = pat_get_nodetype (root, i);

        switch (nt)
        {

            case n_internal:
                pat_delete_all(root->fan[i]);
                break;

            case n_composite:
                pat_delete_all(root->fan[i]);
                break;

            default:
                break;
        }
    }
    free(root);
}


/*
 * pat_destroy:
 *
 *  Use user provided destructor to delete all leaf objects
 *  Then, the tree is free'd
 *
 */
void
pat_destroy(nod *r, void (*fn)(uint key, int bit, void *v))
{

    pat_walk(r, fn);

    pat_delete_all(r);

}

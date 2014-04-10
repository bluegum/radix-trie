Most of the implementations on Radix tree floating around on the web are binary tree based. While it serves quite well on nature languages, especially those functions of lookup, insertion, and deletion, are well documented, and bounded by O(k), where k is the length of key string, it's space complexity are bounded by O(n log n), which is also good. However, I am interested to see how a n-branch can provide a better memory efficiency on tightly clustered keys, or densely populated maps, and mostly comes with common prefix of various length. The LZW compression algorithm should provide a pretty good bench mark on prefix tree's memory efficiency. For the moment this benchmarking work is in the todo list.

Here comes the n-ary radix tree, called radix-trie, with key length is limited to no more than 32 bit in current incarnation, mostly for the ease of implementation.
Howver the key length can vary from 1 bit to 32 bit. This n-ary radix tree would be at its most memory efficiency when all keys are contiguous and
radix order is 32(the maximum in this implemenation). While for sparse maps, the 2 branch nature of the data structure is most memory efficient while the 32 branch is the least memory efficient. When the keys are sparse and random, The 2 branch gives the best memory usage and the 32 branch give the most memory overhead.

On time complexity, one potential advantage of n-ary trie over it's binary form is that can be more cache friendly, because some node hoppings are replaced by indexing of branchs of the n-ary tree, on insertion and find operations. 


Another motivation behind this configuration of radix tree is the need for a mapping structure is to map unicode code point to charater id as used in PDF, etc. Also assuming the unicode code range are confined in (0 .. 0x10FFFF).


Note:
        performance is not measured, assumed to be within the envelop of binary radix tree for search, insert, and deletion.
        Have run valgrind on memory profile, suggest its suitability for clustered maps, not for maps with random keys.

        The purpose of using radix-trie instead radix-tree is because there is an implement in linux called radix-tree.

How to build:
   ./configure
   make

you will get binary of test0, test1, test2.


If you want to use it in your project, just copy radix-trie.c and radix-trie.h into your source folder.

This software open source and free and will be licensed under MIT license.

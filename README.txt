Most of the implementations on PATRICIA tree floating around on the web are binary tree based. While it serves the nature language based mapping quite well, for more densely populated map, mostly with common prefix, the memory usage overhead can not be under-estimated.

Here comes the n-ary PATRICIA tree, with key length is limited to no more than 32 bit, mostly for the ease of implementation.
This n-ary PATRICIA would be more memory efficient when it is densely populated map. While for sparse map, the n branch nature of the data structure on each internal node make it less efficient than it's binary form. In current implementation, I use 16 branch as default. The branch factor(order) can be to up to 32 per node(Also for each of implementation) . 

Another potential advantage holds over it's binary form is that can be more cache friendly. Reason is it should have less hops than binary tree, on verage insertion and find operations.


Another motivation behind this configuration of radix tree is the need
  for a mapping structure is to map unicode code point to charater id
  as used in PDF, etc. Also assuming the unicode code range are
  confined in (0 .. 0x10FFFF).

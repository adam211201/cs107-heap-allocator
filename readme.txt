File: readme.txt
Author: Adam Barry
----------------------

1. For my explicit heap allocator, I decided to model my header as a size_t data type (an unsigned long), as it allowed me to store pointers
up to the size of 2^63. As a result of aligning our headers to a certain number (in our case the alignment value was 8 bytes), we always had
the three least signifcant bits free in which we could one use of the three bits to store whether or not the payload associated with the header
was free or allocated. The payload was merely a block in memory, and when allocated to the user they would be able to place whatever they wanted
into it. When the payload was freed, we would add a node struct to the start of the payload that contained pointers to two other node structs,
a previous free node struct and the next free node struct. If we were at the beginning of the linked list of free nodes, our previous node struct
would point to NULL, and if we were at the end of our linked list, our next node struct would point to NULL. Any node placed in between two other
free nodes would contain pointers to the node previous and the next node, and those respective nodes would pointer their previous and next nodes
to the current node. This structure would allow us to traverse through the linked list of free nodes much quicker than if we had to traverse through
every header in the heap allocator. In order to accomplish this, we would need to be given a pointer to a free node, and if there was only a single
node in the linked list, its previous and next node pointers would be equal to NULL.

I believe my allocator would should strong performance / utilisation when there is a pattern of calls whereby we call mymalloc and then myfree
straight after. I think this because we have implemented coalescing of free blocks so if we were to make many calls in this pattern, the number
of blocks in the linked list would remain small regardless of the number of calls we make. On the other hand, I think the allocator would show
weak performance if we were to consistently make a bunch of calls firstly to mymalloc and then proceed to make many calls to myfree. This is
because if we are constantly modifying the final free block in the linked list it causes us to run in O(N) time each time (as we don't have access
to the next block (its null) which means we don't have access to the previous block, so we must traverse every block on the heap in order to
find the final block). Our program would also have poor performance in this case if we were to free the blocks in the same order that we added them
as our heap allocator doesn't coalesce to the left meaning all of our free blocks would be adjacent but wouldn't be coalesced.

One optimisation I made in my code was to reduce the number of commands / instructions that were run within a loop. Any command that I thought
could be extracted from the loop was, as if I had left it inside the loop, it would be run many more times that was necessary (in some cases
in O(N) time, which is not good). Also, by coalescing free blocks in my explicit heap allocator, I was able to increase the utilisation of
my heap allocator which allowed for more memory to be spared for further use in the program.

2. There are two assumptions that I have made when designing my explicit heap allocator. The first assumption is that when a user frees a
block on the heap, they do not attempt to reuse that pointer again to assign something to that pointer in memory, as that would cause
the node struct to be overwritten and as a result the linked list containing all the free nodes would be corrupted. This would result in us
not being able to make any more calls to mymalloc or myfree as they both rely on the linked list being correctly formatted i.e. containing
one or more free blocks, with the first blocks previous node pointing to null and the last blocks next node pointing to null, and would
likely result in the program producing a segmentation fault.

Another assumption I make when designing my heap allocator was that I was given correct pointers that pointed to a payload for both myfree
and myrealloc (and that this pointer pointed to memory actually on the heap and not the stack). In the event that my heap allocator was given
a pointer to a piece of incorrect memory or memory not contained on the heap, the program would try, at some point, to convert this payload
into a header, and get the size of it or its allocation status, and would perhaps end up modifying nodes on the linked list or corrupting
certain parts of the heap as a result. Therefore, I expected users of the heap allocator to give me a correctly defined pointer that
referenced the beginning of the payload on the heap.

Tell us about your quarter in CS107!
-----------------------------------

I had a great quarter in CS107. I really enjoyed the assignments and have felt as though I am a much more competent
programmer now compared to before.

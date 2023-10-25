/* CS107 Assignment 7
 * Code by Adam Barry
 *
 * In this program we provide our own implementation of an explicit
 * heap allocator.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./allocator.h"
#include "./debug_break.h"

#define HEADER_SIZE 0x8
#define NODE_POINTER_SIZE 0x8
#define MASKING_BIT 1L
#define MIN_BLOCK_SIZE 0x18

#define FREE 1
#define ALLOCATED 0

static void *segment_start;
static size_t segment_size;
static void *segment_end;
static size_t nused;

typedef struct node node_t;
typedef size_t header_t;

struct node
{
  node_t *prev;
  node_t *next;
};

/* Function: roundup
 * -----------------
 * This function rounds up the given number to the given multiple, which
 * must be a power of 2, and returns the result.  (you saw this code in lab1!).
 */
size_t roundup(size_t num, size_t mult)
{
  return (num + mult - 1) & ~(mult - 1);
}

/* Function: is_free
 * -----------------
 * This function returns whether or not a block is free, this is accomplished by
 * reading only the LSB i.e. the status bit of the header. If the block is free
 * the LSB will be a 1, otherwise it will be a 0.
 */
bool is_free(header_t *header)
{
  return *header & MASKING_BIT;
}

/* Function: set_header
 * -----------------
 * This function sets the properties of a header i.e. its size and status bit. We know that
 * the size passed in should always be a multiple of ALIGNMENT.
 */
void set_header(header_t *header, size_t size, char status)
{
  *header = size;
  *header |= (size_t)status;
}

/* Function: get_size
 * -----------------
 * This function returns the size of a block on the heap by zeroing out the LSB
 * i.e. the status bit of the header.
 */
size_t get_size(header_t *header)
{
  size_t zero_out_lsb = ~(MASKING_BIT);

  return *header & zero_out_lsb;
}

/* Function: header2payload
 * -----------------
 * This function returns a pointer to the payload associated with a certain header.
 */
void *header2payload(header_t *header)
{
  void *payload_ptr = (char *)header + HEADER_SIZE;

  return payload_ptr;
}

/* Function: payload2header
 * -----------------
 * This function returns a pointer to the header associated with a certain payload.
 */
header_t *payload2header(void *payload)
{
  header_t *header_ptr = (header_t *)((char *)payload - HEADER_SIZE);

  return header_ptr;
}

/* Function: next_block
 * -----------------
 * This function returns a pointer to next header block allocated on the heap or null if
 * the block is out of range.
 */
header_t *next_block(header_t *header)
{
  /* find the next header block using the size of the current payload */
  size_t payload_size = get_size(header);

  header_t *next_header_ptr = (header_t *)((char *)header2payload(header) + payload_size);

  /* return null pointer if the next header comes after the end of the heap segment */
  return (next_header_ptr < (header_t *)segment_end) ? next_header_ptr : NULL;
}

/* Function: find_free_block
 * -----------------
 * This function finds the first free block on the heap from a given pointer. The block itself could
 * be returned if it is free.
 */
header_t *find_free_block(header_t *start)
{
  /* if we are given a null pointer then return null */
  if (start != NULL)
  {
    /* find the first free block on the heap */
    while (start != NULL && !is_free(start))
    {
      start = next_block(start);
    }
  }

  return start;
}

/* Function: count_blocks
 * -----------------
 * This function counts the number of blocks on the heap.
 */
size_t count_blocks(header_t *start)
{
  /* if we are passed a null pointer then there are no blocks */
  if (start == NULL)
  {
    return 0;
  }

  int block_count = 0;

  /* traverse through blocks, block-by-block */
  header_t *curr_ptr = start;

  while (curr_ptr != NULL)
  {
    block_count++;

    /* update curr_ptr to point to the next block */
    curr_ptr = next_block(curr_ptr);
  }

  return block_count;
}

/* Function: count_free_blocks
 * -----------------
 * This function counts the number of free blocks on the heap.
 */
size_t count_free_blocks(header_t *start)
{
  header_t *free_block_header = find_free_block(start);

  /* if we are passed a null pointer then there are no free blocks */
  if (free_block_header == NULL)
  {
    return 0;
  }

  node_t *free_block_node = header2payload(free_block_header);

  int block_count = 0;

  /* traverse block by block through counting the number of free blocks we find */
  while (free_block_node != NULL)
  {
    block_count++;

    free_block_node = free_block_node->next;
  }

  return block_count;
}

/* Function: add_free_block
 * -----------------
 * This function adds a new free block into the linked list.
 */
void add_free_block(struct node *free_block_node)
{
  /* if there are no free blocks in the heap then this is the head of the linked list */
  if (count_free_blocks(segment_start) == 0)
  {
    free_block_node->prev = NULL;
    free_block_node->next = NULL;
  }
  else
  {
    header_t *free_block_header = payload2header(free_block_node);
    header_t *next_free_block_header = find_free_block(free_block_header);
    node_t *next_free_block_node = NULL;

    /* this part handles the event that a node is added to the end of the linked list */
    if (next_free_block_header == NULL)
    {
      header_t *first_free_block_header = find_free_block(segment_start);
      node_t *first_free_block_node = header2payload(first_free_block_header);

      /* find last free node is linked list */
      while (first_free_block_node->next != NULL)
      {
        first_free_block_node = first_free_block_node->next;
      }

      free_block_node->prev = first_free_block_node;
      free_block_node->next = NULL;

      first_free_block_node->next = free_block_node;
    }
    /* this part handles the event that a node is added to the start of the linked list */
    else if ((next_free_block_node = header2payload(next_free_block_header))->prev == NULL)
    {
      free_block_node->prev = NULL;
      free_block_node->next = next_free_block_node;

      next_free_block_node->prev = free_block_node;
    }
    /* otherwise we will insert a node in between two other nodes on the linked list */
    else
    {
      free_block_node->prev = next_free_block_node->prev;
      free_block_node->next = next_free_block_node;

      next_free_block_node->prev->next = free_block_node;
      next_free_block_node->prev = free_block_node;
    }
  }
}

/* Function: detach_free_block
 * -----------------
 * This function handles the detaching of the free block from the linked list.
 */
void detach_free_block(struct node *free_payload)
{
  struct node *prev = free_payload->prev;
  struct node *next = free_payload->next;

  /* check edge cases where we are at first or last free block on the heap */
  if (prev != NULL)
  {
    prev->next = next;
  }
  if (next != NULL)
  {
    next->prev = prev;
  }
}

/* Function: split_block
 * -----------------
 * This function handles the splitting of a free block into two parts, an allocated part and
 * a free part. It returns a pointer address to the new node.
 */
void *split_block(header_t *free_block_header, size_t needed, size_t block_size)
{
  /* this part handles the allocating of the block */
  node_t *free_block_node = header2payload(free_block_header);

  set_header(free_block_header, needed, ALLOCATED);

  nused += needed;

  /* this part handles the splitting of the block and adding it back into the linked list */
  header_t *new_free_block_header = (header_t *)((char *)free_block_node + needed);
  node_t *new_free_block_node = header2payload(new_free_block_header);

  size_t new_free_block_size = block_size - HEADER_SIZE - needed;

  set_header(new_free_block_header, new_free_block_size, ALLOCATED);

  nused += HEADER_SIZE;

  return new_free_block_node;
}

/* Function: myinit
 * -----------------
 * This function returns true if initialization was successful, or false otherwise.
 * The myinit function can be called to reset the heap to an empty state. When running
 * against a set of of test scripts, our test harness calls myinit before starting each
 * new script.
 */
bool myinit(void *heap_start, size_t heap_size)
{
  segment_start = heap_start;
  segment_size = heap_size;
  segment_end = (char *)segment_start + segment_size;

  /* if we can't store a header and a node on our heap then it is not big enough */
  if (heap_size < MIN_BLOCK_SIZE)
  {
    return false;
  }

  size_t remaining_space = segment_size - HEADER_SIZE;

  /* set up header at start of heap */
  set_header(segment_start, remaining_space, FREE);

  void *payload_ptr = header2payload((header_t *)segment_start);

  /* set up free node block */
  node_t *free_node = payload_ptr;

  free_node->prev = NULL;
  free_node->next = NULL;

  nused = HEADER_SIZE;

  return true;
}

void *mymalloc(size_t requested_size)
{
  /* handle the case where malloc is passed a value of 0 */
  if (requested_size == 0)
  {
    return NULL;
  }

  size_t needed = roundup(requested_size, ALIGNMENT);

  /* we need to ensure that for each malloc we can store both the previous and next pointers */
  if (needed < (2 * NODE_POINTER_SIZE))
  {
    needed = 2 * NODE_POINTER_SIZE;
  }

  size_t internal_num_bytes = HEADER_SIZE + needed;

  /* if we are requesting more memory than we have available to us (including a header and
   * the size needed), return null
   */
  if (nused + internal_num_bytes > segment_size)
  {
    return NULL;
  }

  header_t *free_block_header = find_free_block((header_t *)segment_start);
  node_t *free_block_node = header2payload(free_block_header);

  /* traverse through all the blocks on the heap */
  while (free_block_node != NULL)
  {
    free_block_header = payload2header(free_block_node);

    size_t free_block_size = get_size(free_block_header);

    /* if we have a perfect fit, we don't need to create a header and a node */
    if (needed == free_block_size)
    {
      detach_free_block(free_block_node);

      set_header(free_block_header, needed, ALLOCATED);

      nused += needed;
    }
    /* if we have a fit with enough room for a header and a node then we need to create a new header and node */
    else if ((needed + MIN_BLOCK_SIZE) <= free_block_size)
    {
      detach_free_block(free_block_node);

      void *new_free_block_node = split_block(free_block_header, needed, free_block_size);

      header_t *new_free_block_header = payload2header(new_free_block_node);

      add_free_block(new_free_block_node);

      set_header(new_free_block_header, get_size(new_free_block_header), FREE);
    }

    /* otherwise we need to go to the next free block */
    free_block_node = free_block_node->next;
  }

  return header2payload(free_block_header);
}

/* Function: myfree
 * -----------------
 * This function frees a block on the heap and updates the header accordingly. If the
 * block has already been freed it does nothing. It also handles the coalescing of two
 * free blocks on the heap.
 */
void myfree(void *ptr)
{
  /* if we try to free a null pointer, then do nothing */
  if (ptr == NULL)
  {
    return;
  }

  header_t *block_header = payload2header(ptr);
  node_t *block_node = ptr;

  /* do nothing if pointer is already free */
  if (!is_free(block_header))
  {
    header_t *next_block_header = next_block(block_header);
    node_t *next_block_node = (node_t *)header2payload(next_block_header);

    size_t block_size = get_size(block_header);

    /* this handles the case where we coalesce */
    if (is_free(next_block_header))
    {
      size_t coalesce_block_size = (block_size + HEADER_SIZE + get_size(next_block_header));

      detach_free_block(next_block_node);

      block_node->prev = next_block_node->next;
      block_node->next = next_block_node->prev;

      set_header(block_header, coalesce_block_size, ALLOCATED);

      add_free_block(block_node);

      set_header(block_header, coalesce_block_size, FREE);

      nused -= (HEADER_SIZE + block_size);
    }
    /* this handles the case where we do not coalesce */
    else
    {
      set_header(block_header, block_size, ALLOCATED);

      add_free_block(block_node);

      set_header(block_header, block_size, FREE);

      nused -= block_size;
    }
  }
}

/* Function: myrealloc
 * -----------------
 * This function reallocates memory and moves it to a new location, freeing the old memory
 * in the meantime. I have used the implicit allocator implementation for realloc as I did
 * not have enough time to do the in-place realloc.
 */
void *myrealloc(void *old_ptr, size_t new_size)
{
  void *new_ptr = mymalloc(new_size);

  /* if old_ptr is null then it is simply a mymalloc call */
  if (old_ptr == NULL)
  {
    return new_ptr;
  }

  /* if the pointer isn't equal to null, we want to either reallocate the memory, or free the
   * pointer, depending on the new_size parameter
   */
  if (new_size != 0)
  {
    memcpy(new_ptr, old_ptr, new_size);
  }

  myfree(old_ptr);

  return new_ptr;
}

/* Function: validate_heap
 * -----------------
 * This function validates the heap periodically to make sure all is OK. If everything is
 * OK we return true, otherwise we return false.
 */
bool validate_heap()
{
  /* if we have used more heap than what is available then throw an error */
  if (nused > segment_size)
  {
    printf("You have used more heap than whats available!\n");

    breakpoint();

    return false;
  }

  size_t num_bytes = 0;
  size_t num_bytes_used = 0;

  header_t *curr_ptr = (header_t *)segment_start;

  /* loop over each block and count the number of bytes used and the number of bytes total */
  do
  {
    size_t block_size = get_size(curr_ptr);

    if (!is_free(curr_ptr))
    {
      num_bytes_used += block_size;
    }

    /* update tracking variables */
    num_bytes += HEADER_SIZE + block_size;
    num_bytes_used += HEADER_SIZE;
  } while ((curr_ptr = next_block(curr_ptr)) != NULL);

  /* return false if the number of bytes used and nused don't match */
  if (num_bytes_used != nused)
  {
    printf("Your program uses %ld bytes, but nused says %ld bytes are accounted for!\n", num_bytes_used, nused);

    breakpoint();

    return false;
  }

  /* return false if the number of bytes total and segment size don't match */
  if (num_bytes != segment_size)
  {
    printf("Your program uses %ld bytes on the heap, but the heap segment size is %ld!\n", num_bytes, segment_size);

    breakpoint();

    return false;
  }

  return true;
}

/* Function: dump_heap
 * -------------------
 * This function prints out the the block contents of the heap. It is not
 * called anywhere, but is a useful helper function to call from gdb when
 * tracing through programs. It prints out the total range of the heap, and
 * information about each block within it.
 */
void dump_heap()
{
  printf("Segment start: %p\n", segment_start);
  printf("Segment end: %p\n", segment_end);
  printf("Segment size: %ld bytes\n", segment_size);
  printf("Nused: %ld bytes\n", nused);
  printf("Num blocks: %ld\n", count_blocks(segment_start));
  printf("Num free blocks: %ld\n\n", count_free_blocks(segment_start));

  header_t *curr_ptr = (header_t *)segment_start;

  printf("%21s %12s %5s\n", "POINTER", "SIZE", "FREE");
  printf("----------------------------------------\n");

  /* loop over each header and payload and print them in a table-like format */
  do
  {
    int space = 10;
    int free = is_free(curr_ptr);
    void *payload = header2payload(curr_ptr);
    size_t size = get_size(curr_ptr);

    printf("Header:  [%p   %*d   %2d]\n", curr_ptr, space, HEADER_SIZE, free);
    printf("Payload: [%p   %10ld   %2d]\n", payload, size, free);

    /* if we have a free block print out its node as well */
    if (free)
    {
      node_t *free_node = payload;

      /* adjusting spacing when printing if either pointer is equal to null */
      int space_prev = free_node->prev == NULL ? 23 : 17;
      int space_next = free_node->next == NULL ? 23 : 17;

      printf("Prev:    [%p %*s]\n", free_node->prev, space_prev, "");
      printf("Next:    [%p %*s]\n", free_node->next, space_next, "");
    }

    printf("\n");
  } while ((curr_ptr = next_block(curr_ptr)) != NULL);
}

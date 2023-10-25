/* CS107 Assignment 7
 * Code by Adam Barry
 *
 * In this program we provide our own implementation of an implicit
 * heap allocator.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./allocator.h"
#include "./debug_break.h"

#define HEADER_SIZE 0x8
#define MASKING_BIT 1L

#define FREE 1
#define ALLOCATED 0

static void *segment_start;
static size_t segment_size;
static void *segment_end;
static size_t nused;

typedef size_t header_t;

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

/* Function: count_blocks
 * -----------------
 * This function counts the number of blocks on the heap.
 */
size_t count_blocks(header_t *start)
{
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

/* Function: fit_block
 * -----------------
 * This function attempts to find a match for a new block of size needed (or close to the size).
 * It returns true if a block was found, and false otherwise.
 */
bool fit_block(size_t needed, header_t **starting_ptr)
{
  /* traverse each block on the heap */
  while (*starting_ptr != NULL)
  {
    size_t block_size = get_size(*starting_ptr);

    bool perfect_match = (needed == block_size);
    bool not_perfect_match = ((needed + (2 * HEADER_SIZE)) <= block_size);

    /* if the current block is free and the new block can fit in it, then we will use this block */
    if (is_free(*starting_ptr) && (perfect_match || not_perfect_match))
    {
      set_header(*starting_ptr, needed, ALLOCATED);

      nused += needed;

      /* if the block isn't a perfect match, we will have to split the block and add a new header */
      if (not_perfect_match)
      {
        header_t *new_header = (header_t *)((char *)header2payload(*starting_ptr) + needed);

        size_t new_header_size = block_size - (needed + HEADER_SIZE);

        set_header(new_header, new_header_size, FREE);

        nused += HEADER_SIZE;
      }

      return true;
    }

    /* update curr_ptr to point to the next block */
    *starting_ptr = next_block(*starting_ptr);
  }

  return false;
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

  size_t remaining_space = segment_size - HEADER_SIZE;

  /* if the heap can't even fit a payload then we return false */
  if (remaining_space < HEADER_SIZE)
  {
    return false;
  }

  /* set up header at start of heap */
  set_header(segment_start, remaining_space, FREE);

  nused = HEADER_SIZE;

  return true;
}

/* Function: mymalloc
 * -----------------
 * The function returns true if initialization was successful, or false otherwise.
 * The myinit function can be called to reset the heap to an empty state. When running
 * against a set of of test scripts, our test harness calls myinit before starting each
 * new script.
 */
void *mymalloc(size_t requested_size)
{
  /* handle the case where malloc is passed a value of 0 */
  if (requested_size == 0)
  {
    return NULL;
  }

  /* if requested_size is greater than max request size we return null */
  if (requested_size > MAX_REQUEST_SIZE)
  {
    return NULL;
  }

  size_t needed = roundup(requested_size, ALIGNMENT);
  size_t internal_num_bytes = HEADER_SIZE + needed;

  /* if we are requesting more memory than we have available to us (including a header and
   * the size needed), return null
   */
  if (nused + internal_num_bytes > segment_size)
  {
    return NULL;
  }

  header_t *curr_ptr = (header_t *)segment_start;

  bool node_fitted = fit_block(needed, &curr_ptr);

  /* make a new header directly after the previous header */
  if (!node_fitted)
  {
    header_t *next_header_ptr = next_block(curr_ptr);

    size_t remaining_space = segment_size - nused;

    set_header(next_header_ptr, remaining_space, FREE);

    nused += HEADER_SIZE;
  }

  /* return the payload of the initial header */
  void *payload_ptr = header2payload(curr_ptr);

  return payload_ptr;
}

/* Function: myfree
 * -----------------
 * This function frees a block on the heap and updates the header accordingly. If the
 * block has already been freed it does nothing.
 */
void myfree(void *ptr)
{
  /* if we try to free a null pointer, then do nothing */
  if (ptr == NULL)
  {
    return;
  }

  header_t *header_ptr = payload2header(ptr);

  /* do nothing if pointer is already free */
  if (!is_free(header_ptr))
  {
    header_t *next_block_ptr = next_block(header_ptr);

    size_t curr_block_size = get_size(header_ptr);

    /* coalesce two free blocks if they are adjacent */
    if (is_free(next_block_ptr))
    {
      size_t next_block_size = get_size(next_block_ptr);
      size_t new_size = curr_block_size + HEADER_SIZE + next_block_size;

      set_header(header_ptr, new_size, FREE);

      nused -= HEADER_SIZE + curr_block_size;
    }
    /* if the two adjacent blocks aren't adjacent, simply free the block */
    else
    {
      set_header(header_ptr, curr_block_size, FREE);

      /* we want to keep the header and only remove the payload size */
      nused -= curr_block_size;
    }
  }
}

/* Function: myrealloc
 * -----------------
 * This function reallocates memory and moves it to a new location, freeing the old memory
 * in the meantime.
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
  printf("Num blocks: %ld\n\n", count_blocks(segment_start));

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
    printf("Payload: [%p   %10ld   %2d]\n\n", payload, size, free);
  } while ((curr_ptr = next_block(curr_ptr)) != NULL);
}

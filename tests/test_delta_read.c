#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <librsync.h>

/* Test behaviour when the input to the delta is finished, it needs many attempts
 * before it can finally be unblocked, even when there is plenty of output buffer space.
 */

static void expect(int r, int expected, int line)
{
   if (r != expected)
   {
      printf("------> FAIL on line %d, expected %d got %d\n", line, expected, r);
      exit(1);
   }
}


typedef struct
{
   char* buf;
   size_t size;
} Memory;


int be_bad = 1;

static rs_result read_memory_callback(void * opaque, rs_long_t pos, size_t * len, void ** buf)
{
   Memory * mem = (Memory*)opaque;
   assert(pos < mem->size);
   *buf = mem->buf + pos;

   size_t avail = mem->size-pos;

   if (be_bad)
       *len = avail;
   else
   {
       /* be good */
       if (avail < *len)
           *len = avail;
   }

   return RS_DONE;
}



int main(int argc, const char** argv)
{
   int i;

   if (argc < 3)
   {
      printf(
              "Usage: test_delta_read.test input_size do_bad_callback(1/0)\n"
              "This tests the behaviour of the library with a badly behaved callback.\n"
              "Functions that follow rs_copy_cb() MUST only return UP TO 'len' bytes.\n"
              "\n"
              "Usage Example:\n"
              "./test_delta_read.test 2000000 0  --> shows results of correct behaviour\n"
              "./test_delta_read.test 2000000 1  --> shows results of bad behaviour\n"
              );
      return 1;
   }

   be_bad = atoi(argv[2]);

   Memory input;
   input.size = atoi(argv[1]);
   input.buf = (char*)malloc(input.size);
   for (i = 0; i < input.size; ++i)
      input.buf[i] = 'x';

   printf("input size %lu\n", input.size);

   Memory sig;
   sig.size = 1024 + 10*input.size;  /* ensure we have enough space... */
   sig.buf = (char*)malloc(sig.size);
   size_t siglen = 0;
   rs_signature_t * sigobj = NULL;

   Memory delta;
   delta.size = 1024 + 10*input.size;   /* ensure we have enough space... */
   delta.buf = (char*)malloc(delta.size);

   Memory output;
   output.size = 1024 + 10*input.size;   /* ensure we have enough space... */
   output.buf = (char*)malloc(output.size);

   rs_buffers_t buf;
   rs_job_t * job;

   /* generate a signature */
   buf.next_out = sig.buf;
   buf.avail_out = sig.size;
   buf.next_in = (char*)(input.buf);   /* TODO why is this not const void* ? */
   buf.avail_in = input.size;
   buf.eof_in = 1;

   /* signature in one hit (with eof=1) */
   job = rs_sig_begin(RS_DEFAULT_BLOCK_LEN, 0, RS_BLAKE2_SIG_MAGIC);
   expect(rs_job_iter(job, &buf), RS_DONE, __LINE__);
   rs_job_free(job);
   siglen = sig.size - buf.avail_out;   /* remember how long it is */
   printf("Signature size: %lu\n", siglen);
   /* SIGNATURE is now in sig.buf */


   /* OK, next step... load the signature */
   job = rs_loadsig_begin(&sigobj);

   buf.next_out = NULL;
   buf.avail_out = 0;
   buf.next_in = sig.buf;   /* TODO why is this not const void* ? */
   buf.avail_in = siglen;
   buf.eof_in = 0;
   expect(rs_job_iter(job, &buf), RS_BLOCKED, __LINE__);

   buf.next_out = NULL;
   buf.avail_out = 0;
   buf.next_in = NULL;   /* TODO why is this not const void* ? */
   buf.avail_in = 0;
   buf.eof_in = 1;
   expect(rs_job_iter(job, &buf), RS_DONE, __LINE__);

   rs_job_free(job);
   /* signature loaded */

   /* build hash table */
   expect(rs_build_hash_table(sigobj), RS_DONE, __LINE__);
   /* rs_sumset_dump(sigobj); */

   
   /* send through the same input, compute delta */
   job = rs_delta_begin(sigobj);
   buf.next_out = delta.buf;
   buf.avail_out = delta.size;
   buf.next_in = (char*)input.buf;
   buf.avail_in = input.size;
   buf.eof_in = 0;
   expect(rs_job_iter(job, &buf), RS_BLOCKED, __LINE__);

   /* now push through an EOF. I'd expect this to go through in one step. */
   buf.next_in = NULL;
   buf.avail_in = 0;
   buf.eof_in = 1;

   /* STEP 1 (EOF) */
   rs_result r1 = rs_job_iter(job, &buf);
   printf("EOF Step 1.. Wanted RS_DONE (0), got %d (probably RS_BLOCKED (1) which is wrong...)\n", r1);
   printf("next_in: %p\navail_in: %lu\nnext_out: %p\navail_out: %lu\n", buf.next_in, buf.avail_in, buf.next_out, buf.avail_out);

   /* STEP 2 (EOF) */
   rs_result r2 = rs_job_iter(job, &buf);
   printf("EOF Step 2.. Wanted RS_DONE (0), got %d (should be ok)\n", r2);
   printf("next_in: %p\navail_in: %lu\nnext_out: %p\navail_out: %lu\n", buf.next_in, buf.avail_in, buf.next_out, buf.avail_out);

   printf("------> Delta size is %lu\n", delta.size - buf.avail_out);
   rs_job_free(job);



   /* Now, to generate the same data WITH the delta's help */
   buf.next_out = output.buf;
   buf.avail_out = output.size;
   buf.next_in = (char*)delta.buf;
   buf.avail_in = delta.size;
   buf.eof_in = 1;
   job = rs_patch_begin(read_memory_callback, &input);
   expect(rs_job_iter(job, &buf), RS_DONE, __LINE__);

   int output_len = output.size - buf.avail_out;

   expect(output_len, input.size, __LINE__);

   if (memcmp(input.buf, output.buf, input.size) != 0)
   {
      printf("------> FAIL on line %d, output did not match input\n", __LINE__);
      exit(1);
   }
 


   /* expect to do this in 1 step... (fail at the end, this is a known problem) */
   expect(r1, RS_DONE, __LINE__);

   printf("SUCCESS");

   return 0;
}

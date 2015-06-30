#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <librsync.h>

// Test behaviour when the input to the delta is finished, it needs many attempts
// before it can finally be unblocked, even when there is plenty of output buffer space.

static void expect(int r, int expected, int line)
{
   if (r != expected)
   {
      printf("------> FAIL on line %d, expected %d got %d\n", line, expected, r);
      exit(1);
   }
}



int main(int argc, const char** argv)
{
   int i;
   // const char* input = "Hello World";
   // const size_t input_size = strlen(input);

   if (argc < 2)
   {
      printf("specify input size\n");
      return 1;
   }

   size_t input_size = atoi(argv[1]);
   char * input = (char*)malloc(input_size);
   for (i = 0; i < input_size; ++i)
      input[i] = 'x';

   printf("input size %lu\n", input_size);

   size_t sigbuf_size = 1024 + 10*input_size;  // ensure we have enough space...
   char * sigbuf = (char*)malloc(sigbuf_size);
   size_t siglen = 0;
   rs_signature_t * sigobj = NULL;

   size_t outbuf_size = 1024 + 10*input_size;   // ensure we have enough space...
   char * outbuf = (char*)malloc(outbuf_size);

   rs_buffers_t buf;
   rs_job_t * job;

   // generate a signature
   buf.next_out = sigbuf;
   buf.avail_out = sigbuf_size;
   buf.next_in = (char*)(input);   // TODO why is this not const void* ?
   buf.avail_in = input_size;
   buf.eof_in = 1;

   // signature in one hit (with eof=1)
   job = rs_sig_begin(RS_DEFAULT_BLOCK_LEN, 0, RS_BLAKE2_SIG_MAGIC);
   expect(rs_job_iter(job, &buf), RS_DONE, __LINE__);
   rs_job_free(job);
   siglen = sigbuf_size - buf.avail_out;   // remember how long it is
   printf("Signature size: %lu\n", siglen);
   // SIGNATURE is now in sigbuf


   // OK, next step... load the signature
   job = rs_loadsig_begin(&sigobj);

   buf.next_out = NULL;
   buf.avail_out = 0;
   buf.next_in = sigbuf;   // TODO why is this not const void* ?
   buf.avail_in = siglen;
   buf.eof_in = 0;
   expect(rs_job_iter(job, &buf), RS_BLOCKED, __LINE__);

   buf.next_out = NULL;
   buf.avail_out = 0;
   buf.next_in = NULL;   // TODO why is this not const void* ?
   buf.avail_in = 0;
   buf.eof_in = 1;
   expect(rs_job_iter(job, &buf), RS_DONE, __LINE__);

   rs_job_free(job);
   // signature loaded

   // build hash table
   expect(rs_build_hash_table(sigobj), RS_DONE, __LINE__);
   // rs_sumset_dump(sigobj);

   
   // send through the same input, compute delta
   job = rs_delta_begin(sigobj);
   buf.next_out = outbuf;  // reset outbuf again
   buf.avail_out = outbuf_size;
   buf.next_in = (char*)input;
   buf.avail_in = input_size;
   buf.eof_in = 0;
   expect(rs_job_iter(job, &buf), RS_BLOCKED, __LINE__);

   // now push through an EOF. I'd expect this to go through in one step.
   buf.next_in = NULL;
   buf.avail_in = 0;
   buf.eof_in = 1;

   // STEP 1 (EOF)
   rs_result r1 = rs_job_iter(job, &buf);
   printf("EOF Step 1.. Wanted RS_DONE (0), got %d (probably RS_BLOCKED (1) which is wrong...)\n", r1);
   printf("next_in: %p\navail_in: %lu\nnext_out: %p\navail_out: %lu\n", buf.next_in, buf.avail_in, buf.next_out, buf.avail_out);

   // STEP 2 (EOF)
   rs_result r2 = rs_job_iter(job, &buf);
   printf("EOF Step 2.. Wanted RS_DONE (0), got %d (should be ok)\n", r2);
   printf("next_in: %p\navail_in: %lu\nnext_out: %p\navail_out: %lu\n", buf.next_in, buf.avail_in, buf.next_out, buf.avail_out);

   printf("------> Delta size is %lu\n", outbuf_size - buf.avail_out);
   rs_job_free(job);

   // expect to do this in 1 step...
   expect(r1, RS_DONE, __LINE__);

   printf("SUCCESS");

   return 0;
}

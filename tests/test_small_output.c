#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <librsync.h>

// Test behaviour when the output buffer is too small, and grows bit by bit
// Parameters: test_small_output.test  INPUTSIZE  INITIAL_BUFFER_SIZE  OUTPUT_GROWTH_RATE

int main(int argc, const char** argv)
{
   int i;
   size_t input_size = atoi(argv[1]);
   size_t start = atoi(argv[2]);
   size_t step = atoi(argv[3]);

   printf("Input size: %d\n", input_size);
   printf("Output buffer start at %d step growth of %d\n", start, step);

   // make this silly-huge, we don't want to test part
   char * outbuf = (char*)malloc(1024+10*input_size);

   // load with anything
   char * in = (char*)malloc(input_size);
   for (i = 0; i < input_size; ++i)
      in[i] = 'x';

   // generate a signature
   rs_buffers_t buf;
   buf.next_out = outbuf;
   buf.avail_out = start;
   buf.next_in = (char*)(in);   // TODO why is this not const void* ?
   buf.avail_in = input_size;
   buf.eof_in = 0;

   rs_job_t * job = rs_sig_begin(RS_DEFAULT_BLOCK_LEN, 0, RS_BLAKE2_SIG_MAGIC);

   while (buf.avail_in > 0)
   {
      rs_result r = rs_job_iter(job, &buf);
      if (r != RS_BLOCKED)
      {
         printf("FAIL, expected 'blocked'\n");
         return 1;
      }
      // if there is data remaining, then rsync needs more output
      // give it a little each time...
      if (buf.avail_in > 0)
         buf.avail_out += step;
   }

   // all the input has been consumed.
   // run the job once more, with no data, and the EOF flag set.
   // we should see "DONE" now.
   buf.eof_in = 1;
   rs_result r = rs_job_iter(job, &buf);
   if (r != RS_DONE)
   {
      printf("FAIL, expected 'done'\n");
      return 1;
   }

   rs_job_free(job);

   printf("Final signature size: %d\n", (buf.next_out - outbuf));

   return 0;
}

# TODO

* We have a few functions to do with reading a netint, stashing
  it somewhere, then moving into a different state.  Is it worth
  writing generic functions for that, or would it be too confusing?

* Optimisations and code cleanups;

  scoop.c: Scoop needs major refactor. Perhaps the API needs
  tweaking?

  rsync.h: rs_buffers_s and rs_buffers_t should be one typedef?

  mdfour.c: This code has a different API to the RSA code in libmd
  and is coupled with librsync in unhealthy ways (trace?). Recommend
  changing to RSA API?

* Just how useful is rs_job_drive anyway?

* Don't use the rs_buffers_t structure.

  There's something confusing about the existence of this structure.
  In part it may be the name.  I think people expect that it will be
  something that behaves like a FILE* or C++ stream, and it really
  does not.  Also, the structure does not behave as an object: it's
  really just a shorthand for passing values in to the encoding
  routines, and so does not have a lot of identity of its own.

  An alternative might be

    result = rs_job_iter(job,
                         in_buf, &in_len, in_is_ending,
                         out_buf, &out_len);

  where we update the length parameters on return to show how much we
  really consumed.

  One technicality here will be to restructure the code so that the
  input buffers are passed down to the scoop/tube functions that need
  them, which are relatively deeply embedded.  I guess we could just
  stick them into the job structure, which is becoming a kind of
  catch-all "environment" for poor C programmers.

* Meta-programming

  * Plot lengths of each function

  * Some kind of statistics on delta each day

* Encoding format

  * Include a version in the signature and difference fields

  * Remember to update them if we ever ship a buggy version (nah!) so
    that other parties can know not to trust the encoded data.

* abstract encoding

  In fact, we can vary on several different variables:

    * what signature format are we using

    * what command protocol are we using

    * what search algorithm are we using?

    * what implementation version are we?

  Some are more likely to change than others.  We need a chart
  showing which source files depend on which variable.

* Encoding algorithm

  * Self-referential copy commands

    Suppose we have a file with repeating blocks.  The gdiff format
    allows for COPY commands to extend into the *output* file so that
    they can easily point this out.  By doing this, they get
    compression as well as differencing.

    It'd be pretty simple to implement this, I think: as we produce
    output, we'd also generate checksums (using the search block
    size), and add them to the sum set.  Then matches will fall out
    automatically, although we might have to specially allow for
    short blocks.

    However, I don't see many files which have repeated 1kB chunks,
    so I don't know if it would be worthwhile.

* Support compression of the difference stream.  Does this
  belong here, or should it be in the client and librsync just have
  an interface that lets it cleanly plug in?

  I think if we're going to just do plain gzip, rather than
  rsync-gzip, then it might as well be external.

  rsync-gzip: preload with the omitted text so as to get better
  compression.  Abo thinks this gets significantly better
  compression.  On the other hand we have to important and maintain
  our own zlib fork, at least until we can persuade the upstream to
  take the necessary patch.  Can that be done?

  abo says

       It does get better compression, but at a price. I actually
       think that getting the code to a point where a feature like
       this can be easily added or removed is more important than the
       feature itself. Having generic pre and post processing layers
       for hit/miss data would be useful. I would not like to see it
       added at all if it tangled and complicated the code.

       It also doesn't require a modified zlib... pysync uses the
       standard zlib to do it by compressing the data, then throwing
       it away. I don't know how much benefit the rsync modifications
       to zlib actually are, but if I was implementing it I would
       stick to a stock zlib until it proved significantly better to
       go with the fork.

* Licensing

  Will the GNU Lesser GPL work?  Specifically, will it be a problem
  in distributing this with Mozilla or Apache?

* Testing

  * Just more testing in general.

  * Test broken pipes and that IO errors are handled properly.

  * Test files >2GB, >4GB.  Presumably these must be done in streams
    so that the disk requirements to run the test suite are not too
    ridiculous.  I wonder if it will take too long to run these
    tests?  Probably, but perhaps we can afford to run just one
    carefully-chosen test.

  * Fuzz instruction streams. <https://code.google.com/p/american-fuzzy-lop/>?

  * Generate random data; do random mutations.

  * Tests should fail if they can't find their inputs, or have zero
    inputs: at present they tend to succeed by default.

* Security audit

  * If this code was to read differences or sums from random machines
    on the network, then it's a security boundary.  Make sure that
    corrupt input data can't make the program crash or misbehave.

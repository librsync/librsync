# NEWS

## librsync 2.3.5

NOT RELEASED YET

## librsync 2.3.4

Released 2023-02-19

 * Fix #248 by putting `#include "config.h"` with `/* IWYU pragma: keep */` in
   most `src/*.c` files. Add `/* IWYU pragma: keep */` to includes in
   `src/fileutil.c` that are needed on some platforms but not others so we can
   remove the special exemptions to skip this file for the iwyu and iwyu-fix
   targets in `CMakeLists.txt`. Also add some typecasts to `rollsum.[ch]` and
   `patch.c` to silence warnings on Windows. (dbaarda,
   https://github.com/librsync/librsync/pull/249)

## librsync 2.3.3

Released 2023-02-16

 * Fix #244 Add windows build to stable release. Updated CONTRIBUTING.md
   release instructions to be clearer and include instructions on how to
   upload the win64 install artifact from the github "Check" action. (dbaarda,
   https://github.com/librsync/librsync/pull/245)

 * Update github actions and fix `iwyu` build target. Update `checkout` and
   `upload-artifact` to v3. Update `lint.yml` installed packages for fixed
   iwyu deps. Fix `iwyu` build target to ignore `fileutil.c` and use neater
   clang output with noisy "note:" output removed.  Run `make iwyu-fix` to fix
   includes for `tests/rabinkarp_perf.c`. (dbaarda,
   https://github.com/librsync/librsync/pull/243)

 * Add missing word to README.md. (AvdN,
   https://github.com/librsync/librsync/pull/237)

 * Make delta directly process the input stream if it has enough data. Delta
   operations will only accumulate data into the internal scoop buffer if the
   input buffer is too small, otherwise it will process the input directly.
   This makes delta calculations 5%~15% faster by avoiding extra data copying.
   (dbaarda, https://github.com/librsync/librsync/pull/234)

 * Add .gitignore for `.cmake` created by LSP on Windows. (sourcefrog,
   https://github.com/librsync/librsync/pull/232)

 * Upload build and install artifacts from Github actions. This means we get
   downloadable build and install artifacts for all platforms from the "Check"
   action. (sourcefrog, https://github.com/librsync/librsync/pull/231)

 * Improve documentation so that Doxygen generates more complete documentation
   with diagrams, renders better, and is more navigable as markdown docs on
   GitHub. (dbaarda, https://github.com/librsync/librsync/pull/230)

 * Add github action and make targets for clang-tidy and iwyu. Added
   `clang-tidy` and `iwyu` make targets for checking code and includes, and
   `iwyu-fix` for fixing includes. Added `lint.yml` GitHub action to run these
   checks. Fixed all `clang-tidy` and `iwyu` warnings except for `fileutil.c`
   with platform related include complications. Added consistent include
   guards to all headers. Updated and improved documentation in
   CONTRIBUTING.md to include these changes. (rizsotto, dbaarda,
   https://github.com/librsync/librsync/pull/229)

 * Tidy rdiff integration test scripts. Made the filenames and shell arguments
   for test scripts consistent. (dbaarda,
   https://github.com/librsync/librsync/pull/227)

 * Add better cmake build type configuration support. Added `BuildType.cmake`
   with better support for selecting the build type and making it default to
   Debug. (dbaarda, https://github.com/librsync/librsync/pull/226)

 * Fix #215 Migrate from Travis to GitHub Actions. Added a check.yml GitHub
   action with updated test/platform matrix including full testing of rdiff on
   Windows. (rizsotto, dbaarda, https://github.com/librsync/librsync/pull/225)

 * Fix bash test scripts to work on Windows. Tweaked cmake configuration and
   bash script tests so that full rdiff tests using libpopt from vcpkg work.
   Running `cmake --target check` with rdiff compiled now works on windows.
   (dbaarda, https://github.com/librsync/librsync/pull/224)

 * Remove obsolete unused tests. Removed some obsolete mdfour test data files
   and `check-rdiff` perl script. (dbaarda,
   https://github.com/librsync/librsync/pull/223)

 * Fix warning for later CMake versions. New CMake versions started
   complaining about the filename `Findlibb2.cmake` not matching the LIBB2
   variables being used. (rizsotto,
   https://github.com/librsync/librsync/pull/221)

## librsync 2.3.2

Released 2021-04-10

 * Fix #214 heap corruption for too small kbloom. This could have crashed
   delta operations for very small files/signatures. Strangely it didn't seem
   to cause problems for most compilers/platforms, but did trigger errors for
   new versions of MSVC. (ljusten,
   https://github.com/librsync/librsync/pull/213)

 * Fix #207 and add Travis Windows checks and improve compatibility. Turn on
   `-Wconversion -Wno-sign-conversion` warnings for clang. Add MSVC compiler
   flags to turn off posix warnings. Make all code compile clean with no
   warnings on all Travis platforms. Added cmake config checking for windows
   `io.h` and improve `fileutil.c` for MSVC. Fix broken error handling in
   `rs_file_copy_cb()`. Improved trace output, making it less spamy and more
   consistent. Add patch checking for invalid literal lengths. Improve
   internal variable and argument types. Add explicit type conversions.
   (dbaarda, https://github.com/librsync/librsync/pull/208)

 * Fix a bug so patch will now fail returning RS_CORRUPT on encountering a
   zero length copy command instead of hanging. Make copy_cb() copying more
   data than requested an assert-fail on debug builds, and a log-warning for
   release builds. Make trace output a little less spammy about copy_cb()
   return values. (dbaarda, https://github.com/librsync/librsync/pull/206)

## librsync 2.3.1

Released 2020-05-19

 * Fix #198 cmake popt detection using pkg-config and #199 test scripts on
   FreeBSD. Fixes and tidies FindPOPT.cmake and Findlibb2.cmake to use
   pkg-config correctly and behave more like official FindPackage() cmake
   modules. Makes all test scripts use /bin/sh instead of /bin/bash. (dbaarda,
   mandree https://github.com/librsync/librsync/pull/200)

 * Change default block_len to always be a multiple of the blake2b 128 byte
   blocksize for efficiency. Tidy and update docs to explain using
   rs_sig_args() and rs_build_hash_table(), add rs_file_*() utils, and
   document new magic types. Remove really obsolete entries in TODO.md. Update
   to Doxygen 1.8.16. (dbaarda, https://github.com/librsync/librsync/pull/195)

 * Improve hashtable performance by adding a small optional bloom filter,
   reducing max loadfactor from 80% to 70%, Fix hashcmp_count stats to include
   comparing against empty buckets. This speeds up deltas by 20%~50%.
   (dbaarda, https://github.com/librsync/librsync/pull/192,
   https://github.com/librsync/librsync/pull/193,
   https://github.com/librsync/librsync/pull/196)

 * Optimize rabinkarp_update() by correctly using unsigned constants and
   manually unrolling the loop for best performance. (dbaarda,
   https://github.com/librsync/librsync/pull/191)

## librsync 2.3.0

Released 2020-04-07

 * Bump minor version from 2.2.1 to 2.3.0 to reflect additional rs_sig_args()
   and strong_len=-1 support.

 * Add public rs_sig_args() function for getting the recommend signature args
   from the file size. Added support to rdiff for `--sum-size=-1` to indicate
   "use minimum size safe against random block collisions". Added warning
   output for sum-sizes that are too small to be safe. Fixed possible rdiff
   bug affecting popt parsing on non-little-endian platforms. (dbaarda,
   https://github.com/librsync/librsync/pull/109)

 * Fixed yet more compiler warnings for various platforms/compilers.
   (Adsun701, texierp, https://github.com/librsync/librsync/pull/187,
   https://github.com/librsync/librsync/pull/188)

 * Improved cmake popt handling to find popt dependencies using PkgConfig.
   (ffontaine, https://github.com/librsync/librsync/pull/186)

 * Tidied internal code and improved tests for netint.[ch], tube.c, and
   hashtable.h. (dbaarda, https://github.com/librsync/librsync/pull/183
   https://github.com/librsync/librsync/pull/185).

 * Improved C99 compatibility. Add `-std=c99 -pedantic` to `CMAKE_C_FLAGS` for
   gcc and clang. Fix all C99 warnings by making all code C99 compliant. Tidy
   all CMake checks, `#cmakedefines`, and `#includes`. Fix 64bit support for
   mdfour checksums (texierp, dbaarda,
   https://github.com/librsync/librsync/pull/181,
   https://github.com/librsync/librsync/pull/182)

 * Usage clarified in rdiff (1) man page. (AaronM04,
   https://github.com/librsync/librsync/pull/180)

## librsync 2.2.1

Released 2019-10-16

 * Fix #176 hangs calculating deltas for files larger than 4GB. (dbaarda,
   https://github.com/librsync/librsync/pull/177)

## librsync 2.2.0

Released 2019-10-12

 * Bump minor version from 2.1.0 to 2.2.0 to reflect additional RabinKarp
   rollsum support.

 * Fix MSVC builds by adding missing LIBRSYNC_EXPORT to variables in
   librsync.h, add -DLIBRSYNC_STATIC_DEFINE to the sumset_test target,
   and correctly install .dll files in the bin directory.
   (adsun701, https://github.com/librsync/librsync/pull/161)

 * Add RabinKarp rollsum support and make it the default. RabinKarp is a much
   better rolling hash, which reduces the risk of hash collision corruption
   and speeds up delta calculations. The rdiff cmd gets a new `-R
   (rollsum|rabinkarp)` argument with the default being `rabinkarp`, Use `-R
   rollsum` to generate backwards-compatible signatures. (dbaarda,
   https://github.com/librsync/librsync/issues/3)

 * Use single-byte literal commands for small inserts in deltas. This makes
   each small insert use 1 less byte in deltas. (dbaarda,
   https://github.com/librsync/librsync/issues/120)

 * Fix multiple warnings (cross-)compiling for windows. (Adsun701,
   https://github.com/librsync/librsync/pull/165,
   https://github.com/librsync/librsync/pull/166)

 * Change rs_file_size() to report -1 instead of 0 for unknown file sizes (not
   a regular file). (dbaarda https://github.com/librsync/librsync/pull/168)

 * Add cmake BUILD_SHARED_LIBS option for static library support.
   BUILD_SHARED_LIBS defaults to ON, and can be set to OFF using `ccmake .` to
   build librsync as a static library. (dbaarda
   https://github.com/librsync/librsync/pull/169)

 * Fix compile errors and add .gitignore entries for MSVS 2019. Fixes
   hashtable.h to be C99 compliant. (ardovm
   https://github.com/librsync/librsync/pull/170)

## librsync 2.1.0

Released 2019-08-19

 * Bump minor version from 2.0.3 to 2.1.0 to reflect additions to librsync.h.

 * Fix exporting of private symbols from librsync library. Add export of
   useful large file functions `rs_file_open()`, `rs_file_close()`, and
   `rs_file_size()` to librsync.h. Add export of `rs_signature_log_stats()` to
   log signature hashtable hit/miss stats. Improve rdiff error output.
   (dbaarda, https://github.com/librsync/librsync/issues/130)

 * Updated release process to include stable tarballs. (dbaarda,
   https://github.com/librsync/librsync/issues/146)

 * Remove redundant and broken `--paranoia` argument from rdiff. (dbaarda,
   https://github.com/librsync/librsync/issues/155)

 * Fix memory leak of `rs_signature_t->block_sigs` when freeing signatures.
   (telles-simbiose, https://github.com/librsync/librsync/pull/147)

 * Document delta file format. (zmj,
   https://github.com/librsync/librsync/issues/46)

 * Fix up doxygen comments. (dbaarda,
   https://github.com/librsync/librsync/pull/151)

## librsync 2.0.2

Released 2018-02-27

 * Improve CMake install paths configuration (wRAR,
   https://github.com/librsync/librsync/pull/133) and platform support
   checking when cross-compiling (fornwall,
   https://github.com/librsync/librsync/pull/136).

 * Fix Unaligned memory access for rs_block_sig_init() (dbaarda,
   https://github.com/librsync/librsync/issues/135).

 * Fix hashtable_test.c name collision for key_t in sys/types.h on some
   platforms (dbaarda, https://github.com/librsync/librsync/issues/134)

 * Format code with consistent style, adding `make tidy` and `make
   tidyc` targets for reformating code and comments. (dbaarda,
   https://github.com/librsync/librsync/issues/125)

 * Removed perl as a build dependency. Note it is still required for some
   tests. (dbaarda, https://github.com/librsync/librsync/issues/75)

 * Update RPM spec file for v2.0.2 and fix cmake man page install. (deajan,
   https://github.com/librsync/librsync/issues/47)

## librsync 2.0.1

Released 2017-10-17

 * Extensively reworked Doxygen documentation, now available at
   http://librsync.sourcefrog.net/ (Martin Pool)

 * Removed some declarations from librsync.h that were unimplemented or no
   longer ever useful: `rs_work_options`, `rs_accum_value`. Remove
   declaration of unimplemented `rs_mdfour_file()`. (Martin Pool)

 * Remove shipped `snprintf` code: no longer acutally linked after changing to
   CMake, and since it's part of C99 it should be widely available.
   (Martin Pool)

 * Document that Ninja (http://ninja-build.org/) is supported under CMake.
   It's a bit faster and nicer than Make. (Martin Pool)

 * `make check` (or `ninja check` etc) will now build and run the tests.
   Previously due to a CMake limitation, `make test` would only run existing
   tests and could fail if they weren't built.
   (Martin Pool, https://github.com/librsync/librsync/issues/49)

 * Added cmake options to exclude rdiff target and compression from build.
   See install documentation for details. Thanks to Michele Bertasi.

 * `popt` is only needed when `rdiff` is being built. (gulikoza)

 * Improved large file support for platforms using different variants
   of `fseek` (`fseeko`, `fseeko64`, `_fseeki64`), `fstat` (`fstat64`,
   `_fstati64`), and `fileno` (`_fileno`). (dbaarda, charlievieth,
   gulikoza, marius-nicolae)

 * `rdiff -s` option now shows bytes read/written and speed. (gulikoza).
   For delta operations it also shows hashtable match statistics. (dbaarda)

 * Running rdiff should not overwrite existing files (signatures, deltas and
   new patched files) by default. If the destination file exists, rdiff will
   now exit with an error. Add new option -f (--force) to overwrite existing
   files. (gulikoza)

 * Improve signature memory allocation (doubling size instead of calling
   realloc for every sig block) and added support for preallocation. See
   streaming.md job->estimated_signature_count for usage when using the
   library. `rdiff` uses this by default if possible. (gulikoza, dbaarda)

 * Significantly tidied signature handling code and testing, resulting in more
   consistent error handling behaviour, and making it easier to plug in
   alternative weak and strong sum implementations. Also fixed "slack delta"
   support for delta calculation with no signature. (dbaarda)

 * `stdint.h` and `inttypes.h` from C99 is now required. Removed redundant
   librsync-config.h header file. (dbaarda)

 * Lots of small fixes for windows platforms and building with MSVC.
   (lasalvavida, mbrt, dbaarda)

 * New open addressing hashtable implementation that significantly speeds up
   delta operations, particularly for large files. Also fixed degenerate
   behaviour with large number of duplicate blocks like runs of zeros
   in sparse files. (dbaarda)

 * Optional support with cmake option for using libb2 blake2 implementation.
   Also updated included reference blake2 implementation with bug fixes
   (dbaarda).

 * Improved default values for input and output buffer sizes. The defaults are
   now --input-size=0 and --output-size=0, which will choose recommended
   default sizes based on the --block-size and the operation being performed.
   (dbaarda)

 * Fixed hanging for truncated input files. It will now correctly report an
   error indicating an unexpected EOF was encountered. (dbaarda,
   https://github.com/librsync/librsync/issues/32)

 * Fixed #13 so that faster slack delta's are used for signatures of
   empty files. (dbaarda,
   https://github.com/librsync/librsync/issues/13)

 * Fixed #33 so rs_job_iter() doesn't need calling twice with eof=1.
   Also tidied and optimized it a bit. (dbaarda,
   https://github.com/librsync/librsync/issues/33)

 * Fixed #55 remove excessive rs_fatal() calls, replacing checks for
   programming errors with assert statements. Now rs_fatal() will only
   be called for rare unrecoverable fatal errors like malloc failures or
   impossibly large inputs. (dbaarda,
   https://github.com/librsync/librsync/issues/55)

## librsync 2.0.0

Released 2015-11-29

Note: despite the major version bump, this release has few changes and should
be binary and API compatible with the previous version.

 * Bump librsync version number to 2.0, to match the library
   soname/dylib version.
   (Martin Pool, https://github.com/librsync/librsync/issues/48)

## librsync 1.0.1 (2015-11-21)

 * Better performance on large files. (VictorDenisov)

 * Add comment on usage of rs_build_hash_table(), and assert correct use.
   Callers must call rs_build_hash_table() after loading the signature,
   and before calling rs_delta_begin().
   Thanks to Paul Harris <paulharris@computer.org>

 * Switch from autoconf to CMake.

   Thanks to Adam Schubert.

## librsync 1.0.0 (2015-01-23)

 * SECURITY: CVE-2014-8242: librsync previously used a truncated MD4
   "strong" check sum to match blocks. However, MD4 is not cryptographically
   strong. It's possible that an attacker who can control the contents of one
   part of a file could use it to control other regions of the file, if it's
   transferred using librsync/rdiff. For example this might occur in a
   database, mailbox, or VM image containing some attacker-controlled data.

   To mitigate this issue, signatures will by default be computed with a
   256-bit BLAKE2 hash. Old versions of librsync will complain about a
   bad magic number when given these signature files.

   Backward compatibility can be obtained using the new
   `rdiff sig --hash=md4`
   option or through specifying the "signature magic" in the API, but
   this should not be used when either the old or new file contain
   untrusted data.

   Deltas generated from those signatures will also use BLAKE2 during
   generation, but produce output that can be read by old versions.

   See https://github.com/librsync/librsync/issues/5

   Thanks to Michael Samuel <miknet.net> for reporting this and offering an
   initial patch.

 * Various build fixes, thanks Timothy Gu.

 * Improved rdiff man page from Debian.

 * Improved librsync.spec file for building RPMs.

 * Fixed bug #1110812 'internal error: job made no progress'; on large
   files.

 * Moved hosting to https://github.com/librsync/librsync/

 * Travis-CI.org integration test at https://travis-ci.org/librsync/librsync/

 * You can set `$LIBTOOLIZE` before running `autogen.sh`, for example on
   OS X Homebrew where it is called `glibtoolize`.

## 0.9.7 (released 2004-10-10)

 * Yet more large file support fixes.

 * `extern "C"` guards in librsync.h to let it be used from C++.

 * Removed Debian files from dist tarball.

 * Changed rdiff to an installed program on "make install".

 * Refactored delta calculation code to be cleaner and faster.

 * \#879763: Fixed mdfour to work on little-endian machines which don't
   like unaligned word access.  This should make librsync work on
   pa-risc, and it makes it slightly faster on ia64.

 * \#1022764: Fix corrupted encoding of some COPY commands in large
   files.

 * \#1024881: Print long integers directly, rather than via casts to
   double.

 * Fix printf formats for size_t: both the format and the argument
   should be cast to long.

## 0.9.6

 * Large file support fixes.

 * [v]snprintf or _[v]snprintf autoconf replacement function fix.

 * Changed installed include file from rsync.h to librsync.h.

 * Migration to sourceforge for hosting.

 * Rollsum bugfix that produces much smaller deltas.

 * Memory leaks bugfix patches.

 * mdfour bigendian and >512M bugfix, plus optimisations patch.

 * autoconf/automake updates and cleanups for autoconf 2.53.

 * Windows compilation patch, heavily modified.

 * MacOSX compilation patch, modified to autoconf vararg macro fix.

 * Debian package build scripts patch.

## 0.9.5

 * Bugfix patch from Shirish Hemant Phatak

## 0.9.4: (library 1.1.0)

 * Fixes for rsync.h from Thorsten Schuett <thorsten.schuett@zib.de>

 * RLL encoding fix from Shirish Hemant Phatak <shirish@nustorage.com>

 * RPM spec file by Peter J. Braam <braam@clusterfs.com>

 * No (intentional) changes to binary API.

## 0.9.3

 * Big speed improvements in MD4 routines and generation of weak
   checksums.

 * Patch to build on FreeBSD by Jos Backus <josb@cncdsl.com>

 * Suggestions to build on Solaris 2.6 from Alberto Accomazzi
   <aaccomazzi@cfa.harvard.edu>

 * Add rs_job_drive, a generic mechanism for turning the library into
   blocking mode.  rs_whole_run now builds on top of this.  The
   filebuf interface has changed a little to accomodate it.

 * Generating and loading signatures now generates statistics.

 * More test cases.

 * I suspect there may be a bug in rolling checksums, but it probably
   only causes inefficiency and not corruption.

 * Portability fixes for alphaev67-dec-osf5.1; at the moment builds
   but does not work because librsync tries to do unaligned accesses.

 * Works on sparc64-unknown-linux-gnu (Debian/2.2)

## 0.9.2

 * Improve delta algorithm so that deltas are actually
   delta-compressed, rather than faked.

## 0.9.1

 * Rename the library to `librsync'.

 * Portability fixes.

 * Include the popt library, and use it to build rdiff if the library
   is not present on the host.

 * Add file(1) magic for rdiff.

 * Add more to the manual pages.

 * It's no longer necessary to call rs_buffers_init on a stream before
   starting to use it: all the internal data is kept in the job, not
   in the stream.

 * Rename rs_stream_t to rs_buffers_t, a more obvious name.  Pass the
   buffers to every rs_job_iter() call, rather than setting it at
   startup.  Similarly for all the _begin() functions.

 * rs_job_new also takes the initial state function.

 * Return RS_PARAM_ERROR when library is misused.

## 0.9.0

 * Redesign API to be more like zlib/bzlib.

 * Put all command-line functions into a single rdiff(1) program.

 * New magic number `rs6'

 * Change to using popt for command line parsing.

 * Use Doxygen for API documentation.

## 0.5.7

 * Changes stats string format.

 * Slightly improved test cases

## 0.5.6

 * Don't install debugging tools into /usr/local/bin; leave them in
   the source directory.

 * Fix libhsync to build on (sgi-mips, IRIX64, gcc, GNU Make)

 * Include README.CVS in tarball

 * Back out of using libtool and shared libraries, as it is
   unnecessary at this stage, complicates installation and slows down
   compilation.

 * Use mapptr when reading data to decode, so that decoding should
   have less latency and be more reliable.

 * Cope better on systems that are missing functions like snprintf.

## 0.5.5

 * Put genuine search encoding back into the nad algorithm, and
   further clean up the nad code.  Literals are now sent out using a
   literal buffer integrated with the input mapptr so that data is not
   copied.  Checksums are still calculated from scratch each time
   rather than by rolling -- this is very slow but simple.

 * Reshuffle test cases so that they use files generated by hsmapread,
   rather than the source directory.  This makes the tests quicker and
   more reproducible, hopefully without losing coverage.  Further
   develop the test driver framework.

 * Add hsdumpsums debugging tool.

 * Hex strings (eg strong checksums) are broken up by underscores for
   readability.

 * Stats now go to the log rather than stdout.

 * mapptr acts properly when we're skipping/rewinding to data already
   present in the buffer -- it does a copy if required, but not
   necessarily real IO.

## 0.5.4

 * Improved mapptr input code

 * Turn on more warnings if using gcc

 * More test cases

## 0.5.3

 * Improvements to mapptr to make it work better for network IO.

 * Debug trace code is compiled in unless turned off in ./configure
   (although most programs will not write it out unless asked.)

 * Add libhsyncinfo program to show compiled-in settings and version.

 * Add test cases that run across localhost TCP sockets.

 * Improved build code; should now build easily from CVS through
   autogen.sh.

 * Improved trace code.

 * Clean up to build on sparc-sun-solaris2.8, and in the process clean
   up the handling of bytes vs chars, and of building without gcc

 * Reverse build scripts so that driver.sh calls the particular
   script.

## 0.5.2

 * Use mapptr for input.

 * Implement a new structure for encoding in nad.c.  It doesn't
   encode at the moment, but it's much more maintainable.

 * More regression cases.

 * Clean up build process.

## 0.5.0

 * Rewrite hs_inbuf and hs_encode to make them simpler and more
   reliable.

 * Test cases for input handling.

 * Use the map_ptr idea for input from both streams and files.

## 0.4.1

 * automake/autoconf now works cleanly when the build directory is
   different to the source directory.

 * --enable-ccmalloc works again.

## 0.4.0

* A much better regression suite.

* CHECKSUM token includes the file's checksum up to the current
  location, to aid in self-testing.

* Various bug fixes, particularly to do with short IO returns.

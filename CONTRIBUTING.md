# Contributing to librsync {#page_contributing}

Instructions and conventions for people wanting to work on librsync.  Please
consider these guidelines even if you're doing your own fork.

## Requirements

There are a bunch of tools and libraries that are useful for development or
that librsync depends on. In addition to the standard cmake/gcc/make/etc C
development tools, the following packages are recommended;

* libpopt-dev - the cmdline argument parsing library used by rdiff. If this is
  not available rdiff cannot be compiled, and only the librsync library will
  be compiled.

* libb2-dev - blake2 hash library librsync can use if `USE_LIBB2` is set to
  `ON` in cmake. If this is not avalable librsync will use its included copy
  of blake2 (which may be older... or newer).

* doxygen - documentation generator used to generate html documentation. If
  installed `make doc` can be run to generate all the docs.

* graphviz - graph generator used by doxygen for generating diagrams. If not
  installed doxygen will not generate any diagrams.

* indent - code reformatter for tidying code. If installed all the code can be
  tidied with `make tidy`.

* [tidyc](https://github.com/dbaarda/tidyc) - extension wrapper for indent
  that includes better formatting of doxygen comments. If installed code and
  comments can be tidied with `make tidyc`.

* clang-tidy - code lint checker for potential problems. If installed the code
  can be checked with `make clang-tidy`.

* iwyu - `#include` checker and fixer. If installed includes can be checked
  with `make iwyu`, and automatically fixed with `make iwyu-fix`. Note on some
  platforms this package is [missing a
  dependency](https://bugs.launchpad.net/ubuntu/+source/iwyu/+bug/1769334) and
  also needs `libclang-common-9-dev` to be installed.

These can all be installed on Debian/Ubuntu systems by running;

```Shell
$ apt-get update
$ apt-get install libpopt-dev libb2-dev doxygen graphviz indent clang-tidy iwyu
$ git clone https://github.com/dbaarda/tidyc.git
$ cp tidyc/tidyc ~/bin
```

### Windows

Not all the recommended packages are easily available on windows.
[Cygwin](https://cygwin.com/) and [MSYS2](http://msys2.org/) provide a
development environment similar to Linux. Some packages can also be found on
[Chocolatey](https://chocolatey.org/). For native development using standard
MSVC tools, libpopt can be found on [vcpkg](https://vcpkg.io/) and installed
by running;

```Shell
$ vcpkg update
$ vcpkg --triplet x64-windows install libpopt
```

For cmake to find the installed libpopt you need to add `-D
CMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake` to the cmake
cmdline. This configures cmake to correctly search the vcpkg install locations
to find libraries.

### MacOS

MacOS is generally more similar to Linux than Windows, and several packages
are available on homebrew. The libpopt library can be installed by running;

```Shell
$ brew update
$ brew install popt
```

## Building

The minimal instructions to fetch, configure, compile, and test everything
using a in-place default Debug build with trace enabled using the internal
blake2 implementation is;

```Shell
$ git clone https://github.com/librsync/librsync.git
$ cd librsync
$ cmake .
$ make check
```

For cmake, `-B` can be used to select a separate build directory, and `-G` can
select a different make system. Also the following settings can be changed
with `-D <setting>=<value>` arguments when generating the project with cmake;

* CMAKE_BUILD_TYPE=(Debug|Release|MinSizeRel|RelWithDebInfo) - the build type
  to use, which selects compiler options. The default is `Debug`.

* CMAKE_C_COMPILER=(cc|gcc|clang|...) - The compiler to use. The default is to
  auto-detect the available compiler on the platform.

* BUILD_RDIFF=(ON|OFF) - Whether to build and test the rdiff executable.
  Defaults to ON if libpopt is available.

* BUILD_SHARED_LIBS=(ON|OFF) - Whether to build dynamic libraries or use
  static linking. Defaults to ON.

* ENABLE_TRACE=(ON|OFF) - Whether to enable trace output in the library and
  for rdiff using `-v`. Trace output can help with debugging but its a little
  faster with ENABLE_TRACE=OFF. Defaults to ON for Debug builds, and OFF for
  others.

* USE_LIBB2=(ON|OFF) - Whether to use libb2 instead of the included blake2
  implementation. Defaults to OFF.

So for a Release build in a separate directory using Ninja, clang, static
linking, and libb2 with trace enabled, do this instead;

```Shell
$ cmake -B build -G Ninja
  -D CMAKE_C_COMPILER=clang \
  -D CMAKE_BUILD_TYPE=Release \
  -D BUILD_SHARED_LIBS=OFF \
  -D USE_LIBB2=ON \
  -D ENABLE_TRACE=ON
$ cmake --build build --config Release --target check
```

You can also use ccmake or cmake-gui to interactively configure and generate
into a separate build directory with;

```Shell
$ ccmake -B build
```

## Coding

The prefered style for code is equivalent to using GNU indent with the
following arguments;

```Shell
$ indent -linux -nut -i4 -ppi2 -l80 -lc80 -fc1 -sob -T FILE -T Rollsum -T rs_result
```

The preferred style for non-docbook comments are as follows;

```C

                         /*=
                          | A short poem that
                          | shall never ever be
                          | reformated or reindented
                          */

    /* Single line comment indented to match code indenting. */

    /* Blank line delimited paragraph multi-line comments.

       Without leading stars, or blank line comment delimiters. */

    int a;                      /* code line comments */
```

The preferred style for docbook comments is javadoc with autobrief as
follows;

```C
/** /file file.c
  * Brief summary paragraph.
  *
  * With blank line paragraph delimiters and leading stars.
  *
  * /param foo parameter descriptions...
  *
  * /param bar ...in separate blank-line delimited paragraphs.
  *
  * Example:/code
  *  code blocks that will never be reformated.
  * /endcode
  *
  * Without blank-line comment delimiters. */

    int a;                      /**< brief attribute description */
    int b;                      /**< multiline attribute description
                                 *
                                 * With blank line delimited paragraphs.*/
```

There is a `make tidy` target that will use GNU indent to reformat all code
and non-docbook comments, doing some pre/post processing with sed to handle
some corner cases indent doesn't handle well.

There is a `make tidyc` target that will reformat all code and comments with
[tidyc](https://github.com/dbaarda/tidyc). This will also correctly reformat
all docbook comments, equivalent to running tidyc with the following
arguments;

```Shell
$ tidyc -R -C -l80 -T FILE -T Rollsum -T rs_result
```

There is `make clang-tidy` and `make iwyu` targets for checking for coding
errors and incorrect `#include` statements. Note that the iwyu check gets
confused by and will emit warnings about `fileutil.c` which has extra
conditional includes necessary to find working functions on various platforms.
Other than `fileutil.c` both checks should be clean.

If iwyu finds problems, `make ifwu-fix` can be run to automatically fix them,
followed by `make tidyc` to reformat the result to our preferred style. Note
that this doesn't always produce an ideal result and may require manual
intervention.

Please try to update docs and tests in parallel with code changes.

## Testing

Using `make check` will compile and run all tests. Additional code correctness
checks can be run with `make clang-tidy` and `make iwyu`.

Note that `assert()` is used extensively within the code for verifying the
correctness of internal operations using a roughly design-by-contract
approach. These are only enabled for Debug builds, so testing with a Debug
build will give a better chance of identifying problems during development.
Once you are confident the code is correct, a Release build will turn these
off giving faster execution.

There are also GitHub Actions configured for the librsync project to
configure, build, test, and lint everything on a variety of different
platforms with a variety of different settings. These are run against any pull
request or commit, and are a good way to check things are not broken for other
platforms.

Test results for builds of public github branches are at
https://github.com/librsync/librsync/actions.

## Documenting

[NEWS.md](NEWS.md) contains a list of user-visible changes in the library
between release versions. This includes changes to the way it's packaged, bug
fixes, portability notes, changes to the API, and so on. Add and update items
under a "Changes in X.Y.Z" heading at the top of the file. Do this as you go
along, so that we don't need to work out what happened when it's time for a
release.

[TODO.md](TODO.md) contains a list of ideas and proposals for the future.
Ideally entries should be formated in a way that can be just moved into
[NEWS.md](NEWS.md) when they are done. Regularly check to see if there is
anything that needs removing or updating.

## Submitting

Fixes or improvements in pull requests are welcome.  Please:

- [ ] Send small PRs that address one issues each.

- [ ] Update `NEWS.md` to say what you changed.

- [ ] Add a test as a self-contained C file in `tests/` that passes or fails,
  and is hooked into `CMakeLists.txt`.

- [ ] Keep the code style consistent with what's already there, especially in
  keeping symbols with an `rs_` prefix.

## Releasing

If you are making a new tarball release of librsync, follow this checklist:

* NEWS.md - make sure the top "Changes in X.Y.Z" is correct, and the date is
  correct. Make sure the changes since the last release are documented.

* TODO.md - check if anything needs to be removed or updated.

* `CMakeLists.txt` - version is correct.

* `librsync.spec` - make sure version and URL are right.

* Run `make all doc check` in a clean checkout of the release tag. Also check
  the GitHub Actions [check and lint
  status](https://github.com/librsync/librsync/actions) of the last commit on
  github.

* Draft a new release on github, typing in the release details including an
  overview, included changes, and known issues. The overview should give an
  indication of the magnitude of the changes and their impact, and the
  relative urgency to upgrade. The included changes should come from the
  NEWS.md for the release. It's probably easiest to copy and edit the previous
  release.

* After creating the release, download the tar.gz version, edit the release,
  and re-upload it. This ensures that the release includes a stable tarball
  (See https://github.com/librsync/librsync/issues/146 for details).

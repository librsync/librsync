# File formats {#page_formats}

## Generalities

There are two file formats used by `librsync` and `rdiff`: the
*signature* file, which summarizes a data file, and the *delta* file,
which describes the edits from one data file to another.

librsync does not know or care about any formats in the data files.

All integers are big-endian.

## Magic numbers

All librsync files start with a u32 \ref rs_magic_number identifying them.
These are declared in `librsync.h`, and there are different numbers for every
different signature and delta file type. Note magic numbers for newer file
types are not supported by older versions of librsync. Older librsync versions
will immediately fail with an error when they encounter file types they don't
support.

## Signatures

Signatures consist of a header followed by a number of block signatures for
each block in the data file.

The signature header is:

    u32 magic;     // Some RS_*_SIG_MAGIC value.
    u32 block_len; // Bytes per block.
    u32 strong_sum_len;  // Bytes per strong sum in each block.

Each block signature includes a weaksum followed by a truncated strongsum hash
for one block of `block_len` bytes from the input data file. The strongsum
signature will be truncated to the `strong_sum_len` in the header. The final
data block may be shorter. The number of blocks in the signature is therefore

    ceil(input_len/block_len)

The block signature weak checksum is used as a rolling checksum to find moved
data, and a strong hash used to check the match is correct. The weak checksum
is either a rollsum (based on adler32) or (better alternative) rabinkarp, and
the strong hash is either MD4 or BLAKE2 depending on the magic number.

Truncating the strongsum makes the signatures smaller at a cost of a greater
chance of collisions.  The strongsums are truncated by keeping the left most
(first) bytes after computation.

Each signature block format is (see `rs_sig_do_block`):

    u32 weak_sum;
    u8[strong_sum_len] strong_sum;

## Delta files

Deltas consist of the delta magic constant `RS_DELTA_MAGIC` followed by a
series of commands. Commands tell the patch logic how to construct the result
file (new version) from the basis file (old version).

There are three kinds of commands: the literal command, the copy command, and
the end command. A command consists of a single byte followed by zero or more
arguments. The number and size of the arguments are defined in `prototab.c`.

A literal command describes data not present in the basis file. It has one
argument: `length`. The format is:

    u8 command; // in the range 0x41 through 0x44 inclusive
    u8[arg1_len] length;
    u8[length] data; // new data to append

A copy command describes a range of data in the basis file. It has two
arguments: `start` and `length`. The format is:

    u8 command; // in the range 0x45 through 0x54 inclusive
    u8[arg1_len] start; // offset in the basis to begin copying data
    u8[arg2_len] length; // number of bytes to copy from the basis

The end command indicates the end of the delta file. It consists of a single
null byte and has no arguments.

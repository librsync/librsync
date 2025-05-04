# Stream API example

This example shows how to setup a simple client- & server application to
transfer a single file. The intension is for developers to quickly understand
how the Streaming API works.

## Prerequisites

The library `librsync` needs to be installed.

## Building the example project

```
cd examples/stream
cmake .
make
```

## Running the example project

Open two terminal sessions in the example build directory. One for the server
and one for the client.

### Run server

Swap `<FILENAME>` with the copy you want to generate delta from:

```
./server <FILENAME>
```

Use `-` for `stdin`. E.g.:

```
echo "foo bar baz" | ./server -
```

### Run client

Swap `<FILENAME>` with the copy you want to generate signature from:

```
./client <FILENAME>
```

Use `-` for `stdin`. E.g.:

```
echo "foo baz bar" | ./client -
```

The patched version of the file will be available in `<FILENAME>` or printed to
`stdout` if `-` was used.

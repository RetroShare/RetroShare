# libsam3

[![Build Status](https://travis-ci.org/i2p/libsam3.svg?branch=master)](https://travis-ci.org/i2p/libsam3)

A C library for the [SAM v3 API](https://geti2p.net/en/docs/api/samv3).

## Development Status

Maintained by idk, PRs are accepted on [I2P gitlab](https://i2pgit.org/i2p-hackers/libsam3)/[I2P gitlab](http://git.idk.i2p/i2p-hackers/libsam3), and on github at the official mirror repository: [i2p/libsam3](https://github.com/i2p/libsam3).

## Usage

Copy the two files from one of the following locations into your codebase:

- `src/libsam3` - Synchronous implementation.
- `src/libsam3a` - Asynchronous implementation.

See `examples/` for how to use various parts of the API.

## Cross-Compiling for Windows from debian:

Set your cross-compiler up:

``` sh
export CC=x86_64-w64-mingw32-gcc
export CFLAGS='-Wall -O2 '
export LDFLAGS='-lmingw32 -lws2_32 -lwsock32 -mwindows'
```

and run `make build`. Only libsam3 is available for Windows, libsam3a will be
made available at a later date.
`

## Linker(Windows)

When building for Windows remember to set the flags to link to the Winsock and Windows
libraries.

`-lmingw32 -lws2_32 -lwsock32 -mwindows`

This may apply when cross-compiling or compiling from Windows with mingw.

## Cool Projects using libsam3

Are you using libsam3 to provide an a cool I2P based feature to your project? Let us know about it(and how
it uses libsam3) and we'll think about adding it here*!

 1. [Retroshare](https://retroshare.cc)

*Projects which are listed here must be actively maintained. Those which intentionally violate
the law or the rights of a person or persons directly won't be considered. Neither will obvious
trolling. The maintainer will make the final decision.

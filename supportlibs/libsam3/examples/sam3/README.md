Examples
========

These examples show various ways of using libsam3 to enable i2p in your
application, and are also useful in other ways. If you implement an i2p
application library in another language, making variants basic tools wouldn't be
the worst way to make sure that it works.

building
--------

Once you have build the library in the root of this repository by running make
all, you can build all these examples at once by running

        make

in this directory. I think it makes things easier to experiment with quickly.

namelookup
----------

Namelookup uses the SAM API to find the base64 destination of an readable "jump"
or base32 i2p address. You can use it like this:

        ./lookup i2p-projekt.i2p


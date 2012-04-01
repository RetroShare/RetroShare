To use this branch:

   chekcout the last version of openpgp SDK:
      # svn co svn://openpgp.nominet.org.uk/openpgpsdk/tags/openpgpsdk-0.9 openpgpsdk
      # cd openpgpsdk
      # ./configure --without-idea
      # make

   For the moment, the compilation is not workign on ubuntu 

Work to do
==========
Put a 'x' when done. 1,2,3 means started/ongoing/almost finished.

Compilation
  00   [1] make sure the library compiles on linux
  01   [ ] make sure the library compiles on windows

Project
  02   [1] determine what's missing in OpenPGP-SDK
  03   [1] make a separate layer in RS to handle PGP. AuthPGP is too close to libretroshare.

Notes
=====
   Questions to answer:
     - do we rely on updates from openPGP-sdk ? Probably not. This code seems frozen.
     - do we need an abstract layer for PGP handling in RS ?
     - what new functionalities do we need in RS ?
          * pgp keyring sharing/import/export
          * identity import/export

   Code struture
     - replace current AuthGPG (virtual class) by a class named GPGHandler,
        that is responsible for signing, checking signatures, encrypting etc.
     - add a specific 8-bytes type for GPG Ids. Could be a uint64_t, or a
        uchar[8]



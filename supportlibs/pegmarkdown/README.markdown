
This a forked version of peg-markdown.... only minor changes:

 * Switch to Qt .pro make system for easy x-platform.

 * build a library instead of exec.

 * Add GLibFacade from multimarkdown to allow Win32 compilations.

 * Added ifdefs for C++ linking,


What is this?
=============

This is an implementation of John Gruber's [markdown][] in C. It uses a
[parsing expression grammar (PEG)][] to define the syntax. This should
allow easy modification and extension. It currently supports output in
HTML, LaTeX, ODF, or groff_mm formats, and adding new formats is
relatively easy.

[parsing expression grammar (PEG)]: http://en.wikipedia.org/wiki/Parsing_expression_grammar 
[markdown]: http://daringfireball.net/projects/markdown/

It is pretty fast. A 179K text file that takes 5.7 seconds for
Markdown.pl (v. 1.0.1) to parse takes less than 0.2 seconds for this
markdown. It does, however, use a lot of memory (up to 4M of heap space
while parsing the 179K file, and up to 80K for a 4K file). (Note that
the memory leaks in earlier versions of this program have now been
plugged.)

Both a library and a standalone program are provided.

peg-markdown is written and maintained by John MacFarlane (jgm on
github), with significant contributions by Ryan Tomayko (rtomayko).
It is released under both the GPL and the MIT license; see LICENSE for
details.

Installing
==========

On a linux or unix-based system
-------------------------------

This program is written in portable ANSI C. It requires
[glib2](http://www.gtk.org/download/index.php). Most *nix systems will have
this installed already. The build system requires GNU make.

The other required dependency, [Ian Piumarta's peg/leg PEG parser
generator](http://piumarta.com/software/peg/), is included in the source
directory. It will be built automatically. (However, it is not as portable
as peg-markdown itself, and seems to require gcc.)

To make the 'markdown' executable:

    make

(Or, on some systems, `gmake`.) Then, for usage instructions:

    ./markdown --help

To run John Gruber's Markdown 1.0.3 test suite:

    make test

The test suite will fail on one of the list tests.  Here's why.
Markdown.pl encloses "item one" in the following list in `<p>` tags:

    1.  item one
        * subitem
        * subitem
    
    2.  item two

    3.  item three

peg-markdown does not enclose "item one" in `<p>` tags unless it has a
following blank line. This is consistent with the official markdown
syntax description, and lets the author of the document choose whether
`<p>` tags are desired.

Cross-compiling for Windows with MinGW on a linux box
-----------------------------------------------------

Prerequisites:

*   Linux system with MinGW cross compiler For Ubuntu:

        sudo apt-get install mingw32

*   [Windows glib-2.0 binary & development files](http://www.gtk.org/download-windows.html).
    Unzip files into cross-compiler directory tree (e.g., `/usr/i586-mingw32msvc`).

Steps:

1.  Create the markdown parser using Linux-compiled `leg` from peg-0.1.4:

        ./peg-0.1.4/leg markdown_parser.leg >markdown_parser.c

    (Note: The same thing could be accomplished by cross-compiling leg,
    executing it on Windows, and copying the resulting C file to the Linux
    cross-compiler host.)

2.  Run the cross compiler with include flag for the Windows glib-2.0 headers:
    for example,

        /usr/bin/i586-mingw32msvc-cc -c \
        -I/usr/i586-mingw32msvc/include/glib-2.0 \
        -I/usr/i586-mingw32msvc/lib/glib-2.0/include -Wall -O3 -ansi markdown*.c

3.  Link against Windows glib-2.0 headers: for example,

        /usr/bin/i586-mingw32msvc-cc markdown*.o \
        -Wl,-L/usr/i586-mingw32msvc/lib/glib,--dy,--warn-unresolved-symbols,-lglib-2.0 \
        -o markdown.exe

The resulting executable depends on the glib dll file, so be sure to
load the glib binary on the Windows host.

Compiling with MinGW on Windows
-------------------------------

These directions assume that MinGW is installed in `c:\MinGW` and glib-2.0
is installed in the MinGW directory hierarchy (with the mingw bin directory
in the system path).

Unzip peg-markdown in a temp directory. From the directory with the
peg-markdown source, execute:

    cd peg-0.1.4
    make PKG_CONFIG=c:/path/to/glib/bin/pkg-config.exe

Extensions
==========

peg-markdown supports extensions to standard markdown syntax.
These can be turned on using the command line flag `-x` or
`--extensions`.  `-x` by itself turns on all extensions.  Extensions
can also be turned on selectively, using individual command-line
options. To see the available extensions:

    ./markdown --help-extensions
 
The `--smart` extension provides "smart quotes", dashes, and ellipses.

The `--notes` extension provides a footnote syntax like that of
Pandoc or PHP Markdown Extra.

Using the library
=================

The library exports two functions:

    GString * markdown_to_g_string(char *text, int extensions, int output_format);
    char * markdown_to_string(char *text, int extensions, int output_format);

The only difference between these is that `markdown_to_g_string` returns a
`GString` (glib's automatically resizable string), while `markdown_to_string`
returns a regular character pointer.  The memory allocated for these must be
freed by the calling program, using `g_string_free()` or `free()`.

`text` is the markdown-formatted text to be converted.  Note that tabs will
be converted to spaces, using a four-space tab stop.  Character encodings are
ignored.

`extensions` is a bit-field specifying which syntax extensions should be used.
If `extensions` is 0, no extensions will be used.  If it is `0xFFFFFF`,
all extensions will be used.  To set extensions selectively, use the
bitwise `&` operator and the following constants:

 - `EXT_SMART` turns on smart quotes, dashes, and ellipses.
 - `EXT_NOTES` turns on footnote syntax.  [Pandoc's footnote syntax][] is used here.
 - `EXT_FILTER_HTML` filters out raw HTML (except for styles).
 - `EXT_FILTER_STYLES` filters out styles in HTML.

  [Pandoc's footnote syntax]: http://johnmacfarlane.net/pandoc/README.html#footnotes

`output_format` is either `HTML_FORMAT`, `LATEX_FORMAT`, `ODF_FORMAT`,
or `GROFF_MM_FORMAT`.

To use the library, include `markdown_lib.h`.  See `markdown.c` for an example.

Hacking
=======

It should be pretty easy to modify the program to produce other formats,
and to parse syntax extensions.  A quick guide:

  * `markdown_parser.leg` contains the grammar itself.

  * `markdown_output.c` contains functions for printing the `Element`
    structure in various output formats.

  * To add an output format, add the format to `markdown_formats` in
    `markdown_lib.h`.  Then modify `print_element` in `markdown_output.c`,
    and add functions `print_XXXX_string`, `print_XXXX_element`, and
    `print_XXXX_element_list`. Also add an option in the main program
    that selects the new format. Don't forget to add it to the list of
    formats in the usage message.

  * To add syntax extensions, define them in the PEG grammar
    (`markdown_parser.leg`), using existing extensions as a guide. New
    inline elements will need to be added to `Inline =`; new block
    elements will need to be added to `Block =`. (Note: the order
    of the alternatives does matter in PEG grammars.)

  * If you need to add new types of elements, modify the `keys`
    enum in `markdown_peg.h`.

  * By using `&{ }` rules one can selectively disable extensions
    depending on command-line options. For example,
    `&{ extension(EXT_SMART) }` succeeds only if the `EXT_SMART` bit
    of the global `syntax_extensions` is set. Add your option to
    `markdown_extensions` in `markdown_lib.h`, and add an option in
    `markdown.c` to turn on your extension.

  * Note: Avoid using `[^abc]` character classes in the grammar, because
    they cause problems with non-ascii input. Instead, use: `( !'a' !'b'
    !'c' . )`

Acknowledgements
================

Support for ODF output was added by Fletcher T. Penney.


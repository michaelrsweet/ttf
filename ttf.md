Introduction
============

TTF is a simple C library for using TrueType and OpenType font files.


Using the TTF Library
----------------------

The TTF library has a single header file:

```c
#include <ttf.h>
```

Use the `pkg-config` command to get the proper compiler and linker options:

```
CFLAGS = `pkg-config --cflags ttf`
LIBS = `pkg-config --libs ttf`

cc -o myprogram `pkg-config --cflags ttf` myprogram.c `pkg-config --libs ttf`
```

The [`ttfCreate`](@@) function opens a font file, returning a pointer to a
`ttf_t` object:

```c
ttf_t *font = ttfCreate("FILENAME.ttf", /*idx*/0, /*err_cb*/NULL, /*err_data*/NULL);
```

The returned pointer can be used with the other TTF library functions to provide
various bits of metadata and measure the bounds of a given string of Unicode
text.  Once you are done with the font data, use the [`ttfDelete`](@@) function
to free the memory that was used.

If you already have a font file loaded in memory, the [`ttfCreateData`](@@)
function can be used to create a `ttf_t` object from the buffer:

```c
ttf_t *font = ttfCreateData(data, datasize, /*idx*/0, /*err_cb*/NULL, /*err_data*/NULL);
```

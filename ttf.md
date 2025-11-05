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


Opening Fonts
-------------

The [`ttfCreate`](@@) function opens a font file, returning a pointer to a
`ttf_t` object:

```c
ttf_t *font = ttfCreate("FILENAME.ttf", /*idx*/0, /*err_cb*/NULL, /*err_cbdata*/NULL);
```

The returned pointer can be used with the other TTF library functions to provide
various bits of metadata and measure the bounds of a given string of Unicode
text.  Once you are done with the font data, use the [`ttfDelete`](@@) function
to free the memory that was used.

If you already have a font file loaded in memory, the [`ttfCreateData`](@@)
function can be used to create a `ttf_t` object from the buffer:

```c
ttf_t *font = ttfCreateData(data, datasize, /*idx*/0, /*err_cb*/NULL, /*err_cbdata*/NULL);
```

The error callback function ("err_cb") and data ("err_cbdata") allow you to
specify a function that will receive any error messages that are ordinarily
displayed to the standard error file.  The callback receives the data pointer
and a message string, for example the following callback will display the error
message to the standard output file instead:

```c
void
my_error_cb(void *err_cbdata, const char *message)
{
  (void)err_cbdata;

  puts(message);
}
```


Accessing System and User Fonts
-------------------------------

TTF also provides a `ttf_cache_t` object representing a cache of available
fonts.  The [`ttfCacheCreate`] function creates a cache of fonts that are
provided by the operating system and/or installed by the user:

```c
ttf_cache_t *cache = ttfCacheCreate("my-application-name", /*err_cb*/NULL, /*err_cbdata*/NULL);
```

The error callback function and data are the same as you would provide to the
[`ttfCreate`](@@) or [`ttfCreateData`](@@) functions.

Once loaded, you can access the available fonts using the [`ttfCacheFind`](@@)
or [`ttfCacheGetFont`](@@) functions, for example:

```c
// Find Helvetica
ttf_t *font = ttfCacheFind(cache, "Helvetica", TTF_STYLE_NORMAL, TTF_WEIGHT_400, TTF_STRETCH_NORMAL);

// List all fonts
size_t i, num_fonts = ttfCacheGetNumFonts(cache);

for (i = 0; i < num_fonts; i ++)
{
  font = ttfCacheGetFont(cache, i);

  puts(ttfGetFamily(font));
}
```

> **Note:** Do not call [`ttfDelete`](@@) on fonts obtained from the cache.
> Instead, call [`ttfCacheDelete`](@@) to free all fonts in the cache.

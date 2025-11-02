//
// Font caching code for TTF library
//
//     https://github.com/michaelrsweet/ttf
//
// Copyright © 2025 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "ttf.h"


//
// Types...
//

typedef struct _ttf_centry_s		// Font cache entry
{
  char          *filename,              // Filename
                *family;                // Family
  off_t         filesize;               // File size
  time_t        filemtime;              // File modification date/time
  size_t        idx;                    // Index inside collection
  ttf_stretch_t stretch;                // Stretch
  ttf_style_t   style;                  // Style
  ttf_variant_t variant;                // Variant
  ttf_weight_t  weight;                 // Weight
} _ttf_centry_t;


struct _ttf_cache_s			// Font cache
{
  size_t	num_entries,		// Number of cache entries
		alloc_entries;		// Number of allocated entries
  _ttf_centry_t	*entries;		// Cache entries
};


//
// 'ttfCacheDelete()' - Delete a cache.
//

void
ttfCacheDelete(ttf_cache_t *tc)		// I - Font cache
{
  if (tc)
  {
    size_t	i;			// Looping var
    _ttf_centry_t *c;			// Current cache entry

    for (i = tc->num_entries, c = tc->entries; i > 0; i --, c ++)
    {
      free(c->filename);
      free(c->family);
    }

    free(tc->entries);
    free(tc);
  }
}


//
// 'ttfCacheFind()' - Find a font in the cache.
//

ttf_t *					// O - Font or `NULL` if no matching font found
ttfCacheFind(ttf_cache_t   *tc,		// I - Font cache
             const char    *family,	// I - Family name
             ttf_style_t   style,	// I - Font style or `TTF_STYLE_UNSPEC`
             ttf_weight_t  weight,	// I - Font weight or `TTF_WEIGHT_UNSPEC`
             ttf_stretch_t stretch,	// I - Font stretch or `TTF_STRETCH_UNSPEC`
             ttf_variant_t variant)	// I - Font variant or `TTF_VARIANT_UNSPEC`
{
}


//
// 'ttfCacheGetFilename()' - Get the font filename  at index N.
//

const char *
ttfCacheGetFilename(ttf_cache_t *tc,	// I - Font cache
                    size_t      n)	// I - Font index starting at `0`
{
}


//
// 'ttfCacheGetFamily()' - Get the font family at index N.
//

const char *
ttfCacheGetFamily(ttf_cache_t *tc,	// I - Font cache
		  size_t      n)	// I - Font index starting at `0`
{
}



//
// 'ttfCacheGetStretch()' - Get the font stretch at index N.
//

ttf_stretch_t
ttfCacheGetStretch(ttf_cache_t *tc,	// I - Font cache
                    size_t      n)	// I - Font index starting at `0`
{
}


//
// 'ttfCacheGetStyle()' - Get the font style at index N.
//

ttf_style_t
ttfCacheGetStyle(ttf_cache_t *tc,	// I - Font cache
		 size_t      n)		// I - Font index starting at `0`
{
}


//
// 'ttfCacheGetVariant()' - Get the font variant at index N.
//

ttf_variant_t
ttfCacheGetVariant(ttf_cache_t *tc,	// I - Font cache
		   size_t      n)	// I - Font index starting at `0`
{
}


//
// 'ttfCacheGetWeight()' - Get the font weight at index N.
//

ttf_weight_t
ttfCacheGetWeight(ttf_cache_t *tc,	// I - Font cache
		  size_t      n)	// I - Font index starting at `0`
{
}


//
// 'ttfCacheGetFont()' - Get the font at index N
//

ttf_t *
ttfCacheGetFont(ttf_cache_t *tc,	// I - Font cache
		size_t      n)		// I - Font index starting at `0`
{
}


//
// 'ttfCacheGetNumFonts()' - Get the number of fonts in the cache.
//

size_t					// O - Number of fonts
ttfCacheGetNumFonts(ttf_cache_t *tc)	// I - Font cache
{
}


//
// 'ttfCacheNew()' - Create a cache of system and user fonts.
//
// This function creates a cache of system and user fonts, loading any
// previously cached results for the named application.
//

ttf_cache_t *				// O - Font cache
ttfCacheNew(const char *appname)	// I - Application name
{
}

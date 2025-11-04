//
// Font caching code for TTF library
//
//     https://github.com/michaelrsweet/ttf
//
// Copyright © 2019-2025 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "ttf-private.h"


//
// Types...
//

typedef struct _ttf_centry_s		// Font cache entry
{
  ttf_t		*font;			// Font, if loaded
  char          *filename,              // Filename or URL
                *family;                // Family
  off_t         filesize;               // File size
  time_t        filemtime;              // File modification date/time
  size_t        idx;                    // Index inside collection
  ttf_stretch_t stretch;                // Stretch
  ttf_style_t   style;                  // Style
  ttf_weight_t  weight;                 // Weight
} _ttf_centry_t;


struct _ttf_cache_s			// Font cache
{
  char		*appname;		// Application name
  bool		changed;		// Has the cache been changed?
  size_t	num_entries,		// Number of cache entries
		alloc_entries;		// Number of allocated entries
  _ttf_centry_t	*entries;		// Cache entries
};


//
// Local functions...
//

static void	ttf_add_font(ttf_cache_t *cache, ttf_t *font, const char *filename, struct stat *fileinfo, bool delete_it);
static int	ttf_compare_entries(_ttf_centry_t *a, _ttf_centry_t *b);
static void	ttf_load_all_fonts(ttf_cache_t *cache);
static bool	ttf_load_cache(ttf_cache_t *cache, struct stat *cinfo);
static time_t	ttf_load_fonts(ttf_cache_t *cache, const char *d, bool scanonly);
static void	ttf_save_cache(ttf_cache_t *cache);
static void	ttf_sort_fonts(ttf_cache_t *cache);


//
// 'ttfCacheAdd()' - Add a font to the cache.
//

void
ttfCacheAdd(ttf_cache_t *cache,		// I - Font cache
            ttf_t       *font,		// I - Font
            const char  *filename)	// I - Filename/URL or `NULL` for in-memory
{
  struct stat	fileinfo;		// File information


  // Range check input...
  if (!cache || !font)
    return;

  // Add the font and re-sort...
  if (filename && *filename == '/' && !stat(filename, &fileinfo))
    ttf_add_font(cache, font, filename, &fileinfo, /*delete_it*/false);
  else
    ttf_add_font(cache, font, filename, /*fileinfo*/NULL, /*delete_it*/false);

  ttf_sort_fonts(cache);
}


//
// 'ttfCacheDelete()' - Delete a cache.
//

void
ttfCacheDelete(ttf_cache_t *cache)		// I - Font cache
{
  if (cache)
  {
    size_t	i;			// Looping var
    _ttf_centry_t *entry;		// Current cache entry

    for (i = cache->num_entries, entry = cache->entries; i > 0; i --, entry ++)
    {
      ttfDelete(entry->font);
      free(entry->filename);
      free(entry->family);
    }

    free(cache->entries);
    free(cache);
  }
}


//
// 'ttfCacheFind()' - Find a font in the cache.
//

ttf_t *					// O - Font or `NULL` if no matching font found
ttfCacheFind(ttf_cache_t   *cache,	// I - Font cache
             const char    *family,	// I - Family name
             ttf_style_t   style,	// I - Font style or `TTF_STYLE_UNSPEC`
             ttf_weight_t  weight,	// I - Font weight or `TTF_WEIGHT_UNSPEC`
             ttf_stretch_t stretch)	// I - Font stretch or `TTF_STRETCH_UNSPEC`
{
  (void)cache;
  (void)family;
  (void)style;
  (void)weight;
  (void)stretch;

  return (NULL);
}


//
// 'ttfCacheGetFilename()' - Get the font filename  at index N.
//

const char *				// O - Font filename/URL or `NULL` if none
ttfCacheGetFilename(ttf_cache_t *cache,	// I - Font cache
                    size_t      n)	// I - Font index starting at `0`
{
  return ((cache && n < cache->num_entries) ? cache->entries[n].filename : NULL);
}


//
// 'ttfCacheGetFamily()' - Get the font family at index N.
//

const char *				// O - Font family name or `NULL`
ttfCacheGetFamily(ttf_cache_t *cache,	// I - Font cache
		  size_t      n)	// I - Font index starting at `0`
{
  return ((cache && n < cache->num_entries) ? cache->entries[n].family : NULL);
}



//
// 'ttfCacheGetStretch()' - Get the font stretch at index N.
//

ttf_stretch_t				// O - Font stretch or `TTF_STRETCH_UNSPEC`
ttfCacheGetStretch(ttf_cache_t *cache,	// I - Font cache
                    size_t      n)	// I - Font index starting at `0`
{
  return ((cache && n < cache->num_entries) ? cache->entries[n].stretch : TTF_STRETCH_UNSPEC);
}


//
// 'ttfCacheGetStyle()' - Get the font style at index N.
//

ttf_style_t				// O - Font style or `TTF_STYLE_UNSPEC`
ttfCacheGetStyle(ttf_cache_t *cache,	// I - Font cache
		 size_t      n)		// I - Font index starting at `0`
{
  return ((cache && n < cache->num_entries) ? cache->entries[n].style : TTF_STYLE_UNSPEC);
}


//
// 'ttfCacheGetWeight()' - Get the font weight at index N.
//

ttf_weight_t				// O - Font weight or `TTF_WEIGHT_UNSPEC`
ttfCacheGetWeight(ttf_cache_t *cache,	// I - Font cache
		  size_t      n)	// I - Font index starting at `0`
{
  return ((cache && n < cache->num_entries) ? cache->entries[n].weight : TTF_WEIGHT_UNSPEC);
}


//
// 'ttfCacheGetFont()' - Get the font at index N
//

ttf_t *					// O - Font
ttfCacheGetFont(ttf_cache_t *cache,	// I - Font cache
		size_t      n)		// I - Font index starting at `0`
{
  return ((cache && n < cache->num_entries) ? cache->entries[n].font : NULL);
}


//
// 'ttfCacheGetNumFonts()' - Get the number of fonts in the cache.
//

size_t					// O - Number of fonts
ttfCacheGetNumFonts(ttf_cache_t *cache)	// I - Font cache
{
  return (cache ? cache->num_entries : 0);
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
  (void)appname;

  return (NULL);
}


//
// 'ttf_add_font()' - Add a font to the cache and optionally delete it.
//

static void
ttf_add_font(ttf_cache_t *cache,	// I - Font cache
             ttf_t       *font,		// I - Font to add
             const char  *filename,	// I - Filename/URL or `NULL` for in-memory
             struct stat *fileinfo,	// I - File information or `NULL` for none
             bool        delete_it)	// I - Delete the font after adding?
{
  _ttf_centry_t	*entry;			// Current entry
  const char	*family;		// Font family


  // Expand the font cache array as needed...
  if (cache->num_entries >= cache->alloc_entries)
  {
    if ((entry = realloc(cache->entries, (cache->alloc_entries + 32) * sizeof(_ttf_centry_t))) == NULL)
      return;

    cache->entries = entry;
    cache->alloc_entries += 32;
  }

  // Add this font to the end...
  entry = cache->entries + cache->num_entries;

  memset(entry, 0, sizeof(_ttf_centry_t));

  entry->font     = font;
  entry->filename = filename ? strdup(filename) : NULL;
  entry->stretch  = ttfGetStretch(font);
  entry->style    = ttfGetStyle(font);
  entry->weight   = ttfGetWeight(font);

  if ((family = ttfGetFamily(font)) == NULL || (entry->family = strdup(family)) == NULL)
    return;

  if (fileinfo)
  {
    entry->filesize  = fileinfo->st_size;
    entry->filemtime = fileinfo->st_mtime;
  }

  cache->num_entries ++;
}


//
// 'ttf_compare_entries()' - Compare two font cache entries.
//

static int				// O - Result of comparison
ttf_compare_entries(_ttf_centry_t *a,	// I - First entry
                    _ttf_centry_t *b)	// I - Second entry
{
  int	ret = strcmp(a->family, b->family);


  if (!ret)
    ret = (int)a->stretch - (int)b->stretch;

  if (!ret)
    ret = (int)a->style - (int)b->style;

  if (!ret)
    ret = (int)a->weight - (int)b->weight;

  if (!ret)
  {
    if (a->filename && b->filename)
      ret = strcmp(a->filename, b->filename);
    else if (a->filename && !b->filename)
      ret = 1;
    else if (!a->filename && b->filename)
      ret = -1;
  }

  return (ret);
}


//
// 'ttf_load_all_fonts()' - Load all fonts accessible to the user.
//

static void
ttf_load_all_fonts(ttf_cache_t *cache)	// I - Font cache
{
  (void)cache;
}


//
// 'ttf_load_cache()' - Load all fonts from the cache.
//

static bool				// O - `true` on success, `false` on error
ttf_load_cache(ttf_cache_t *cache,	// I - Font cache
               struct stat *cinfo)	// I - Font cache file info
{
  (void)cache;
  (void)cinfo;

  return (false);
}


//
// 'ttf_load_fonts()' - Load fonts from the specified directory into the cache.
//

static time_t				// O - Newest mtime
ttf_load_fonts(ttf_cache_t *cache,	// I - Font cache
               const char  *d,		// I - Directory
               bool        scanonly)	// I - Only scan for fonts
{
  (void)cache;
  (void)d;
  (void)scanonly;

  return (0);
}


//
// 'ttf_save_cache()' - Save a font cache.
//

static void
ttf_save_cache(ttf_cache_t *cache)	// I - Font cache
{
  (void)cache;
}


//
// 'ttf_sort_fonts()' - Sort the fonts in the cache.
//

static void
ttf_sort_fonts(ttf_cache_t *cache)	// I - Font cache
{
  (void)cache;
}

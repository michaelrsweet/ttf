//
// Font caching code for TTF library
//
// https://www.msweet.org/ttf
//
// Copyright © 2019-2025 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "ttf-private.h"
#include <dirent.h>


//
// Constants...
//

#define TTF_CACHE_HEADER	"ttf-cache"
					// Header/prefix string for first line
#define TTF_CACHE_HEADERLEN	9	// Length of header string
#define TTF_CACHE_MAX		65536	// Maximum number of cached fonts
#define TTF_CACHE_VERSION	0	// Version number of cache format


//
// Types...
//

typedef struct _ttf_cfont_s		// Cached font
{
  ttf_t		*font;			// Font, if loaded
  char          *filename,              // Filename or URL
                *family;                // Family
  size_t        idx;                    // Index inside collection
  ttf_stretch_t stretch;                // Stretch
  ttf_style_t   style;                  // Style
  ttf_weight_t  weight;                 // Weight
} _ttf_cfont_t;


struct _ttf_cache_s			// Font cache
{
  char		cname[1024];		// Font cache filename
  ttf_err_cb_t	err_cb;			// Error callback function
  void		*err_cbdata;		// Error callback data
  size_t	num_fonts,		// Number of cached fonts
		alloc_fonts;		// Allocated cached fonts
  _ttf_cfont_t	*fonts;			// Cached fonts
  size_t	font_index[256];	// Index for fonts
  const char	*current_name;		// Current font filename
  size_t	current_index;		// Current font index
};


//
// Local functions...
//

static void	ttf_add_font(ttf_cache_t *cache, ttf_t *font, const char *filename, size_t idx, bool delete_it);
static void	ttf_cache_err_cb(ttf_cache_t *cache, const char *message);
static int	ttf_compare_fonts(_ttf_cfont_t *a, _ttf_cfont_t *b);
static char	*ttf_gets(FILE *fp, char *buffer, size_t bufsize);
static bool	ttf_load_cache(ttf_cache_t *cache);
static time_t	ttf_load_fonts(ttf_cache_t *cache, const char *d, int depth, bool scanonly);
static void	ttf_save_cache(ttf_cache_t *cache);
static void	ttf_sort_fonts(ttf_cache_t *cache);


//
// 'ttfCacheAdd()' - Add a font to the cache.
//
// This function adds the specified font to the cache.
//
// > **Note**: Once added, the cache takes over management of the font.  Do not
// > call @link ttfDelete@ with the font pointer.  Instead, the font will be
// > freed when you call @link ttfCacheDelete@ to free the cache.
//

void
ttfCacheAdd(ttf_cache_t *cache,		// I - Font cache
            ttf_t       *font,		// I - Font
            const char  *filename)	// I - Filename/URL or `NULL` for in-memory
{
  // Range check input...
  if (!cache || !font)
    return;

  // Add the font and re-sort...
  if (filename && *filename == '/')
  {
    // Add a file on disk...
    size_t	i,			// Index
		num_fonts;		// Number of fonts in a collection

    ttf_add_font(cache, font, filename, /*idx*/0, /*delete_it*/false);

    for (i = 1, num_fonts = ttfGetNumFonts(font); i < num_fonts; i ++)
    {
      // Add companion fonts in a collection...
      ttf_t *font_n = ttfCreate(filename, i, cache->err_cb, cache->err_cbdata);

      if (font_n)
        ttf_add_font(cache, font_n, filename, /*idx*/i, /*delete_it*/false);
    }
  }
  else
  {
    // Add an in-memory font...
    ttf_add_font(cache, font, /*filename*/NULL, /*idx*/0, /*delete_it*/false);
  }

  ttf_sort_fonts(cache);
}


//
// 'ttfCacheCreate()' - Create a cache of system and user fonts.
//
// This function creates a cache of system and user fonts, loading any
// previously cached results for the named application "appname".
//
// The "err_cb" and "err_cbdata" arguments specify a callback function and data
// pointer for receiving error messages.  If `NULL`, errors are sent to the
// `stderr` file.  The callback function receives the data pointer and a text
// message string, for example:
//
// ```
// void my_err_cb(void *err_cbdata, const char *message)
// {
//   fprintf(stderr, "ERROR: %s\\n", message);
// }
// ```
//

ttf_cache_t *				// O - Font cache
ttfCacheCreate(const char   *appname,	// I - Application name
               ttf_err_cb_t err_cb,	// I - Error callback function
               void         *err_cbdata)// I - Error callback data
{
  ttf_cache_t	*cache;			// Font cache
  size_t	i,			// Looping var
		num_dirs = 0;		// Number of directories
  const char	*dirs[5];		// Directories
  char		dname[1024];		// Directory filename
#if _WIN32
  const char	*home = getenv("USERPROFILE");
					// Home directory
#else
  const char	*home = getenv("HOME");	// Home directory
#endif // _WIN32
  struct stat	cinfo;			// Cache file information
  bool		rescan = false;		// Rescan fonts?


  // Range check input...
  if (!appname || strchr(appname, '/'))
    return (NULL);

  // Allocate memory for the font cache...
  if ((cache = (ttf_cache_t *)calloc(1, sizeof(ttf_cache_t))) == NULL)
    return (NULL);

  cache->err_cb     = err_cb;
  cache->err_cbdata = err_cbdata;

  // Build a list of font directories...
#ifdef __APPLE__
  dirs[num_dirs ++] = "/System/Library/Fonts";
  dirs[num_dirs ++] = "/Library/Fonts";

  if (home)
  {
    snprintf(dname, sizeof(dname), "%s/Library/Fonts", home);
    dirs[num_dirs ++] = dname;
  }

#elif _WIN32
  dirs[num_dirs ++] = "C:/Windows/Fonts"; // TODO: fix me

#else
  dirs[num_dirs ++] = "/usr/X11R6/lib/X11/fonts";
  dirs[num_dirs ++] = "/usr/share/fonts";
  dirs[num_dirs ++] = "/usr/local/share/fonts";

  if (home)
  {
    snprintf(dname, sizeof(dname), "%s/.fonts", home);
    dirs[num_dirs ++] = dname;
  }
#endif // __APPLE__

#ifdef DEBUG
  TTF_DEBUG("ttfCacheNew: num_dirs=%u\n", (unsigned)num_dirs);
  for (i = 0; i < num_dirs; i ++)
    TTF_DEBUG("ttfCacheNew: dirs[%u]=\"%s\"\n", (unsigned)i, dirs[i]);
#endif // DEBUG

  // Determine where the font cache goes...
#ifdef __APPLE__
  if (home)
    snprintf(cache->cname, sizeof(cache->cname), "%s/Library/Caches/%s.dat", home, appname);

#elif _WIN32
  if (home)
    snprintf(cache->cname, sizeof(cache->cname), "%s/AppData/Local/%s.dat", home, appname);

#else
  const char *xdg_cache_home = getenv("XDG_CACHE_HOME");
					// XDG Per-User Cache Directory

  if (xdg_cache_home)
    snprintf(cache->cname, sizeof(cache->cname), "%s/%s.dat", xdg_cache_home, appname);
  else if (home)
    snprintf(cache->cname, sizeof(cache->cname), "%s/.cache/%s.dat", home, appname);
#endif // __APPLE__

  TTF_DEBUG("ttfCacheNew: cname=\"%s\"\n", cache->cname);

  // See if we need to re-scan the font directories...
  if (stat(cache->cname, &cinfo))
  {
    // No cache file...
    rescan = true;
  }
  else
  {
    // Compare cache file to age of the newest font...
    for (i = 0; i < num_dirs; i ++)
    {
      if (ttf_load_fonts(cache, dirs[i], /*depth*/0, /*scanonly*/true) > cinfo.st_mtime)
      {
        rescan = true;
        break;
      }
    }
  }

  // Load the list of system fonts...
  if (!rescan && !ttf_load_cache(cache))
    rescan = true;

  if (rescan)
  {
    // Scan for fonts...
    for (i = 0; i < num_dirs; i ++)
      ttf_load_fonts(cache, dirs[i], /*depth*/0, /*scanonly*/false);

    // Save the cache...
    ttf_save_cache(cache);
  }

  // Sort the fonts in the cache...
  ttf_sort_fonts(cache);

  TTF_DEBUG("ttfCacheNew: Found %lu fonts.\n", (unsigned long)cache->num_fonts);

  // Return the font cache...
  return (cache);
}


//
// 'ttfCacheDelete()' - Delete a cache.
//
// This function deletes a cache, freeing any loaded fonts.
//

void
ttfCacheDelete(ttf_cache_t *cache)		// I - Font cache
{
  if (cache)
  {
    size_t	i;			// Looping var
    _ttf_cfont_t *font;			// Current cached font

    for (i = cache->num_fonts, font = cache->fonts; i > 0; i --, font ++)
    {
      ttfDelete(font->font);
      free(font->filename);
      free(font->family);
    }

    free(cache->fonts);
    free(cache);
  }
}


//
// 'ttfCacheFind()' - Find a font in the cache.
//
// This function finds a font in the cache.  The returned `ttf_t` object is
// managed by the cache and should not be deleted.
//

ttf_t *					// O - Font or `NULL` if no matching font found
ttfCacheFind(ttf_cache_t   *cache,	// I - Font cache
             const char    *family,	// I - Family name
             ttf_style_t   style,	// I - Font style or `TTF_STYLE_UNSPEC`
             ttf_weight_t  weight,	// I - Font weight or `TTF_WEIGHT_UNSPEC`
             ttf_stretch_t stretch)	// I - Font stretch or `TTF_STRETCH_UNSPEC`
{
  size_t	i;			// Looping var
  _ttf_cfont_t	*font,			// Current font
		*best_font = NULL;	// Best font
  int		result,			// Result of compare
		score,			// Current score
		best_score = 999999;	// Best score


  TTF_DEBUG("ttfCacheFind(cache=%p, family=\"%s\", style=%d, weight=%d, stretch=%d)\n", (void *)cache, family, style, weight, stretch);

  // Range check input...
  if (!cache || !family)
    return (NULL);

  // Loop through the cache looking for a match...
  for (i = cache->font_index[tolower(*family & 255)], font = cache->fonts + i; i < cache->num_fonts; i ++, font ++)
  {
    result = strcasecmp(font->family, family);

    TTF_DEBUG("ttfCacheFind: [%u] family=\"%s\", weight=%d, style=%d, stretch=%d, result=%d\n", (unsigned)i, font->family, font->weight, font->style, font->stretch, result);

    if (result < 0)
      continue;
    else if (result > 0)
      break;

    if (weight == TTF_WEIGHT_UNSPEC)
      score = 0;
    else if (font->weight > weight)
      score = (int)(font->weight - weight);
    else
      score = (int)(weight - font->weight);

    if (stretch != TTF_STRETCH_UNSPEC)
    {
      if (font->stretch > stretch)
        score += (int)(font->stretch - stretch) * 10;
      else
        score += (int)(stretch - font->stretch) * 10;
    }

    if (style != TTF_STYLE_UNSPEC && (style != TTF_STYLE_NORMAL) != (font->style != TTF_STYLE_NORMAL))
      score ++;

    TTF_DEBUG("ttfCacheFind: [%u] score=%d\n", (unsigned)i, score);

    if (score < best_score)
    {
      best_score = score;
      best_font  = font;

      if (score == 0)
        break;
    }
  }

  TTF_DEBUG("ttfCacheFind: best_font=%p(%p)\n", (void *)best_font, best_font ? (void *)best_font->font : NULL);

  if (best_font)
  {
    // Load the font as needed...
    if (!best_font->font && best_font->filename)
      best_font->font = ttfCreate(best_font->filename, best_font->idx, cache->err_cb, cache->err_cbdata);

    // Return the matching font...
    return (best_font->font);
  }

  return (NULL);
}


//
// 'ttfCacheGetFilename()' - Get the font filename at index N.
//

const char *				// O - Font filename/URL or `NULL` if none
ttfCacheGetFilename(ttf_cache_t *cache,	// I - Font cache
                    size_t      n)	// I - Font index starting at `0`
{
  return ((cache && n < cache->num_fonts) ? cache->fonts[n].filename : NULL);
}


//
// 'ttfCacheGetFamily()' - Get the font family at index N.
//

const char *				// O - Font family name or `NULL`
ttfCacheGetFamily(ttf_cache_t *cache,	// I - Font cache
		  size_t      n)	// I - Font index starting at `0`
{
  return ((cache && n < cache->num_fonts) ? cache->fonts[n].family : NULL);
}


//
// 'ttfCacheGetFont()' - Get the font at index N
//
// This function gets a font from the cache.  The returned `ttf_t` object is
// managed by the cache and should not be deleted.
//

ttf_t *					// O - Font
ttfCacheGetFont(ttf_cache_t *cache,	// I - Font cache
		size_t      n)		// I - Font index starting at `0`
{
  ttf_t	*font;				// Font


  // Range check input...
  if (!cache || n >= cache->num_fonts)
    return (NULL);

  // Load the font as needed...
  if ((font = cache->fonts[n].font) == NULL && cache->fonts[n].filename)
  {
    if ((font = ttfCreate(cache->fonts[n].filename, cache->fonts[n].idx, cache->err_cb, cache->err_cbdata)) != NULL)
      cache->fonts[n].font = font;
  }

  return (font);
}


//
// 'ttfCacheGetIndex()' - Get the font index for the composite font at index N.
//

size_t					// O - Composite font index
ttfCacheGetIndex(ttf_cache_t *cache,	// I - Font cache
                 size_t      n)		// I - Cached font index starting at `0`
{
  return ((cache && n < cache->num_fonts) ? cache->fonts[n].idx : 0);
}


//
// 'ttfCacheGetStretch()' - Get the font stretch at index N.
//

ttf_stretch_t				// O - Font stretch or `TTF_STRETCH_UNSPEC`
ttfCacheGetStretch(ttf_cache_t *cache,	// I - Font cache
                    size_t      n)	// I - Font index starting at `0`
{
  return ((cache && n < cache->num_fonts) ? cache->fonts[n].stretch : TTF_STRETCH_UNSPEC);
}


//
// 'ttfCacheGetStyle()' - Get the font style at index N.
//

ttf_style_t				// O - Font style or `TTF_STYLE_UNSPEC`
ttfCacheGetStyle(ttf_cache_t *cache,	// I - Font cache
		 size_t      n)		// I - Font index starting at `0`
{
  return ((cache && n < cache->num_fonts) ? cache->fonts[n].style : TTF_STYLE_UNSPEC);
}


//
// 'ttfCacheGetWeight()' - Get the font weight at index N.
//

ttf_weight_t				// O - Font weight or `TTF_WEIGHT_UNSPEC`
ttfCacheGetWeight(ttf_cache_t *cache,	// I - Font cache
		  size_t      n)	// I - Font index starting at `0`
{
  return ((cache && n < cache->num_fonts) ? cache->fonts[n].weight : TTF_WEIGHT_UNSPEC);
}


//
// 'ttfCacheGetNumFonts()' - Get the number of fonts in the cache.
//

size_t					// O - Number of fonts
ttfCacheGetNumFonts(ttf_cache_t *cache)	// I - Font cache
{
  return (cache ? cache->num_fonts : 0);
}


//
// 'ttf_add_font()' - Add a font to the cache and optionally delete it.
//

static void
ttf_add_font(ttf_cache_t *cache,	// I - Font cache
             ttf_t       *font,		// I - Font to add
             const char  *filename,	// I - Filename/URL or `NULL` for in-memory
             size_t      idx,		// I - Font index
             bool        delete_it)	// I - Delete the font after adding?
{
  _ttf_cfont_t	*cfont;			// Cached font
  const char	*family;		// Font family


#if DEBUG > 1
  TTF_DEBUG("ttf_add_font(cache=%p, font=%p(%s), filename=\"%s\", idx=%u, delete_it=%s)\n", (void *)cache, (void *)font, ttfGetFamily(font), filename, (unsigned)idx, delete_it ? "true" : "false");
#endif // DEBUG > 1

  // Expand the font cache array as needed...
  if (cache->num_fonts >= cache->alloc_fonts)
  {
    if ((cfont = realloc(cache->fonts, (cache->alloc_fonts + 32) * sizeof(_ttf_cfont_t))) == NULL)
      goto cleanup;

    cache->fonts = cfont;
    cache->alloc_fonts += 32;
  }

  // Add this font to the end...
  cfont = cache->fonts + cache->num_fonts;

  memset(cfont, 0, sizeof(_ttf_cfont_t));

  cfont->font     = delete_it ? NULL : font;
  cfont->filename = filename ? strdup(filename) : NULL;
  cfont->idx      = idx;
  cfont->stretch  = ttfGetStretch(font);
  cfont->style    = ttfGetStyle(font);
  cfont->weight   = ttfGetWeight(font);

  if ((family = ttfGetFamily(font)) == NULL || (cfont->family = strdup(family)) == NULL)
    goto cleanup;

  cache->num_fonts ++;

  cleanup:

  if (delete_it)
    ttfDelete(font);
}


//
// 'ttf_cache_err_cb()' - Report an error with the current font filename.
//

static void
ttf_cache_err_cb(ttf_cache_t *cache,	// I - Font cache
                 const char  *message)	// I - Error message
{
  char	xmessage[2048];			// Extended message


  if (cache->current_name)
  {
    if (cache->current_index)
      snprintf(xmessage, sizeof(xmessage), "%s(%u): %s", cache->current_name, (unsigned)cache->current_index, message);
    else
      snprintf(xmessage, sizeof(xmessage), "%s: %s", cache->current_name, message);

    message = xmessage;
  }

  if (cache->err_cb)
    (cache->err_cb)(cache->err_cbdata, message);
  else
    fprintf(stderr, "%s\n", message);
}


//
// 'ttf_compare_fonts()' - Compare two font cache fonts.
//

static int				// O - Result of comparison
ttf_compare_fonts(_ttf_cfont_t *a,	// I - First entry
                    _ttf_cfont_t *b)	// I - Second entry
{
  int	ret = strcasecmp(a->family, b->family);
					// Result of comparison

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
// 'ttf_gets()' - Read a line from the cache file and strip any trailing newline.
//

static char *				// O - Line or `NULL` on EOF
ttf_gets(FILE   *fp,			// I - File pointer
         char   *buffer,		// I - Line buffer
         size_t bufsize)		// I - Size of line buffer
{
  char	*bufend;			// End of line


  // Try reading a line...
  if (!fgets(buffer, (int)bufsize, fp))
    return (NULL);

  // Strip any trailing newline and return it...
  bufend = buffer + strlen(buffer) - 1;

  if (bufend >= buffer && *bufend == '\n')
    *bufend = '\0';

  return (buffer);
}


//
// 'ttf_load_cache()' - Load all fonts from the cache.
//

static bool				// O - `true` on success, `false` on error
ttf_load_cache(ttf_cache_t *cache)	// I - Font cache
{
  FILE		*fp;			// Cache file
  _ttf_cfont_t	*font;			// Current cached font
  char		line[1024],		// Header/attributes line
		*lineptr;		// Pointer into line
  long		num_fonts,		// Number of fonts in cache
		idx,			// Font index in a file
		stretch,		// Font stretch
		style,			// Font style
		weight;			// Font weight


  // Try opening the cache file...
  if ((fp = fopen(cache->cname, "r")) == NULL)
  {
    TTF_DEBUG("ttf_load_cache: Unable to open '%s': %s\n", cache->cname, strerror(errno));
    return (false);
  }

  // Read the first line of the form:
  //
  // ttf-cacheVERSION NUM-FONTS
  if (!ttf_gets(fp, line, sizeof(line)))
    goto error;

  TTF_DEBUG("ttf_load_cache: (version-numfont) %s\n", line);

  if (strncmp(line, TTF_CACHE_HEADER, TTF_CACHE_HEADERLEN))
    goto error;

  if (strtol(line + TTF_CACHE_HEADERLEN, &lineptr, 10) != TTF_CACHE_VERSION || !lineptr || !isspace(*lineptr & 255))
    goto error;

  if ((num_fonts = strtol(lineptr, &lineptr, 10)) < 1 || num_fonts > TTF_CACHE_MAX || !lineptr || *lineptr)
    goto error;

  // Allocate memory for the specified number of fonts...
  if ((font = calloc((size_t)num_fonts, sizeof(_ttf_cfont_t))) == NULL)
    goto error;

  cache->alloc_fonts = (size_t)num_fonts;
  cache->fonts       = font;

  // Read cache entries of the form:
  //
  // INDEX FILENAME
  // STRETCH STYLE WEIGHT FAMILY

  while (ttf_gets(fp, line, sizeof(line)))
  {
    // Get index and filename...
    TTF_DEBUG("ttf_load_cache: (index-filename ) %s\n", line);

    if ((idx = strtol(line, &lineptr, 10)) < 0 || !lineptr || !isspace(*lineptr & 255))
    {
      TTF_DEBUG("ttf_load_cache: Bad index or filename.\n");
      goto error;
    }

    while (*lineptr && isspace(*lineptr & 255))
      lineptr ++;

    if (!*lineptr)
    {
      TTF_DEBUG("ttf_load_cache: Bad filename.\n");
      goto error;
    }

    if (access(lineptr, R_OK))
    {
      TTF_DEBUG("ttf_load_cache: Unable to access '%s': %s\n", lineptr, strerror(errno));
      goto error;
    }

    if ((font->filename = strdup(lineptr)) == NULL)
    {
      TTF_DEBUG("ttf_load_cache: Unable to allocate filename.\n");
      goto error;
    }

    font->idx = (size_t)idx;
    cache->num_fonts ++;

    // Get font attributes (stretch, style, weight, and family name)...
    if (!ttf_gets(fp, line, sizeof(line)))
    {
      TTF_DEBUG("ttf_load_cache: Unable to get font attributes line.\n");
      goto error;
    }

    TTF_DEBUG("ttf_load_cache: (str-sty-wgt-fam) %s\n", line);

    if ((stretch = strtol(line, &lineptr, 10)) < TTF_STRETCH_NORMAL || stretch > TTF_STRETCH_ULTRA_EXPANDED || !lineptr || !isspace(*lineptr & 255))
    {
      TTF_DEBUG("ttf_load_cache: Bad stretch.\n");
      goto error;
    }

    if ((style = strtol(lineptr, &lineptr, 10)) < TTF_STYLE_NORMAL || style > TTF_STYLE_OBLIQUE || !lineptr || !isspace(*lineptr & 255))
    {
      TTF_DEBUG("ttf_load_cache: Bad style.\n");
      goto error;
    }

    if ((weight = strtol(lineptr, &lineptr, 10)) < TTF_WEIGHT_100 || weight > TTF_WEIGHT_900 || !lineptr || !isspace(*lineptr & 255))
    {
      TTF_DEBUG("ttf_load_cache: Bad weight.\n");
      goto error;
    }

    while (*lineptr && isspace(*lineptr & 255))
      lineptr ++;

    if (!*lineptr)
    {
      TTF_DEBUG("ttf_load_cache: Bad family name.\n");
      goto error;
    }

    if ((font->family = strdup(lineptr)) == NULL)
    {
      TTF_DEBUG("ttf_load_cache: Unable to allocate family name.\n");
      goto error;
    }

    font->stretch = (ttf_stretch_t)stretch;
    font->style   = (ttf_style_t)style;
    font->weight  = (ttf_weight_t)weight;

    font ++;
  }

  // See if we got as many fonts as advertised in the header...
  if (cache->num_fonts < cache->alloc_fonts)
  {
    TTF_DEBUG("ttf_load_cache: Got %lu fonts, expected %lu fonts.\n", (unsigned long)cache->num_fonts, (unsigned long)cache->alloc_fonts);
    goto error;
  }

  // Everything was good, close the cache file and return...
  fclose(fp);

  TTF_DEBUG("ttf_load_cache: Returning true.\n");

  return (true);

  // If we get here there was a problem...
  error:

  // Close the cache file...
  fclose(fp);

  // Clear any loaded fonts...
  size_t i;				// Looping var

  for (i = cache->num_fonts, font = cache->fonts; i > 0; i --, font ++)
  {
    free(font->filename);
    free(font->family);
  }

  cache->num_fonts = 0;

  TTF_DEBUG("ttf_load_cache: Returning false.\n");

  return (false);
}


//
// 'ttf_load_fonts()' - Load fonts from the specified directory into the cache.
//

static time_t				// O - Newest mtime
ttf_load_fonts(ttf_cache_t *cache,	// I - Font cache
               const char  *d,		// I - Directory
               int         depth,	// I - Directory depth
               bool        scanonly)	// I - Only scan for fonts
{
  DIR		*dir;			// Directory pointer
  struct dirent	*dent;			// Directory entry
  char		filename[1024],		// Filename
		*ext;			// Extension
  ttf_t		*font;			// Font
  struct stat	info;			// Information about the current file
  time_t	mtime = 0;		// Newest mtime


  if ((dir = opendir(d)) == NULL)
    return (mtime);

  while ((dent = readdir(dir)) != NULL)
  {
    if (dent->d_name[0] == '.')
      continue;

    snprintf(filename, sizeof(filename), "%s/%s", d, dent->d_name);

    if (lstat(filename, &info))
      continue;

    if (info.st_mtime > mtime)
      mtime = info.st_mtime;

    if (S_ISDIR(info.st_mode))
    {
      if (depth < 10 && (info.st_mtime = ttf_load_fonts(cache, filename, depth + 1, scanonly)) > mtime)
        mtime = info.st_mtime;

      continue;
    }

    if (scanonly)
      continue;

    if ((ext = strrchr(dent->d_name, '.')) == NULL)
      continue;

    if (strcmp(ext, ".otc") && strcmp(ext, ".otf") && strcmp(ext, ".ttc") && strcmp(ext, ".ttf"))
      continue;

    cache->current_name  = filename;
    cache->current_index = 0;

    if ((font = ttfCreate(filename, /*idx*/0, (ttf_err_cb_t)ttf_cache_err_cb, cache)) != NULL)
    {
      const char *family = ttfGetFamily(font);
					// Font family name

      if (!family || *family == '.')
      {
        // Ignore fonts starting with a "." since (on macOS at least) these are
        // hidden system fonts...
        ttfDelete(font);
      }
      else
      {
        size_t	i,			// Looping var
		num_fonts;		// Number of fonts

        num_fonts = ttfGetNumFonts(font);

        ttf_add_font(cache, font, filename, /*idx*/0, /*delete_it*/true);

	for (i = 1; i < num_fonts; i ++)
	{
	  cache->current_index = i;

	  if ((font = ttfCreate(filename, /*idx*/i, (ttf_err_cb_t)ttf_cache_err_cb, cache)) != NULL)
	  {
	    if ((family = ttfGetFamily(font)) != NULL && *family != '.')
	      ttf_add_font(cache, font, filename, /*idx*/i, /*delete_it*/true);
	    else
	      ttfDelete(font);
	  }
	}
      }
    }
  }

  closedir(dir);

  return (mtime);
}


//
// 'ttf_save_cache()' - Save a font cache.
//

static void
ttf_save_cache(ttf_cache_t *cache)	// I - Font cache
{
  FILE		*fp;			// Cache file
  size_t	i;			// Looping var
  _ttf_cfont_t	*font;			// Current font


  if (cache->num_fonts == 0)
    return;

  if ((fp = fopen(cache->cname, "w")) == NULL)
  {
    TTF_DEBUG("ttf_load_cache: Unable to create '%s': %s\n", cache->cname, strerror(errno));
    return;
  }

  fprintf(fp, TTF_CACHE_HEADER "%d %lu\n", TTF_CACHE_VERSION, (unsigned long)cache->num_fonts);

  for (i = cache->num_fonts, font = cache->fonts; i > 0; i --, font ++)
  {
    if (!font->filename || !font->family)
      continue;

    fprintf(fp, "%u %s\n", (unsigned)font->idx, font->filename);
    fprintf(fp, "%d %d %d %s\n", font->stretch, font->style, font->weight, font->family);
  }

  fclose(fp);
}


//
// 'ttf_sort_fonts()' - Sort the fonts in the cache.
//

static void
ttf_sort_fonts(ttf_cache_t *cache)	// I - Font cache
{
  size_t	i;			// Looping var
  _ttf_cfont_t	*font;			// Current font


  // First sort the fonts...
  if (cache->num_fonts > 1)
    qsort(cache->fonts, cache->num_fonts, sizeof(_ttf_cfont_t), (int (*)(const void *, const void *))ttf_compare_fonts);

  // Then re-index the fonts by the first character of the font family...
  for (i = 0; i < 256; i ++)
    cache->font_index[i] = cache->num_fonts;

  for (i = 0, font = cache->fonts; i < cache->num_fonts; i ++, font ++)
  {
    int idx = tolower(font->family[0] & 255);
					// Index for font family name

    if (i < cache->font_index[idx])
      cache->font_index[idx] = i;
  }
}

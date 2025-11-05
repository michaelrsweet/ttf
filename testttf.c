//
// Unit test program for TTF library
//
// https://www.msweet.org/ttf
//
// Copyright © 2018-2025 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//
// Usage:
//
//   ./testttf [--list] [FILENAME]
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include "ttf.h"
#include "test.h"


//
// Local functions...
//

static void	error_cb(void *data, const char *message);
static int	list_fonts(bool verbose);
static int	test_font(const char *filename);


//
// 'main()' - Main entry for unit tests.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  int		i;			// Looping var
  int		errors = 0;		// Number of errors
  bool		verbose = false;	// Be verbose?


  if (argc > 1)
  {
    for (i = 1; i < argc; i ++)
    {
      if (!strcmp(argv[i], "--list"))
        errors += list_fonts(verbose);
      else if (!strcmp(argv[i], "--verbose"))
        verbose = true;
      else
	errors += test_font(argv[i]);
    }
  }
  else
  {
    // Test with the bundled TrueType files...
    errors += test_font("testfiles/OpenSans-Bold.ttf");
    errors += test_font("testfiles/OpenSans-Regular.ttf");
    errors += test_font("testfiles/NotoSansJP-Regular.otf");

    errors += list_fonts(false);
  }

  return (errors);
}


//
// 'error_cb()' - Error callback.
//

static void
error_cb(void       *data,		// I - User data (not used)
         const char *message)		// I - Message string
{
  testEndMessage(false, "%s", message);
}


//
// 'list_fonts()' - List available fonts.
//

static int				// O - Number of errors
list_fonts(bool verbose)		// I - Be verbose?
{
  ttf_cache_t	*cache;			// Font cache
  size_t	i,			// Looping var
		num_fonts;		// Number of fonts
  time_t	start,			// Start time
		end;			// End time
  static const char * const stretches[] =
  {					// Font stretch values
    " Normal",				// TTF_STRETCH_NORMAL
    " Ultra-Condensed",			// TTF_STRETCH_ULTRA_CONDENSED
    " Extra-Condensed",			// TTF_STRETCH_EXTRA_CONDENSED
    " Condensed",			// TTF_STRETCH_CONDENSED
    " Semi-Condensed",			// TTF_STRETCH_SEMI_CONDENSED
    " Semi-Expanded",			// TTF_STRETCH_SEMI_EXPANDED
    " Expanded",			// TTF_STRETCH_EXPANDED
    " Extra-Expanded",			// TTF_STRETCH_EXTRA_EXPANDED
    " Ultra-Expanded"			// TTF_STRETCH_ULTRA_EXPANDED
  };
  static const char * const styles[] =	// Font styles
  {
    "",					// TTF_STYLE_NORMAL
    " Italic",				// TTF_STYLE_ITALIC
    " Oblique"				// TTF_STYLE_OBLIQUE
  };
  static const char * const weights[] =	// Font weights
  {
    " Thin",				// TTF_WEIGHT_100
    " Extra-Light",			// TTF_WEIGHT_200
    " Light",				// TTF_WEIGHT_300
    " Regular",				// TTF_WEIGHT_400
    " Medium",				// TTF_WEIGHT_500
    " Semi-Bold",			// TTF_WEIGHT_600
    " Bold",				// TTF_WEIGHT_700
    " Extra-Bold",			// TTF_WEIGHT_800
    " Black"				// TTF_WEIGHT_900
  };


  start = time(NULL);
  testBegin("ttfCacheCreate");
  if ((cache = ttfCacheCreate("testttf", error_cb, /*err_cbdata*/NULL)) == NULL)
    return (1);
  end = time(NULL);

  num_fonts = ttfCacheGetNumFonts(cache);

  testEndMessage(true, "%lu fonts found, %d seconds", (unsigned long)num_fonts, (int)(end - start));

  for (i = 0; i < num_fonts; i ++)
  {
    ttf_stretch_t stretch = ttfCacheGetStretch(cache, i);
    ttf_style_t style = ttfCacheGetStyle(cache, i);
    ttf_weight_t weight = ttfCacheGetWeight(cache, i);

    if (weight < TTF_WEIGHT_100)
      weight = TTF_WEIGHT_100;
    else if (weight > TTF_WEIGHT_900)
      weight = TTF_WEIGHT_900;

    if (!verbose)
      testMessage("%s%s%s%s", ttfCacheGetFamily(cache, i), stretches[stretch], weights[weight / 100 - 1], styles[style]);
    else if (ttfCacheGetIndex(cache, i) > 0)
      testMessage("%s(%u): %s%s%s%s", ttfCacheGetFilename(cache, i), (unsigned)ttfCacheGetIndex(cache, i), ttfCacheGetFamily(cache, i), stretches[stretch], weights[weight / 100 - 1], styles[style]);
    else
      testMessage("%s: %s%s%s%s", ttfCacheGetFilename(cache, i), ttfCacheGetFamily(cache, i), stretches[stretch], weights[weight / 100 - 1], styles[style]);
  }

  return (0);
}


//
// 'test_font()' - Test a font file.
//

static int				// O - Number of errors
test_font(const char *filename)		// I - Font filename
{
  int		i,			// Looping var
		errors = 0;		// Number of errors
  ttf_t		*font;			// Font
  struct stat	fileinfo;		// Font file information
  FILE		*fp = NULL;		// File pointer
  void		*data = NULL;		// Memory buffer for font file
  const char	*value;			// Font (string) value
  int		intvalue;		// Font (integer) value
  float		realvalue;		// Font (real) value
  char		psname[1024];		// Postscript font name
  ttf_rect_t	bounds;			// Bounds
  ttf_rect_t	extents;		// Extents
  size_t	num_fonts;		// Number of fonts
  ttf_style_t	style;			// Font style
  ttf_weight_t	weight;			// Font weight
  static const char * const stretches[] =
  {					// Font stretch strings
    "TTF_STRETCH_NORMAL",		// normal
    "TTF_STRETCH_ULTRA_CONDENSED",	// ultra-condensed
    "TTF_STRETCH_EXTRA_CONDENSED",	// extra-condensed
    "TTF_STRETCH_CONDENSED",		// condensed
    "TTF_STRETCH_SEMI_CONDENSED",	// semi-condensed
    "TTF_STRETCH_SEMI_EXPANDED",	// semi-expanded
    "TTF_STRETCH_EXPANDED",		// expanded
    "TTF_STRETCH_EXTRA_EXPANDED",	// extra-expanded
    "TTF_STRETCH_ULTRA_EXPANDED"	// ultra-expanded
  };
  static const char * const strings[] =	// Test strings
  {
    "Hello, World!",			// English
    "مرحبا بالعالم!",			// Arabic
    "Bonjour le monde!",		// French
    "Γειά σου Κόσμε!",			// Greek
    "שלום עולם!",			// Hebrew
    "Привет мир!",			// Russian
    "こんにちは世界！"			// Japanese
  };
  static const char * const styles[] =	// Font style names
  {
    "TTF_STYLE_NORMAL",
    "TTF_STYLE_ITALIC",
    "TTF_STYLE_OBLIQUE"
  };


  testBegin("ttfCreate(\"%s\")", filename);
  if ((font = ttfCreate(filename, 0, error_cb, NULL)) != NULL)
    testEnd(true);
  else
    return (1);

  testBegin("ttfGetAscent");
  if ((intvalue = ttfGetAscent(font)) > 0)
  {
    testEndMessage(true, "%d", intvalue);
  }
  else
  {
    testEndMessage(false, "%d", intvalue);
    errors ++;
  }

  testBegin("ttfGetBounds");
  if (ttfGetBounds(font, &bounds))
  {
    testEndMessage(true, "%g %g %g %g", bounds.left, bounds.bottom, bounds.right, bounds.top);
  }
  else
  {
    testEnd(false);
    errors ++;
  }

  testBegin("ttfGetCapHeight");
  if ((intvalue = ttfGetCapHeight(font)) > 0)
  {
    testEndMessage(true, "%d", intvalue);
  }
  else
  {
    testEndMessage(false, "%d", intvalue);
    errors ++;
  }

  testBegin("ttfGetCopyright");
  if ((value = ttfGetCopyright(font)) != NULL)
  {
    testEndMessage(true, "%s", value);
  }
  else
  {
    testEndMessage(true, "warning: no copyright found");
  }

  for (i = 0; i < (int)(sizeof(strings) / sizeof(strings[0])); i ++)
  {
    testBegin("ttfGetExtents(\"%s\")", strings[i]);
    if (ttfGetExtents(font, 12.0f, strings[i], &extents))
    {
      testEndMessage(true, "%.1f %.1f %.1f %.1f", extents.left, extents.bottom, extents.right, extents.top);
    }
    else
    {
      testEnd(false);
      errors ++;
    }
  }

  testBegin("ttfGetFamily");
  if ((value = ttfGetFamily(font)) != NULL)
  {
    testEndMessage(true, "%s", value);
  }
  else
  {
    testEnd(false);
    errors ++;
  }

  testBegin("ttfGetItalicAngle");
  if ((realvalue = ttfGetItalicAngle(font)) >= -180.0 && realvalue <= 180.0)
  {
    testEndMessage(true, "%g", realvalue);
  }
  else
  {
    testEndMessage(false, "%g", realvalue);
    errors ++;
  }

  testBegin("ttfGetNumFonts");
  if ((num_fonts = ttfGetNumFonts(font)) > 0)
  {
    testEndMessage(true, "%u", (unsigned)num_fonts);
  }
  else
  {
    testEnd(false);
    errors ++;
  }

  testBegin("ttfGetPostScriptName");
  if ((value = ttfGetPostScriptName(font)) != NULL)
  {
    testEndMessage(true, "%s", value);

    strncpy(psname, value, sizeof(psname) - 1);
    psname[sizeof(psname) - 1] = '\0';
  }
  else
  {
    testEnd(false);
    errors ++;
  }

  testBegin("ttfGetStretch");
  if ((intvalue = (int)ttfGetStretch(font)) >= TTF_STRETCH_NORMAL && intvalue <= TTF_STRETCH_ULTRA_EXPANDED)
  {
    testEndMessage(true, "%s", stretches[intvalue]);
  }
  else
  {
    testEndMessage(false, "%d", intvalue);
    errors ++;
  }

  testBegin("ttfGetStyle");
  if ((style = ttfGetStyle(font)) >= TTF_STYLE_NORMAL && style <= TTF_STYLE_ITALIC)
  {
    testEndMessage(true, "%s", styles[style]);
  }
  else
  {
    testEnd(false);
    errors ++;
  }

  testBegin("ttfGetVersion");
  if ((value = ttfGetVersion(font)) != NULL)
  {
    testEndMessage(true, "%s", value);
  }
  else
  {
    testEnd(false);
    errors ++;
  }

  testBegin("ttfGetWeight");
  if ((weight = ttfGetWeight(font)) >= 0)
  {
    testEndMessage(true, "%u", (unsigned)weight);
  }
  else
  {
    testEnd(false);
    errors ++;
  }

  testBegin("ttfGetWidth(' ')");

  if ((intvalue = ttfGetWidth(font, ' ')) > 0)
  {
    testEndMessage(true, "%d", intvalue);
  }
  else
  {
    testEndMessage(false, "%d", intvalue);
    errors ++;
  }

  testBegin("ttfGetXHeight");
  if ((intvalue = ttfGetXHeight(font)) > 0)
  {
    testEndMessage(true, "%d", intvalue);
  }
  else
  {
    testEndMessage(false, "%d", intvalue);
    errors ++;
  }

  testBegin("ttfIsFixedPitch");
  testEndMessage(true, "%s", ttfIsFixedPitch(font) ? "true" : "false");

  ttfDelete(font);
  font = NULL;

  // Now copy the font to memory and open it that way...
  testBegin("fopen(\"%s\", \"rb\")", filename);
  if ((fp = fopen(filename, "rb")) == NULL)
  {
    testEndMessage(false, "%s", strerror(errno));
    errors ++;
  }
  else
  {
    testEndMessage(true, "%d", fileno(fp));
    testBegin("fstat(%d)", fileno(fp));
    if (fstat(fileno(fp), &fileinfo))
    {
      testEndMessage(false, "%s", strerror(errno));
      errors ++;
    }
    else
    {
      testEndMessage(true, "%lu bytes", (unsigned long)fileinfo.st_size);

      testBegin("malloc(%lu)", (unsigned long)fileinfo.st_size);
      if ((data = malloc((size_t)fileinfo.st_size)) == NULL)
      {
	testEndMessage(false, "%s", strerror(errno));
	errors ++;
      }
      else
      {
	testEnd(true);
        testBegin("fread(%lu)", (unsigned long)fileinfo.st_size);
        if (fread(data, (size_t)fileinfo.st_size, 1, fp) != 1)
        {
	  testEndMessage(false, "%s", strerror(errno));
	  errors ++;
        }
        else
        {
          testEnd(true);
          testBegin("ttfCreateData()");
          if ((font = ttfCreateData(data, (size_t)fileinfo.st_size, /*idx*/0, error_cb, /*err_data*/NULL)) == NULL)
          {
            errors ++;
          }
          else
          {
            testEnd(true);

	    testBegin("ttfGetPostScriptName");
	    if ((value = ttfGetPostScriptName(font)) != NULL)
	    {
	      if (!strcmp(value, psname))
	      {
		testEndMessage(true, "%s", value);
	      }
	      else
	      {
		testEndMessage(false, "got \"%s\", expected \"%s\"", value, psname);
		errors ++;
	      }
	    }
	    else
	    {
	      testEnd(false);
	      errors ++;
	    }
          }
        }
      }
    }

    if (fp)
      fclose(fp);

    free(data);

    ttfDelete(font);
  }

  return (errors);
}

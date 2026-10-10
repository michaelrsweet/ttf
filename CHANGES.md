Changes in TTF
==============

v1.2.0 - YYYY-MM-DD
-------------------

- Added `ttfCacheGetFamilyClass` and `ttfGetFamilyClass` functions to access
  the OS/2 family class metadata (Issue #10)
- Added `ttfCacheIsFixedPitch` function to access the fixed pitch property of
  cached fonts.
- Added `ttfCacheSearch` function for finding groups of fonts (Issue #10)
- Added tests for `ttfGetCMap`.
- Added missing implementation of `ttfGetFilename`.
- Updated the range limit check in `ttfGetWidth`.
- Fixed error handling for bad font unitsPerEm value (Issue #16)
- Fixed support for extended planes in Unicode text.
- Fixed some clang-reported issues.
- Fixed builds on Windows.


v1.1.0 - 2025-12-31
-------------------

- Added configure script for better build system compatibility.
- Added `ttfCache` functions to access user- and system-installed fonts
  (Issue #5)
- Added `ttfCreateData` function to create a font from a memory buffer.
- Added `ttfContainsChar` and `ttfContainsChars` functions to determine whether
  a font contains the necessary characters (Issue #2)
- Added `ttfGetKernedExtents` function to get kerned extents (Issue #6)
- Added guards against fonts claiming they have 0 characters.
- Added stddef.h include to ttf.h.
- Added support for more kinds of TrueType/OpenType fonts.
- Fixed range checking in `ttfGetExtents` (Issue #9)
- Fixed potential heap/integer overflow issues in the TrueType cmap code.
- Fixed underflow in TrueType cmap code.


v1.0.0 - 2024-08-19
-------------------

- Initial stable release.

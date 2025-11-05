Changes in TTF
==============

v1.1.0 - YYYY-MM-DD
-------------------

- Added configure script for better build system compatibility.
- Added `ttfCache` functions to access user- and system-installed fonts
  (Issue #5)
- Added `ttfCreateData` function to create a font from a memory buffer.
- Added guards against fonts claiming they have 0 characters.
- Added stddef.h include to ttf.h.
- Added support for more kinds of TrueType/OpenType fonts.
- Fixed potential heap/integer overflow issues in the TrueType cmap code.
- Fixed underflow in TrueType cmap code.


v1.0.0 - 2024-08-19
-------------------

- Initial stable release.

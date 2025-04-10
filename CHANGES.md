Changes in TTF
==============

v1.1.0 - YYYY-MM-DD
-------------------

- Added `ttfCreateData` function to create a font from a memory buffer.
- Added guards against fonts claiming they have 0 characters.
- Added stddef.h include to ttf.h.
- Fixed potential heap/integer overflow issues in the TrueType cmap code.


v1.0.0 - 2024-08-19
-------------------

- Initial stable release.

TTF - TrueType/OpenType Font Library
====================================

TTF is a simple C library for using TrueType and OpenType font files.


Requirements
------------

You'll need a C compiler.


How to Incorporate in Your Project
----------------------------------

Add the `ttf.c` and `ttf.h` files to your project.  Include the `ttf.h`
header in any file that needs to read font files.


"Kicking the Tires"
-------------------

The supplied makefile allows you to build the unit tests on Linux and macOS (at
least), which verify that all of the functions work as expected to read some
sample font files in the `testfiles` directory:

    make test


Resources
---------

The "ttf.html" file contains some simple HTML documentation for the library.


Legal Stuff
-----------

Copyright © 2018-2024 by Michael R Sweet.

TTF is licensed under the Apache License Version 2.0 with an exception to
allow linking against GPL2/LGPL2 software (like older versions of CUPS).  See
the files "LICENSE" and "NOTICE" for more information.

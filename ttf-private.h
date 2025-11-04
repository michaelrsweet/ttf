//
// Private header file for TTF library
//
//     https://github.com/michaelrsweet/ttf
//
// Copyright © 2018-2025 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#ifndef TTF_PRIVATE_H
#  define TTF_PRIVATE_H


//
// Common stuff...
//

#  ifdef _WIN32
     // Disable C runtime warnings on Windows...
#    define _CRT_SECURE_NO_WARNINGS 1
#  endif // _WIN32
#  include "ttf.h"
#  include <errno.h>
#  include <stdio.h>
#  include <stdlib.h>
#  include <stdarg.h>
#  include <string.h>
#  include <fcntl.h>
#  include <sys/stat.h>
#  ifdef _WIN32
#    include <io.h>
#    include <direct.h>
     // Microsoft renames the POSIX functions to _name, and introduces
     // a broken compatibility layer using the original names.  As a result,
     // random crashes can occur when, for example, strdup() allocates memory
     // from a different heap than used by malloc() and free().
     //
     // To avoid moronic problems like this, we #define the POSIX function
     // names to the corresponding non-standard Microsoft names.
#    define access	_access
#    define close	_close
#    define fileno	_fileno
#    define lseek(f,o,w) (off_t)_lseek((f),(long)(o),(w))
#    define mkdir(d,p)	_mkdir(d)
#    define open	_open
#    define read(f,b,s)	_read((f),(b),(unsigned)(s))
#    define rmdir	_rmdir
#    define snprintf	_snprintf
#    define strdup	_strdup
#    define unlink	_unlink
#    define vsnprintf	_vsnprintf
#    define write(f,b,s) _write((f),(b),(unsigned)(s))
     // Map various POSIX values, too...
#    define F_OK	00
#    define W_OK	02
#    define R_OK	04
#    define O_BINARY	_O_BINARY
#    define O_RDONLY	_O_RDONLY
#    define O_WRONLY	_O_WRONLY
#    define O_CREAT	_O_CREAT
#    define O_TRUNC	_O_TRUNC
typedef __int64 ssize_t;		// POSIX type not present on Windows... @private@
#  else
#    include <unistd.h>
     // Map Windows value for binary file I/O...
#    define O_BINARY	0
#  endif // _WIN32


//
// DEBUG is typically defined for debug builds.  TTF_DEBUG maps to fprintf when
// DEBUG is defined and is a no-op otherwise...
//

#  ifdef DEBUG
#    define TTF_DEBUG(...) fprintf(stderr, __VA_ARGS__)
#  else
#    define TTF_DEBUG(...)
#  endif // DEBUG


//
// TTF_FORMAT_ARGS tells the compiler to validate printf-style format
// arguments, producing warnings when there are issues...
//

#  if defined(__has_extension) || defined(__GNUC__)
#    define TTF_FORMAT_ARGS(a,b) __attribute__ ((__format__(__printf__, a, b)))
#  else
#    define TTF_FORMAT_ARGS(a,b)
#  endif // __has_extension || __GNUC__
#endif // !TTF_PRIVATE_H

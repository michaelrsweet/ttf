#
# Unit test makefile for TTF library.
#
#     https://github.com/michaelrsweet/ttf
#
# Copyright © 2018-2024 by Michael R Sweet.
#
# Licensed under Apache License v2.0.  See the file "LICENSE" for more
# information.
#

# This is a POSIX makefile
.POSIX:


# Version and installation directories
VERSION	=	1.1.0
prefix	=	$(DESTDIR)/usr/local
includedir =	$(prefix)/include
libdir	=	$(prefix)/lib
mandir	=	$(prefix)/share/man


# Commands and options
AR	=	ar
ARFLAGS	=	rc
CC	=	gcc
CFLAGS	=	$(OPTIM) -Wall $(CPPFLAGS)
CPPFLAGS =	'-DVERSION="$(VERSION)"'
DOCFLAGS =	--author "Michael R Sweet" \
		--copyright "Copyright (c) 2018-2025 by Michael R Sweet" \
		--docversion $(VERSION)
LDFLAGS	=	$(OPTIM)
LIBS	=
OPTIM	=	-Os -g
RANLIB	=	ranlib


# Targets
LIBTTF	=	libttf.a
OBJS	=	testttf.o ttf.o
TARGETS	=	testttf $(LIBTTF)


# Build rules
.SUFFIXES:	.c .o
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<


# Build everything
all:		$(TARGETS)


# Clean all build files
clean:
	rm -f $(TARGETS) $(OBJS)


# Install the library and header file
install:	all
	mkdir -p $(includedir)
	cp ttf.h $(includedir)
	mkdir -p $(libdir)
	cp $(LIBTTF) $(libdir)
	if test $(LIBTTF) = libttf.a; then \
		$(RANLIB) $(libdir)/libttf.a; \
	fi
	mkdir -p $(mandir)/man3
	cp ttf.3 $(mandir)/man3
	mkdir -p $(prefix)/lib/pkgconfig
	echo "prefix=$(prefix)" >$(prefix)/lib/pkgconfig/ttf.pc
	echo "includedir=$(includedir)" >>$(prefix)/lib/pkgconfig/ttf.pc
	echo "libdir=$(libdir)" >>$(prefix)/lib/pkgconfig/ttf.pc
	echo "Name: ttf" >>$(prefix)/lib/pkgconfig/ttf.pc
	echo "Description: TTF library" >>$(prefix)/lib/pkgconfig/ttf.pc
	echo "URL: https://www.msweet.org/ttf/" >>$(prefix)/lib/pkgconfig/ttf.pc
	echo "Version: $(VERSION)" >>$(prefix)/lib/pkgconfig/ttf.pc
	echo "Libs: -L$${libdir} -lttf" >>$(prefix)/lib/pkgconfig/ttf.pc
	echo "Cflags: -I$${includedir}" >>$(prefix)/lib/pkgconfig/ttf.pc


# Run the unit tests
test:		testttf
	./testttf


# Analyze code with the Clang static analyzer <https://clang-analyzer.llvm.org>
clang:
	clang $(CPPFLAGS) --analyze $(OBJS:.o=.c) 2>clang.log
	rm -rf $(OBJS:.o=.plist)
	test -s clang.log && (echo "$(GHA_ERROR)Clang detected issues."; echo ""; cat clang.log; exit 1) || exit 0


# Scan the code using Cppcheck <http://cppcheck.sourceforge.net>
cppcheck:
	cppcheck --template=gcc --addon=cert.py --suppress=cert-MSC24-C --suppress=cert-INT31-c --suppress=cert-EXP05-C --suppress=cert-API01-C $(OBJS:.o=.c) 2>cppcheck.log
	@test -s cppcheck.log && (echo "$(GHA_ERROR)Cppcheck detected issues."; echo ""; cat cppcheck.log; exit 1) || exit 0


# Make the library with debug printfs and the address sanitizer
debug:
	$(MAKE) clean
	$(MAKE) OPTIM="-DDEBUG -g -fsanitize=address" all


# Unit test program
testttf:	ttf.o testttf.o
	$(CC) $(LDFLAGS) -o testttf ttf.o testttf.o $(LIBS)


# Library
libttf.a:	ttf.o
	$(AR) $(ARFLAGS) $@ ttf.o
	$(RANLIB) $@


# Make documentation
doc:
	codedoc $(DOCFLAGS) --title "TTF v$(VERSION) Manual" \
		--coverimage ttf-512.png --body ttf.md \
		ttf.xml ttf.h ttf.c >ttf.html
	codedoc $(DOCFLAGS) --title "ttf - functions" --man ttf --section 3 --body ttf.md ttf.xml >ttf.3
	$(RM) ttf.xml


# Dependencies
$(OBJS):	ttf.h Makefile

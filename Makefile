#
# Unit test makefile for TTF library.
#
#     https://github.com/michaelrsweet/ttf
#
# Copyright © 2021 by Michael R Sweet.
#
# Licensed under Apache License v2.0.  See the file "LICENSE" for more
# information.
#

VERSION	=	1.0
prefix	=	$(DESTDIR)/usr/local
includedir =	$(prefix)/include
libdir	=	$(prefix)/lib
mandir	=	$(prefix)/share/man

AR	=	ar
ARFLAGS	=	rc
CC	=	gcc
CFLAGS	=	$(OPTIM) -Wall '-DVERSION="$(VERSION)"'
LDFLAGS	=	$(OPTIM)
LIBS	=
LIBTTF	=	libttf.a
OBJS	=	testttf.o ttf.o
OPTIM	=	-Os -g
RANLIB	=	ranlib
TARGETS	=	testttf $(LIBTTF)

.SUFFIXES:	.c .o
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<


all:		$(TARGETS)


clean:
	rm -f $(TARGETS) $(OBJS)


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


test:		testttf
	./testttf


# Analyze code with the Clang static analyzer <https://clang-analyzer.llvm.org>
clang:
	clang $(CPPFLAGS) --analyze $(OBJS:.o=.c) 2>clang.log
	rm -rf $(OBJS:.o=.plist)
	test -s clang.log && (echo "$(GHA_ERROR)Clang detected issues."; echo ""; cat clang.log; exit 1) || exit 0


# Scan the code using Cppcheck <http://cppcheck.sourceforge.net>
cppcheck:
	cppcheck --template=gcc --addon=cert.py --suppress=cert-MSC24-C --suppress=cert-EXP05-C --suppress=cert-API01-C $(OBJS:.o=.c) 2>cppcheck.log
	@test -s cppcheck.log && (echo "$(GHA_ERROR)Cppcheck detected issues."; echo ""; cat cppcheck.log; exit 1) || exit 0


debug:
	$(MAKE) clean
	$(MAKE) OPTIM="-DDEBUG -g -fsanitize=address" all


testttf:	ttf.o testttf.o
	$(CC) $(LDFLAGS) -o testttf ttf.o testttf.o $(LIBS)


libttf.a:	ttf.o
	$(AR) $(ARFLAGS) $@ ttf.o
	$(RANLIB) $@


$(OBJS):	ttf.h Makefile

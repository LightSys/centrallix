## $Id: Makefile,v 1.1 2001/08/13 18:04:19 gbeeley Exp $
## $Source: /srv/bld/centrallix-repo/centrallix-lib/Attic/Makefile,v $
##
## Centrallix-Lib Makefile
##
## This library is made available under the GNU Lesser General Public
## License, version 2.1.
##
## Copyright (C) 1998-2001 LightSys Technology Services, Inc.
##

## CFLAGS.  "MTCFLAGS" is used exclusively by the MTASK module
## compilation.  DO NOT OPTIMIZE MTASK!!!!  It will cause it to
## stop working!!!
##
CFLAGS=-Iinclude -DDBMAGIC -DNM_USE_SYSMALLOC -Wall
MTCFLAGS=-Iinclude -DDBMAGIC -DNM_USE_SYSMALLOC -Wall

XSTATICFILES=mtask.o mtlexer.o xarray.o xhash.o xstring.o mtsession.o newmalloc.o xhashqueue.o stparse.o
#XSTATICFILES=mtask.o mtlexer.o xarray.o xhash.o xstring.o mtsession.o newmalloc.o xhashqueue.o stparse.o net_raw_api.o
STATICFILES=$(patsubst %,src/%,$(XSTATICFILES))

XDYNAMICFILES=mtask.lo mtlexer.lo xarray.lo xhash.lo xstring.lo mtsession.lo newmalloc.lo xhashqueue.lo stparse.lo
#XDYNAMICFILES=mtask.lo mtlexer.lo xarray.lo xhash.lo xstring.lo mtsession.lo newmalloc.lo xhashqueue.lo stparse.lo net_raw_api.lo
DYNAMICFILES=$(patsubst %,src/%,$(XDYNAMICFILES))

.SUFFIXES:	.c .o .lo

.PHONY: all
all:	libCentrallix.so libCentrallix.a

src/mtask.o: src/mtask.c
	gcc $(MTCFLAGS) -c $< -o $@

src/mtask.lo: src/mtask.c
	gcc $(MTCFLAGS) -shared -fPIC -c $< -o $@

.c.lo:	$<
	gcc $(CFLAGS) -shared -fPIC -c $< -o $@

.c.o:	$<
	gcc $(CFLAGS) -c $< -o $@

libCentrallix.so:	$(DYNAMICFILES)
	gcc $(CFLAGS) -shared -fPIC $(DYNAMICFILES) -o libCentrallix.so

libCentrallix.a:	$(STATICFILES)
	ar -r libCentrallix.a $(STATICFILES)
	ar -s libCentrallix.a

.PHONY: clean
clean:
	/bin/rm -f src/*.o src/*.lo libCentrallix.a libCentrallix.so 2>/dev/null

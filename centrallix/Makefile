####  Centrallix Makefile
####
####  LightSys Technology Services, Inc
####  Copyright (C) 1998-2001.  All rights reserved.
####
####  If you're in this file looking for how to add a new objectsystem
####  driver to the build, add its .o file to the XOBJECTSYSTEM variable's
####  file list, below.  Don't forget to call the driver's initialization
####  function from lsmain.c and test_obj.c at appropriate places!!  And,
####  don't forget to add the content-type information to types.cfg.
####

# VPATH - all the directories we manage.
VPATH=objectsystem:osdrivers:utility:multiquery:htmlgen:netdrivers:include:report:testsuite:expression

# Support modules -- only basic utilities.
#XBASIC=mtask.o mtlexer.o xarray.o xhash.o xstring.o mtsession.o newmalloc.o xhashqueue.o stparse.o
#BASIC=$(patsubst %,utility/%,$(XBASIC))

# Support modules -- basic and advanced utilities.
XSUPPORT=stparse.o st_node.o htmlparse.o 
SUPPORT=$(patsubst %,utility/%,$(XSUPPORT))

# OSML modules that implement the OSML API for internal C use
XOBJECTSYSTEM=obj_main.o obj_session.o obj_object.o obj_attr.o obj_query.o obj_method.o obj_content.o obj_params.o obj_trx.o obj_datatypes.o obj_rootnode.o
OBJECTSYSTEM=$(patsubst %,objectsystem/%,$(XOBJECTSYSTEM))

# Objectsystem drivers.
#XOBJDRIVERS=objdrv_ux.o objdrv_sybase.o objdrv_struct.o objdrv_qytree.o objdrv_report.o objdrv_uxprint.o objdrv_datafile.o objdrv_unixuser.o
XOBJDRIVERS=objdrv_ux.o objdrv_unixuser.o objdrv_sybase.o objdrv_struct.o objdrv_report.o objdrv_uxprint.o objdrv_qytree.o objdrv_datafile.o objdrv_audio.o
OBJDRIVERS=$(patsubst %,osdrivers/%,$(XOBJDRIVERS))

# DHTML generation subsystem, including drivers and the ht_render main system.
XHTDRIVERS=htdrv_page.o \
           htdrv_scrollpane.o \
	   htdrv_treeview.o \
	   htdrv_html.o \
	   htdrv_connector.o \
	   htdrv_imagebutton.o \
	   htdrv_textbutton.o \
	   htdrv_menu.o \
	   htdrv_frameset.o \
	   ht_render.o \
	   htdrv_variable.o \
	   htdrv_tab.o \
	   htdrv_pane.o \
	   htdrv_table.o \
	   htdrv_window.o \
	   htdrv_checkbox.o \
	   htdrv_radiobutton.o \
	   htdrv_editbox.o \
	   htdrv_timer.o \
	   htdrv_execmethod.o \
	   htdrv_form.o \
	   htdrv_formstatus.o \
	   htdrv_alerter.o \
	   htdrv_osrc.o \
	   htdrv_spinner.o \
	   htdrv_dropdown.o \

HTDRIVERS=$(patsubst %,htmlgen/%,$(XHTDRIVERS))

# Multiquery subsystem, SQL parser, and query drivers.
XQYDRIVERS=multiq_tablegen.o multiq_projection.o multiq_equijoin.o multiquery.o
QYDRIVERS=$(patsubst %,multiquery/%,$(XQYDRIVERS))

# Network drivers.
XNETDRIVERS=net_http.o net_bdqs.o
NETDRIVERS=$(patsubst %,netdrivers/%,$(XNETDRIVERS))

# Reporting subsystem, including printer drivers and print management system.
XRPTMODS=epsonfx_prt.o hppcl_prt.o html_prt.o barcode.o prtmgmt.o text_prt.o
RPTMODS=$(patsubst %,report/%,$(XRPTMODS))

# Expression evaluation/compilation manager.
XEXPRMODS=exp_main.o exp_params.o exp_compiler.o exp_evaluate.o exp_functions.o exp_generator.o
EXPRMODS=$(patsubst %,expression/%,$(XEXPRMODS))

# Javascript VM modules... for Centrallix side of things.
XLSJSVM=jsvm_client.o
LSJSVM=$(patsubst %,jsvm/%,$(XLSJSVM))

# Javascript VM modules... runtime engine side of things.
XJSVM=jsvm_server.o
JSVM=$(patsubst %,jsvm/%,$(XJSVM))

# Test suite.
XTESTSUITE=ts1.c ts2.c ts3.c
TESTSUITE=$(patsubst %,testsuite/%,$(XTESTSUITE))

# Some basic build parameters.
MASTERHOST=greg
PROFILE=-pg
SYBASE=/opt/sybase
BUILD=$(shell cat .build)
BLDFILE=$(shell if [ "$$HOSTNAME" = "$(MASTERHOST)" ]; then echo ".build"; echo '1' >.subbuild; else echo ".subbuild"; if [ ! -e .subbuild ]; then echo '1' > .subbuild; fi; fi)
SUBBUILD=$(shell if [ ! -e .subbuild ]; then echo '1' > .subbuild; fi; cat .subbuild)
PREVBUILD=$(shell expr $(BUILD) - 1)
#YEARS=$(shell YR=1998; while [ $$YR != `date +%Y` ]; do /bin/echo -n $$YR,; YR=`expr $$YR + 1`; done; /bin/echo -n $$YR)
YEARS=$(shell YR=`date +%Y`; echo 1998-$$YR)
VERSION=$(shell cat .version)
CFLAGS=-g $(PROFILE) -I$(SYBASE)/include -I./include -I../centrallix-lib/include -DBUILD=$(BUILD) -DSUBBUILD=$(SUBBUILD) -DYEARS=\"$(YEARS)\" -DVERSION=\"$(VERSION)\" -DNHT_ENABLE -DDBMAGIC -DNM_USE_SYSMALLOC -DHAS_IOPUTC -DSTABILITY=\"DEVELOPMENT\" -Wall -Wno-implicit-function-declaration
#CFLAGS=-g $(PROFILE) -I$(SYBASE)/include -I./include -I../centrallix-lib/include -DBUILD=$(BUILD) -DYEARS=\"$(YEARS)\" -DVERSION=\"$(VERSION)\" -DNHT_ENABLE -DDBMAGIC -DNM_USE_SYSMALLOC -DHAS_IOPUTC -Wall
#LIBS=-lnsl -lsocket
#LIBS=-lsybtcl -lct -linsck -lcomn -lintl -lcs -lm
LIBS=-lct -lcomn -lintl -lsybtcl -lcs -linsck -lm -lcrypt -lreadline ../centrallix-lib/libCentrallix.a -lncurses
#LIBS=-lm
STATICLIBS=/usr/local/syblinux/elf/lib/libct.a /usr/local/syblinux/elf/lib/libcomn.a /usr/local/syblinux/elf/lib/libintl.a /usr/local/syblinux/elf/lib/libsybtcl.a /usr/local/syblinux/elf/lib/libcs.a /usr/local/syblinux/elf/lib/libinsck.a
LIBDIRS=-L$(SYBASE)/lib
#BASEOBJS=$(SUPPORT) utility/expression.o $(OBJECTSYSTEM) $(OBJDRIVERS) $(QYDRIVERS) $(RPTMODS) $(STATICLIBS)
#BASEOBJS=$(SUPPORT) $(EXPRMODS) $(OBJECTSYSTEM) $(OBJDRIVERS) $(QYDRIVERS) $(RPTMODS) $(LSJSVM)
BASEOBJS=$(SUPPORT) $(EXPRMODS) $(OBJECTSYSTEM) $(OBJDRIVERS) $(QYDRIVERS) $(RPTMODS)
LSOBJS=$(BASEOBJS) $(HTDRIVERS) $(NETDRIVERS)

centrallix: .build .subbuild lsmain.o $(LSOBJS)
	@echo ''
	@echo "Linking Build #"`cat .build`"-"`cat .subbuild`
	@echo
	cc lsmain.o $(LSOBJS) $(LIBDIRS) $(LIBS) $(PROFILE) -o centrallix
	@sh -c 'BLD=`cat $(BLDFILE)`; BLD=`expr $$BLD + 1`; echo $$BLD > $(BLDFILE)'
	@rm lsmain.o

jsengine: $(JSVM)
	cc $(JSVM) $(LIBDIRS) $(PROFILE) -o jsvm

test_obj: test_obj.o $(BASEOBJS)
	cc test_obj.o $(BASEOBJS) $(LIBDIRS) $(PROFILE) -o test_obj $(LIBS)

test_prt: test_prt.o $(BASEOBJS)
	cc test_prt.o $(BASEOBJS) $(LIBDIRS) $(PROFILE) -o test_prt $(LIBS)

ts: ts.o $(LSOBJS) $(TESTSUITE)
	cc ts.o $(LSOBJS) $(TESTSUITE) $(LIBDIRS) $(PROFILE) -o ts $(LIBS)

domatches: domatches.o $(BASEOBJS)
	cc domatches.o $(BASEOBJS) $(LIBDIRS) $(PROFILE) -o domatches $(LIBS)

lsclient: lsclient.o $(BASIC)
	cc lsclient.o $(BASIC) $(PROFILE) -o lsclient

htmltest: htmltest.o $(SUPPORT)
	cc htmltest.o $(SUPPORT) $(PROFILE) -o htmltest

clean:
	-@rm */*.o
	-@rm centrallix test_obj lsclient
	-@rm *.o

depend:
	makedepend -I include -I ../centrallix-lib/include -f .depend *.c */*.c 2>/dev/null

backup:
	@/bin/echo -n Creating a backup of $(VERSION).$(PREVBUILD) "... "
	@mkdir VER/$(VERSION).$(PREVBUILD) 2>/dev/null; exit 0
	@cp -r htmlgen include lsmain.c lsclient.c test_obj.c Makefile multiquery netdrivers objectsystem osdrivers report utility expression VER/$(VERSION).$(PREVBUILD)
	@echo "done."

# Dependencies made by "make depend"
include .depend

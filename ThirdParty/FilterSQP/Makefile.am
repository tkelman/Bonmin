# Copyright (C) 2003, 2006 International Business Machines and others.
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id$

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =

noinst_LTLIBRARIES = libfiltersqp.la

nodist_libfiltersqp_la_SOURCES = \
	source/src/auxil.f \
	source/src/bqpd.f \
	source/src/filteraux.f \
	source/src/filter.f \
	source/src/QPsolves.f \
	source/src/scaling.f \
	source/src/sparseA.f \
	source/src/sparseL.f \
	source/src/util.f \
	source/src/wdotd.f

libfiltersqp_la_LDFLAGS = $(LT_LDFLAGS)

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =

include ../../BuildTools/Makemain.inc

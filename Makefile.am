# Makefile.am - by Edward Grace <ej.grace@imperial.ac.uk>
#
# automake Makefile
#
# $Id:$
#

@SET_MAKE@

# Define the binaraies (programs) to build 
bin_PROGRAMS = pdf2pcl

# For each binary define the sources that it depends on, the C flags and any librarys
# N.B. POPPLERGLIB_ should be set by the configure script either using pkg-config, 
# by guessing where the librarys may be, or by the user specifying.
pdf2pcl_SOURCES = pdf2pcl.cpp
pdf2pcl_CXXFLAGS = $(CAIRO_CFLAGS) $(GTK2_CFLAGS) $(POPPLERGLIB_CFLAGS) -O
pdf2pcl_LDADD = $(CAIRO_LIBS) $(GTK2_LIBS) $(POPPLERGLIB_LIBS) -lpoppler

MAINTAINERCLEANFILES = Makefile.in

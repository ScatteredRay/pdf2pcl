#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# autoconf 
#
# configure.ac - by Edward Grace <ej.grace@imperial.ac.uk>

AC_PREREQ(2.60)
AC_INIT(pdf2svg, 0.2.1, David Barton <davebarton@cityinthesky.co.uk>)
AC_CONFIG_SRCDIR([pdf2pcl.cpp])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Use pkg-config to check for the existence of the cairo libraries
#PKG_CHECK_MODULES(CAIRO,cairo >= 1.2.6)
# Use pkg-config to check for the existence of the poppler-glib libraries
PKG_CHECK_MODULES(POPPLERGLIB,poppler >= 0.5.4)
# GTK
#PKG_CHECK_MODULES(GTK2,gtk+-2.0)

# Generate the Makefile
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([navi], [0.0.8], [gpsnavi-inquiry@itg.hitachi.co.jp])

AC_ARG_ENABLE([standalone],
  [AS_HELP_STRING([--enable-standalone], [Enable non AGL build (default is no)])],
  [:],
  [enable_standalone=no])

AM_CONDITIONAL([ENABLE_STANDALONE], [test "$enable_standalone" = "yes"])

AM_INIT_AUTOMAKE()
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_SRCDIR([src/main/navi.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([GLIB], [glib-2.0 gthread-2.0])
PKG_CHECK_MODULES([FREETYPE2], [freetype2])
PKG_CHECK_MODULES([WAYLAND], [wayland-client wayland-egl egl])
PKG_CHECK_MODULES([GL], [glesv2])
PKG_CHECK_MODULES([ZLIB], [zlib])
PKG_CHECK_MODULES([SQLITE3], [sqlite3])
PKG_CHECK_MODULES([EXPAT], [expat])
PKG_CHECK_MODULES([OPENSSL], [openssl])
PKG_CHECK_MODULES([DBUSCXX], [dbus-c++-1])
PKG_CHECK_MODULES([WINDOWMANAGER], [libwindowmanager])
PKG_CHECK_MODULES([HOMESCREEN], [libhomescreen])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT

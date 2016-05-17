AC_INIT([hbox], [0.1])
AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([include/config.h])

AC_PROG_CC
AC_PROG_CXX
AC_LANG([C++])
AX_BOOST_BASE([1.38])
AX_BOOST_FILESYSTEM
AX_BOOST_THREAD

AC_CONFIG_FILES([Makefile])

PKG_CHECK_MODULES([log4cpp], [log4cpp])
PKG_CHECK_MODULES([gloox], [gloox])
PKG_CHECK_MODULES([openssl],[openssl])

AC_OUTPUT

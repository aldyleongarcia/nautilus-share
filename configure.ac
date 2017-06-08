AC_PREREQ([2.69])
AC_INIT([nautilus-share],[0.8.1])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([foreign 1.9 -Wall])

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

dnl Checks for header files.
AC_CHECK_HEADERS([locale.h stdlib.h string.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_MODE_T
AC_TYPE_PID_T

dnl Checks for library functions.
AC_CHECK_FUNCS([setlocale strdup])


AM_PROG_AR
LT_INIT
IT_PROG_INTLTOOL([0.29])


dnl  Checks for libraries.

NAUTILUS_REQUIRED=2.10.0
GLIB_REQUIRED=2.4.0
PKG_CHECK_MODULES(NAUTILUS, libnautilus-extension >= $NAUTILUS_REQUIRED)
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED)

dnl ========================================================
dnl Get nautilus extensionsdir
dnl ========================================================

nautilus_extensiondir=`$PKG_CONFIG --variable=extensiondir libnautilus-extension`
AC_SUBST([nautilus_extensiondir])

dnl ========================================================
dnl Set interfacesdir
dnl ========================================================
AC_SUBST([interfacesdir], ['${pkgdatadir}/interfaces'])

dnl ========================================================
dnl Set sudoers
dnl ========================================================
AC_SUBST([sudopdbeditdir], ['/etc/sudoers.d'])


dnl ========================================================
dnl Translations
dnl ========================================================

AM_GNU_GETTEXT_VERSION([0.19.7])
AM_GNU_GETTEXT([external])

ALL_LINGUAS="ar as ast bn_IN bs ca ca@valencia cs da de el en_GB eo es et de dz eu fi fr gl gu he hu id it ja kn ko lt lv ml mr nb oc or pa pl pt pt_BR ro ru sk sl sr sr@latin sv ta te uk vi zh_CN zh_HK zh_TW"
GETTEXT_PACKAGE=nautilus-share

AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [gettext package name])
AM_GLIB_GNU_GETTEXT

AM_GLIB_DEFINE_LOCALEDIR(NAUTILUS_SHARE_LOCALEDIR)

dnl ========================================================

AC_CONFIG_FILES([ 
  po/Makefile.in
  Makefile
  src/Makefile
  interfaces/Makefile
  sudoers/Makefile
])
AC_OUTPUT


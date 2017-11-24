AC_INIT([watermark], [0.1], [chris@rorvick.com])
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE([-Wall -Werror])
LT_INIT

AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O

AC_CHECK_LIB([m], [tan])

PKG_CHECK_MODULES([GDK], [gdk-2.0])
PKG_CHECK_MODULES([GDK_PIXBUF], [gdk-pixbuf-2.0])
PKG_CHECK_MODULES([CAIRO], [cairo])
PKG_CHECK_MODULES([POPPLERGLIB], [poppler-glib])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_CONFIG_MACRO_DIR([m4])

AC_OUTPUT

echo \
"------------------------------------------------
${PACKAGE_NAME} Version ${PACKAGE_VERSION}

Prefix        : ${prefix}
Compiler      : ${CC} ${CFLAGS} ${CPPFLAGS}
Linker        : ${LD} ${LDFLAGS} ${LIBS}
------------------------------------------------"

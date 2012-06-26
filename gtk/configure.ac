AC_INIT(main.c)

GIT_REVISION=`git rev-parse HEAD`
VERSION="WAC-r$SVN_REVISION"
AM_INIT_AUTOMAKE(xlook,$VERSION)
AM_CONFIG_HEADER(config.h)

# Check for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_CHECK_PROG(DOXYGEN,doxygen,doxygen)

if test "x${DOXYGEN}" = "x" ; then
	AC_MSG_WARN([** No Doxygen found.])
	AC_MSG_WARN([** Developer documentation will not be generated.])
fi

AC_CHECK_PROG(PKGCONFIG,pkg-config,pkg-config)
if test "x${PKGCONFIG}" = "x" ; then
	AC_MSG_ERROR([** No pkg-config found.  See README.txt for troubleshooting tips.])
fi

# Put builds in their subdirs.
AC_LANG_C

PKG_CHECK_MODULES([GTK], [gtk+-2.0])

CFLAGS="$CFLAGS -Wall $GTK_CFLAGS"
CPPFLAGS="$CPPFLAGS -Wall $GTK_CFLAGS" 
LDFLAGS="$LDFLAGS"

# Check for libraries.
AC_CHECK_LIB(m,floor,,[AC_MSG_ERROR([couldn't find m library.])])

AC_CHECK_FUNCS([floor memset pow sqrt strchr strcspn])
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netinet/in.h stdint.h stdlib.h string.h strings.h sys/file.h sys/timeb.h unistd.h])

AC_CHECK_HEADER_STDBOOL
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_TYPE_UINT32_T

# Check for headers.
#AC_CHECK_HEADERS(gtk/gtk.h,,
#                [AC_MSG_ERROR([couldn't find gtk header files. Make sure gtk development package is installed from http://r.research.att.com/libs/GTK_2.18.5-X11.pkg/])])

AC_OUTPUT(Makefile)

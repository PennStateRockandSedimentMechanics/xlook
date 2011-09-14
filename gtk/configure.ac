AC_INIT(main.c)

GIT_REVISION=`git rev-parse HEAD`
VERSION="WAC-r$SVN_REVISION"
AM_INIT_AUTOMAKE(xlook,$VERSION)
AM_CONFIG_HEADER(config.h)

# Check for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_CHECK_PROG(DOXYGEN,doxygen,doxygen,[@echo 'You must have doxygen installed to generate the developer documentation.'])
AC_CHECK_PROG(PKGCONFIG,pkg-config,pkg-config,[@echo 'You must have pkg-config installed to generate the code.'])

# Put builds in their subdirs.

AC_LANG_C

# Check for pkg-config in path
# Using pkg-config, check for gtk/gtk.h

CFLAGS="$CFLAGS -Wall -I/usr/openwin/include -DOWTOOLKIT_WARNING_DISABLED `pkg-config --cflags gtk+-2.0`"
CPPFLAGS="$CPPFLAGS -I/usr/openwin/include"
LDFLAGS="$LDFLAGS -L/usr/openwin/lib -L/usr/X11R6/lib `pkg-config --libs gtk+-2.0`"

# Check for libraries.
AC_CHECK_LIB(X11,XDrawLine,,[AC_MSG_ERROR([couldn't find X11 libraries.])])
#AC_CHECK_LIB(olgx,olgx_draw_box,,[AC_MSG_ERROR([couldn't find xview libraries. Make sure xview development package is installed from http://physionet.org/physiotools/xview/])],[-lX11])
#AC_CHECK_LIB(xview,xv_init,,[AC_MSG_ERROR([couldn't find xview libraries. Make sure xview development package is installed from http://physionet.org/physiotools/xview/])],[-lX11 -lolgx])
AC_CHECK_LIB(m,floor,,[AC_MSG_ERROR([couldn't find m library.])])

# Check for headers.
AC_CHECK_HEADERS(gtk/gtk.h,,
                [AC_MSG_ERROR([couldn't find gtk header files. Make sure gtk development package is installed from http://r.research.att.com/libs/GTK_2.18.5-X11.pkg/])])
AC_CHECK_HEADERS(arpa/inet.h)
AC_CHECK_HEADERS(netinet/in.h)

AC_OUTPUT(Makefile)

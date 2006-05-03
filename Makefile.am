AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = xlook

xlook_SOURCES = main.c event.c can.c menus.c drawwin.c cmds.c \
	filtersm.c nrutil.c sort.c special.c array.c \
	median.c polyfit.c func.c tpulsel.c perrfl.c mem.c fq.c \
	look_funcs.c lookio.c simplexl.c strcmd.c cmds1.c \
	mouse.c rs_fric_tool.c qi_look.c notices.c messages.c

noinst_HEADERS = global.h
    
LDADD = -lm -lX11 -lxview -lolgx

AM_CFLAGS = -I/usr/openwin/include -DOWTOOLKIT_WARNING_DISABLED

AM_LDFLAGS = -L/usr/openwin/lib -L/usr/X11R6/lib

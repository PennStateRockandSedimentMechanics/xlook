SOURCES = main.c event.c can.c menus.c drawwin.c cmds.c \
	filtersm.c nrutil.c sort.c special.c array.c \
	median.c polyfit.c func.c tpulsel.c perrfl.c mem.c fq.c \
	look_funcs.c lookio.c simplexl.c strcmd.c cmds1.c \
	mouse.c rs_fric_tool.c qi_look.c notices.c messages.c

OBJECTS = main.o event.o can.o menus.o drawwin.o cmds.o \
	filtersm.o nrutil.o sort.o special.o array.o \
	median.o polyfit.o func.o tpulsel.o perrfl.o mem.o fq.o \
	look_funcs.o lookio.o simplexl.o strcmd.o cmds1.o \
	mouse.o rs_fric_tool.o qi_look.o notices.o messages.o 
#	/usr/lib/debug/malloc.o /usr/lib/debug/mallocmap.o*
LIBS = -L/usr/openwin/lib -L/opt/local/bin -R/usr/openwin/lib -R/opt/local/bin -lX11 -lxview -lolgx -lm
#LIBS = -lX11 -lm
#To disable the warning message, use #define OWTOOLKIT_WARNING_DISABLED or -D."
CFLAGS = -Xs -I/usr/openwin/include -I/opt/local/bin -DOWTOOLKIT_WARNING_DISABLED
CPPFLAGS = 
LDFLAGS =
CC = cc
dbxlook := CFLAGS= -g -Xs -I/usr/openwin/include -I/opt/local/bin -DOWTOOLKIT_WARNING_DISABLED

xlook: $(OBJECTS) global.h
	cc -o thisxlook $(OBJECTS) $(CFLAGS) $(LIBS)

dbxlook: $(OBJECTS) global.h
	cc -g -o thisxlook $(OBJECTS) $(CFLAGS) $(LIBS)


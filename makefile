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

LIBS = -lm -lX11 -lXm -lxview -lolgx

CFLAGS = -I/usr/openwin/include -DOWTOOLKIT_WARNING_DISABLED

LDFLAGS = -L/usr/openwin/lib -L/usr/X11R6/lib

CC = cc

xlook: $(OBJECTS) global.h
	cc -o xlook $(OBJECTS) $(LDFLAGS) $(LIBS)

clean: 
	rm -f *.o xlook
#this should work to compile on mac.  cjm 26.3.08
SOURCES = xmsp.c

OBJECTS = xmsp.o

LIBS = -L/usr/openwin/lib -L/usr/X11R6/lib -lX11 -lxview -lolgx -lm
#To disable the warning message, use #define OWTOOLKIT_WARNING_DISABLED or -D."
CFLAGS =  -I/usr/openwin/include -I/opt/local/bin -DOWTOOLKIT_WARNING_DISABLED

# orig line: LIBS = -L/usr/openwin/lib -lX11 -lxview -lolgx -lm
# orig line: CFLAGS = -I/usr/openwin/include
CPPFLAGS = 
LDFLAGS =
CC = cc
dbxxmsp := CFLAGS= -g -I/usr/openwin/include


xmsp: $(OBJECTS) 
	cc -o thisxmsp $(OBJECTS) $(CFLAGS) $(LIBS)

dbxxmsp: $(OBJECTS) 
	cc -g -o thisxmsp $(OBJECTS) $(DBX_CFLAGS) $(LIBS)



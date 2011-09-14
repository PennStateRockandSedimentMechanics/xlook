AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = xlook

xlook_SOURCES = \
		tutorial.c \
        array.c \
        array.h \
        can.c \
        can.h \
        cmds.c \
        cmds.h \
        cmds1.c \
        cmds1.h \
        event.c \
        event.h \
        filtersm.c \
        filtersm.h \
        fq.c \
        fq.h \
        func.c \
        func.h \
        global.h \
        look_funcs.c \
        look_funcs.h \
        lookio.c \
        lookio.h \
        median.c \
        median.h \
        mem.c \
        mem.h \
        messages.c \
        messages.h \
        notices.c \
        notices.h \
        nr.h \
        nrutil.c \
        nrutil.h \
        perrfl.c \
        perrfl.h \
        polyfit.c \
        polyfit.h \
        qi_look.c \
        qi_look.h \
        simplexl.c \
        simplexl.h \
        sort.c \
        sort.h \
        special.c \
        special.h \
        strcmd.c \
        strcmd.h \
        tpulsel.c \
        tpulsel.h \
		ui.c \
		ui.h
		
xlook_REALSOURCES= \
        drawwin.c \
        drawwin.h \
        main.c \
        main.h \
        menus.c \
        menus.h \
        mouse.c \
        mouse.h \
        rs_fric_tool.c \
        rs_fric_tool.h 

EXTRA_DIST = \
        Doxyfile \
        canvas.ico \
        xlook.ico \
        example/p446_r \
        example/p446s7tr005l

.PHONY: doc
doc:
	$(DOXYGEN)

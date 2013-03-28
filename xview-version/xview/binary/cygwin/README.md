[The binaries and the following instructions were taken from 
http://www.physionet.org/physiotools/xview/#windows on 2011-05-25]

The file `xview-3.2p1.4-18c-cygwin.tar.bz2` (in the cygwin directory) contains
a set of XView binaries, header files, and standard XView clients for use
under any modern version of MS-Windows (95/98/NT/2000/ME/XP). It was compiled
using the free [Cygwin development environment](http://cygwin.com/), which
includes an X11 server that must be running in order to interact with XView
applications. To install this package, first install Cygwin, including the
optional sunrpc, libX11-devel and X-start-menu-icons packages; then open a
Cygwin terminal window, copy the tarball into your default (home) directory,
and run these commands:

    cd /
    tar xfvj ~/xview-3.2p1.4-18c-cygwin.tar.bz2

These commands install the XView package into subdirectories of
`/usr/openwin`. In order to link and use the libraries or to use the standard
clients, you must add `/usr/openwin/bin` to your `PATH`. You can do this
automatically by adding the lines

    export PATH=/usr/openwin/bin:$PATH
    export DISPLAY=:0.0

to the text files named `.bashrc` and `.bash_profile` (note the initial '.' in
the names of these files) that should be located in your home directory. (Edit
these files using any text editor, such as Windows Notepad, or create them if
they don't exist; be sure to save them as plain text and without any suffix
attached to the file names.)

You must also start the X server before attempting to run any X clients. One
way to do this is via `/usr/X11R6/bin/startxwin.bat`. The version of this
script that comes with Cygwin's X-start-menu-icons package runs the X server
with backing store disabled, which causes XView applications to open with
blank (solid white) windows. To avoid this problem, open `startxwin.bat` in
any text editor (Windows Notepad will work), and find the line that reads

    run XWin -multiwindow -clipboard -silent-dup-error

Add the option `+bs` to the end of this line, so that it reads

    run XWin -multiwindow -clipboard -silent-dup-error +bs

If you have a two-button mouse, you will be able to simulate a middle button
click by "chording" (clicking both buttons at approximately the same time) by
using this form of the XWin command instead:

    run XWin -multiwindow -clipboard -silent-dup-error +bs -emulate3buttons

Save `startwin.bat`. If you make a desktop shortcut to this file, you can
click on it to launch the X server and an xterm window.

You can then start XView clients either from the xterm window or (if you have
set `DISPLAY` as shown above) from a Cygwin terminal window.


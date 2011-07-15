# xlook

This is the source code for xlook, a tool developed by Chris Marone
&lt;marone@psu.edu&gt; and used by the [Penn State Rock Mechanics
Lab](http://www3.geosc.psu.edu/~cjm38/lab.html).

Maintenance has been performed by [West Arete](http://westarete.com/) and by 
Ryan Martell.

# Prerequisites

## xview 

To compile xlook, you'll need to have the xview and olgx libraries and headers
installed. 

As of this writing, xview is included as part of the standard packages
for modern Linux distributions.

Windows users will need to have [Cygwin](http://www.cygwin.com/) installed,
including its compilers and X server. The xview libraries for cygwin are
included here under `xview/binary/cygwin/`.

Mac users should install the universal xview binaries included here under
`xview/binary/darwin/`.

## X11

xlook requires X11 to run. Linux users will already have it, Windows users
should use the version provided by cygwin, and Mac users should ensure they
already have it installed under `/Applications/Utilities/` (it comes with most
recent versions).

# Compiling

First, try compiling xlook from scratch using the following commands:

    autoreconf
    ./configure
    make

With any luck, that should leave you with an xlook executable in the current
directory.

## 64-Bit

If you are compiling on a 64-bit operating system (such as MacOS 10.6 or
greater), you will probably need to set environment variables to
force the compiler to create a 32-bit executable. On MacOS this can be 
accomplished with:

    ./configure CFLAGS=-m32

We can't build a 64-bit version of xlook because the xview libraries are only
available in 32-bit, and
[apparently](http://www.physionet.org/physiotools/xview/#64-bit) it would be
quite a bit of effort to convert those to 64-bit, so it's unlikely to happen.

# Running

Run the resulting executable directly:

    ./xlook

Sample data is included under the `example` subdirectory. 

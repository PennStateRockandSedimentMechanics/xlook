
== Prerequisites

* XCode (for compilers and related tools)
* xview libraries (usually located in /usr/openwin/lib).

== Compiling

First, try compiling xlook from scratch using the following commands:

	autoreconf
	./configure
	make
	
That should leave you with an xlook executable in the current directory.

== Snow Leopard

If you are compiling on Mac OS X 10.6 Snow Leopard, you will probably need to set the 
following environment variable to prevent the compiler from trying to create
a 64-bit-capable executable:

export CFLAGS=-m32

We can't build a 64-bit version of xlook because the xview libraries are only
available in 32-bit, and apparently it would be quite a bit of effort to
convert those to 64-bit (so it's unlikely to happen). Reference:

http://www.physionet.org/physiotools/xview/#64-bit
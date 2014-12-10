# xlook

This is the source code for xlook, a tool developed by Chris Marone
<marone@psu.edu> and used by the [Penn State Rock Mechanics
Lab](http://rockmechanics.psu.edu) and others.

## Installation
To compile xlook, you'll need to have the xview and olgx libraries and headers
installed.

As of this writing, xview is included as part of the standard packages
for modern Linux distributions.

Windows users will need to have [Cygwin](http://www.cygwin.com/) installed,
including its compilers and X server. The xview libraries for cygwin are
included here under `xview/binary/cygwin/`.

xlook also requires X11 to run. Linux users will already have it, Windows users
should use the version provided by cygwin. Mac users should follow the instructions
below

### Mac

#### Pre-Requisites

- From App Store, buy (free) XCode

Other dependencies can be handled from Macports, Brew, etc.

##### Macports
- Install macports [http://www.macports.org/install.php](http://www.macports.org/install.php)
- Restart terminal
- *sudo port install pkgconfig*

##### Brew
- Open a terminal in the bash shell
- Install [homebrew](http://brew.sh): *ruby -e "$(curl -fsSL https<nolink>://raw.githubusercontent.com/Homebrew/install/master/install)"*

#### Compiling
- Launch Terminal
- Change to directory you want xlook to live
- *git clone https<nolink>://github.com/PennStateRockandSedimentMechanics/xlook.git*
- *cd xlook/gtk-version*
- *./create_static_ui.sh*
- *./configure*
- *make*

### Problems
- Did you set the GTK path if necessary? *export PATH=/Library/Frameworks/GTK+.framework/Resources/bin:$PATH*




### General Compiling Instructions

First, try compiling xlook from scratch using the following commands:

    autoreconf
    ./configure
    make

With any luck, that should leave you with an xlook executable in the current
directory.

#### 64-Bit

If you are compiling on a 64-bit operating system (such as MacOS 10.6 or
greater), you will probably need to set environment variables to
force the compiler to create a 32-bit executable. On MacOS this can be
accomplished with:

    ./configure CFLAGS=-m32

We can't build a 64-bit version of xlook because the xview libraries are only
available in 32-bit, and
[apparently](http://www.physionet.org/physiotools/xview/#64-bit) it would be
quite a bit of effort to convert those to 64-bit, so it's unlikely to happen.

## Running

Run the resulting executable directly:

    ./xlook

Sample data is included under the `example` subdirectory.

It is recommended to add xlook to your system path.

## Contributors
- [West Arete](http://westarete.com/)
- [Ryan Martell](http://www.martellventures.com)
- [John Leeman](http://www.johnrleeman.com)

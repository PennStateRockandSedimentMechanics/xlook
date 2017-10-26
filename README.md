# xlook

This is the source code for xlook, a tool developed by Chris Marone
<marone@psu.edu> and used by the [Penn State Rock Mechanics
Lab](http://rockmechanics.psu.edu) and others.

## Installation
xlook requires X11/XQuartz to run. Linux users will already have it, Windows
users should use the version provided by cygwin. Mac users should follow the
instructions below

### Mac

#### Pre-Requisites

- From App Store, buy (free) XCode
- Start XCode and accept the agreement, quit XCode
- Download and install [XQuartz](http://xquartz.macosforge.org/landing/)

Other dependencies can be handled from Homebrew.

##### Brew
- Open a terminal in the bash shell
- Install [homebrew](http://brew.sh):
```
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```
- Setup and update brew

```
brew doctor
brew update
```

- Install packages with the following:

```
brew install pkgconfig gtk autoconf automake xz
```
- Because of a gtk installation issue, we need to export a path. Your version
  number may be different, so **do not just copy and paste this**.

```
export PKG_CONFIG_PATH='/usr/local/Cellar/gtk+/2.24.31_1/lib/pkgconfig/'
```

#### Compiling
- Launch Terminal
- Change to directory you want xlook to live
- Clone into the git repo:

```
git clone https://github.com/PennStateRockandSedimentMechanics/xlook.git
```

- Go into the `src` directory and compile:

```
cd xlook/src
./create_static_ui.sh
./configure
make
```

### Ubuntu
- Open a terminal and run the following commands.
- `sudo apt-get update`
- `sudo apt-get install git`
- `git clone https://github.com/PennStateRockandSedimentMechanics/xlook.git`
- `sudo apt-get install libgtk2.0-dev autoconf pkgconf gcc`
- `cd xlook/src`
- `touch ./missing`
- `touch ./compile`
- `./create_static_ui.sh`
- `autoreconf`
- `./configure`
- `make`

### Problems
- Did you set the GTK path if necessary? *export PATH=/Library/Frameworks/GTK+.framework/Resources/bin:$PATH*

#### 64-Bit

If you are compiling on a 64-bit operating system you may need to set a compiler
flag, though this has not been necessary on any recent Mac or Linux version.

```
./configure CFLAGS=-m32
```

We can't build a 64-bit version of xlook because the xview libraries are only
available in 32-bit, and
[apparently](http://www.physionet.org/physiotools/xview/#64-bit) it would be
quite a bit of effort to convert those to 64-bit, so it's unlikely to happen.

## Running

Run the resulting executable directly:

```
./xlook
```

Sample data is included under the `example` subdirectory.

It is recommended that you add xlook to your system path.

## Contributors
- [West Arete](http://westarete.com/)
- [Ryan Martell](http://www.martellventures.com)
- [John Leeman](http://www.johnrleeman.com)

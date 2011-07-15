# Mac Installer for xlook

This subdirectory contains the source and distribution files for a MacOS
installer package for xlook. This makes it easy to distribute the application
to users. When you create a new version of xlook, you'll want to use the files
in this subdirectory to create a new version of the installer package for
distribution.

## Prerequisites

Download and install the free application 
[Iceberg](http://s.sudre.free.fr/Software/Iceberg.html).

## Layout

Open Iceberg, and then use it to open `mac-installer/xlook/xlook.packproj`.
Navigate through the left sidebar and you'll see all of the different
configuration settings that can be used to control how the application is
packaged. For the most part, you'll only be concerned with "Settings",
"Documents", and "Files".

You can see from the Documents section that the text on the splash screen is
taken from `mac-installer/xlook/README.txt`. Note that the path to this file
and to all other files is relative, so that Iceberg can find them regardless
of where this source tree is located (and you don't wind up with e.g.
`/Users/joe` in the path).

You can see from the Files section that the resulting package will pull the
prerequisite xview and olgx libraries from the `xview/binary/darwin/`
subdirectory and install them in `/usr/openwin/lib`. This way the end user
should not have to install them themselves.

Similarly, the package expects to find the xlook executable to be packaged
alongside the source code, where it's usually built.

## Packaging

1. Build xlook as usual and leave the resulting binary in place
2. Open `mac-installer/xlook/xlook.packproj` in Iceberg
3. Optionally update the xlook version number under xlook/settings
4. Choose "Build" from the Build menu
5. You should now have `mac-installer/xlook/build/xlook.pkg` that you can 
   distribute to others

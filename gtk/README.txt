Building:

You have to have pkg-config in your path.  If you install the GTK framework in the default location, then you need to run configure with this line:

env PATH=$PATH:/Library/Frameworks/GTK+.framework/Versions/2.18.X11/Resources/bin ./configure

(You can find pkg-config with locate pkg-config)
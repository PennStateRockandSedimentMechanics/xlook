Building:

You have to have pkg-config in your path.  If you install the GTK framework in the default location, then you need to run configure with this line:

env PATH=$PATH:/Library/Frameworks/GTK+.framework/Versions/2.18.X11/Resources/bin ./configure

(You can find pkg-config with locate pkg-config)

If you get an error about ui_static.c not being present, you need to generate it by running:

./create_static_ui.sh

(The ui_static.c file is just a include of all the *.glade files, into strings, so that the xlook program can be copied around without requiring the
*.glade files to be copied as well)
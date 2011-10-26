#!/usr/bin/env bash

echo "Regnerating static UI..."

# Remove previous one..
rm ui_static.c

# Create new one...
DATE=`date`

echo '/* ' > ui_static.c
echo '* ui_static.c' >> ui_static.c
echo '* generated on '$DATE >> ui_static.c
echo '* ' >> ui_static.c
echo '*/ ' >> ui_static.c

# Add the main file...
echo 'const char *xlook_glade_data= "\' >> ui_static.c
sed -E -e 's/$/\\/g' -e 's/\"/\\\"/g' xlook.glade >> ui_static.c
echo '";' >> ui_static.c
echo '' >> ui_static.c

# Add rs_fric file...
echo 'const char *rs_fric_window_glade_data= "\' >> ui_static.c
sed -E -e 's/$/\\/g' -e 's/\"/\\\"/g' rs_fric_window.glade >> ui_static.c
echo '";' >> ui_static.c
echo '' >> ui_static.c

# Add plot window
echo 'const char *plot_window_glade_data= "\' >> ui_static.c
sed -E -e 's/$/\\/g' -e 's/\"/\\\"/g' plot_window.glade >> ui_static.c
echo '";' >> ui_static.c
echo '' >> ui_static.c
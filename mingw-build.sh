#!/bin/sh

echo "Temp image will be staged to $1"

./configure --prefix=/c/atheme --enable-contrib
make -j16
make install DESTDIR=$1

# stage
cp /mingw/bin/libgnurx-0.dll $1/c/atheme/bin
cp /mingw/bin/libeay32.dll $1/c/atheme/bin
cp /mingw/bin/libintl-8.dll $1/c/atheme/bin

# done!
echo "Image built at $1"

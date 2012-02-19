#!/bin/sh

echo "Temp image will be staged to $1"

./configure --prefix=. --disable-nls --enable-contrib
make -j16
make install DESTDIR=$1

# move dependencies around
mv $1/lib/* $1/bin
rm -rf $1/lib

# we need libregex from msys
mv /mingw/msys/1.0/lib/libregex.dll $1/bin

# done!
echo "Image built at $1"

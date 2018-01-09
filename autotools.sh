#/bin/sh
libtoolize && \
aclocal -I ./m4 -I . && \
autoheader && \
automake --add-missing && \
autoconf ./configure.ac > ./configure &&
chmod +x configure &&
./configure --build=i686-pc-linux-gnu "CFLAGS=-m32" "LDFLAGS=-m32"

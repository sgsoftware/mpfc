#!/bin/bash
mkdir -p m4
cp -sf /usr/share/aclocal/pkg.m4 m4/
aclocal
autoconf
autoheader
libtoolize
autopoint
cp po/Makevars.template po/Makevars
automake -a --foreign

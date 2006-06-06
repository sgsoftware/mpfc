#!/bin/bash
aclocal
autoconf
autoheader
libtoolize
autopoint
cp po/Makevars.template po/Makevars
automake -a

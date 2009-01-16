#!/bin/sh

aclocal
autoheader
svn log -v > ChangeLog
automake --add-missing --copy
autoconf

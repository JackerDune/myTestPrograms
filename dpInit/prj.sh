#!/bin/sh
rm -f cscope.file
rm -f cscope.out
find `pwd` -regex '.*\.h\|.*\.c\|.*\.cpp\|.*\.hh\|.*\.S\|.*\*.s\|.*\.html\|.*\.css\|.*\.php\|.*\.js' > cscope.file
cscope -bkRi cscope.file

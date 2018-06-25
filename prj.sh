#!/bin/sh
rm -f cscope.files
rm -f cscope.out
find `pwd` -regex '.*\.h\|.*\.c\|.*\.cpp\|.*\.hh\|.*\.S\|.*\.s\|.*\.html\|.*\.css\|.*\.php\|.*\.java\|.*\.py\|.*\.js' > cscope.files
cp cscope.files gtags.files
cscope -bkRi cscope.files
gtags -v

#find `pwd` -regex '.*\.h\|.*\.c\|.*\.cpp\|.*\.hh\|.*\.S\|.*\.s\|.*\.html\|.*\.css\|.*\.php\|.*\.java\|.*\.py\|.*\.js' |xargs etags


#etags etags

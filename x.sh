#!/bin/bash

COMM_LIB=`pwd`
rm -f ${COMM_LIB}/cscope*
echo ${COMM_LIB}

find ${COMM_LIB}  -name "*.h" >>cscope.files
find ${COMM_LIB}  -name "*.c" >>cscope.files
find ${COMM_LIB}  -name "*.cc" >>cscope.files
find ${COMM_LIB}  -name "*.cpp" >>cscope.files
find ${COMM_LIB}  -name "*.hpp" >>cscope.files
find ${COMM_LIB}  -name "Makefile.*" >>cscope.files
find ${COMM_LIB}  -name "makefile.*" >>cscope.files
find ${COMM_LIB}  -name "Makefile" >>cscope.files
find ${COMM_LIB}  -name "*.s" >>cscope.files
find ${COMM_LIB}  -name "*.S" >>cscope.files
find ${COMM_LIB}  -name "*.tcl" >>cscope.files
find ${COMM_LIB}  -name "*.mk" >>cscope.files
find ${COMM_LIB}  -name "*.mk" >>cscope.files
find ${COMM_LIB}  -name "*.def" >>cscope.files
find ${COMM_LIB}  -name "*.py" >>cscope.files

cscope -i cscope.files


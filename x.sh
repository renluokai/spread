rm -rf cscope.files
find ./ -name *.h >>cscope.files
find ./ -name *.cpp >>cscope.files
cscope -i cscope.out

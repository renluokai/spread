VPATH = src 
vpath *.h include
SOURCE = $(shell `find './ -name *.cpp'`)
OBJECTS = main.cpp
.PHONE: td
td:
	@echo $(SOURCE)
	#g++ -M $(SOURCE)
###############################################################################
.PHONY: clean help

clean:
	rm -rf a.out

help:
	@echo "this Makefile is used to build luren's trading system"

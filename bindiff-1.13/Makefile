#
# Makefile for "bindiff"
#
# Written by: Hugh Mahon
# Last update: (March 18, 2008)
# $Source: /home/hugh/sources/misc/bindiff/Makefile,v $
# $Revision: 1.2 $  $Author: hugh $
# $State: Exp $  $Locker: hugh $
# $Date: 2008/03/22 20:32:39 $
#

CC         = gcc
EXE        = bindiff

OBJECTS    = bindiff.o

all: $(EXE) 

$(EXE): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(EXE) $(OBJECTS)
	strip $(EXE)
	rm -f $(OBJECTS)

install:
	cp $(EXE) /usr/local/bin/
	cp bindiff.1 /usr/local/man/man1

clean:
	rm -f $(OBJECTS) $(EXE)

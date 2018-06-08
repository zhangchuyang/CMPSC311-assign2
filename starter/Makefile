####################################################
#
# CMPSC311 - Fall 2017
# Assignment #2 Makefile
# Author   : Patrick McDaniel
# Created  : Sat Sep 2nd 08:56:10 EDT 2017
#
####################################################
#
# STUDENTS MUST ADD COMMENTS BELOW
#

# Variables
ARCHIVE=ar
CC=gcc 
LINK=gcc
CFLAGS=-c -Wall -I. -fpic -g
LINKFLAGS=-L. -g
LIBFLAGS=-shared -Wall
LINKLIBS=-lcrud -lgcrypt 
DEPFILE=Makefile.dep

# Files to build

HDD_SIM_OBJFILES=	hdd_sim.o \
					hdd_file_io.o
#target file
TARGETS=	hdd_sim 
					
# Suffix rules
.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS)  -o $@ $<

# Productions

all : $(TARGETS) 
#make hdd_sim able to compile	
hdd_sim : $(HDD_SIM_OBJFILES) libcrud.a
	$(LINK) $(LINKFLAGS) -o $@ $(HDD_SIM_OBJFILES) $(LINKLIBS) 

# Do dependency generation
depend : $(DEPFILE)

$(DEPFILE) : $(HDD_SIM_OBJFILES:.o=.c)
	gcc -MM $(CFLAGS) $(HDD_SIM_OBJFILES:.o=.c) > $(DEPFILE)

#do the remove file        
# Cleanup 
clean:
	rm -f $(TARGETS) $(HDD_SIM_OBJFILES) 
  
# Dependancies


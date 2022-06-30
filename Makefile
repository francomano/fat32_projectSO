CCOPTS= -Wall -g -lm -std=gnu99 -Wstrict-prototypes 
LIBS= 
CC=gcc
AR=ar


BINS= shell

OBJS=disk_driver.o\
	 linked_list.o\
	 fat32.o
#add here your object files

HEADERS=disk_driver.h\
		linked_list.h\
	    fat32.h

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

.phony: clean all


all:	$(BINS) 

shell: shell.c $(OBJS) 
	$(CC) $(CCOPTS)  -o $@ $^ $(LIBS)

clean:
	rm -rf *.o *~  $(BINS)

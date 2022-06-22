CCOPTS= -Wall -g -std=gnu99 -Wstrict-prototypes -lm
LIBS= 
CC=gcc
AR=ar


BINS= shell

OBJS=disk_driver.o\
	 fat32.o
#add here your object files

HEADERS=disk_driver.h\
	    fat32.h

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

.phony: clean all


all:	$(BINS) 

shell: shell.c $(OBJS) 
	$(CC) $(CCOPTS)  -o $@ $^ $(LIBS)

clean:
	rm -rf *.o *~  $(BINS)

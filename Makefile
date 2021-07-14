CC = g++
INCLUDE = -I./thread -I./poll
CFLAG = -g -ggdb -pthread

AR_FLAG = -rcs

THREAD_SUBDIRS = thread
THREAD_SUBOBJS = $(THREAD_SUBDIRS)/*.o

RPOLL_SUBDIRS = poll
RPOLL_SUBOBJS = $(RPOLL_SUBDIRS)/*.o

OBJ = main.o


.SUFFIXES: .cpp
.cpp.o:
	$(CC) $(INCLUDE) $(CFLAG) -c $< -o $@

main : $(THREAD_SUBDIRS) $(RPOLL_SUBDIRS) $(OBJ)
	$(CC) $(INCLUDE) $(CFLAG) $(OBJ) $(THREAD_SUBOBJS) $(RPOLL_SUBOBJS) -o $@

$(THREAD_SUBDIRS) : clean
	cd $@; make

$(RPOLL_SUBDIRS) : clean
	cd $@; make

clean:
	-rm *.o
	-cd $(THREAD_SUBDIRS); make clean
	-cd $(RPOLL_SUBDIRS); make clean


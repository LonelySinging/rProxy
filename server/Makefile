TOP = ..

CC = g++
INCLUDE = -I$(TOP)/thread -I$(TOP)/poll -I$(TOP)/common
CFLAG = -g -ggdb -pg -pthread -fsanitize=address


AR_FLAG = -rcs

THREAD_SUBDIRS = $(TOP)/thread
THREAD_SUBOBJS = $(THREAD_SUBDIRS)/*.o

RPOLL_SUBDIRS = $(TOP)/poll
RPOLL_SUBOBJS = $(RPOLL_SUBDIRS)/*.o

COMMON_SUBDIRS = $(TOP)/common
COMMON_SUBOBJS = $(COMMON_SUBDIRS)/*.o

# 
OBJ = main.o httpd.o


.SUFFIXES: .cpp
.cpp.o:
	$(CC) $(INCLUDE) $(CFLAG) -c $< -o $@

main : $(THREAD_SUBDIRS) $(RPOLL_SUBDIRS) $(COMMON_SUBDIRS) $(OBJ)
	$(CC) $(INCLUDE) $(CFLAG) $(OBJ) $(THREAD_SUBOBJS) $(RPOLL_SUBOBJS) $(COMMON_SUBOBJS) -o $@

$(THREAD_SUBDIRS) : clean
	cd $@; make

$(RPOLL_SUBDIRS) : clean
	cd $@; make

$(COMMON_SUBDIRS) : clean
	cd $@; make
clean:
	-rm *.o
	-cd $(THREAD_SUBDIRS); make clean
	-cd $(RPOLL_SUBDIRS); make clean
	-cd $(COMMON_SUBDIRS); make clean


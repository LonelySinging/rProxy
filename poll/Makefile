CC = g++
CFLAG = -g -ggdb

OBJ = rpoll.o

.cpp.o:
	$(CC) $(CFLAG) -c $< -o $@

rpoll.o: *.cpp
#	$(CC) $(CFLAG) -o $@

clean:
	-rm -f *.o

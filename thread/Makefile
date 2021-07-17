CC = g++
CFLAG = -g -ggdb -pthread

OBJ = rthread.o

.cpp.o:
	$(CC) $(CFLAG) -c $< -o $@

rthread.o: *.cpp
#	$(CC) $(CFLAG) -o $@

clean:
	-rm -f *.o

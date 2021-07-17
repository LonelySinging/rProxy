CC = g++
CFLAG = -g -ggdb

OBJ = packet.o

.cpp.o:
	$(CC) $(CFLAG) -c $< -o $@

packet.o: *.cpp
#	$(CC) $(CFLAG) -o $@

clean:
	-rm -f *.o

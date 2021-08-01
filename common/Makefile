CC = g++
CFLAG = -g -ggdb

OBJ = types.o

all:$(OBJ)

.cpp.o:
	$(CC) $(CFLAG) -c $< -o $@

$(OBJ): *.cpp
	$(CC) $(CFLAG) -c $< -o $@

clean:
	-rm -f *.o

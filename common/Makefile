CC = g++
CFLAG = -g -ggdb -fsanitize=address

OBJ = types.o

all:$(OBJ)

.cpp.o:
	$(CC) $(CFLAG) -c $< -o $@

$(OBJ): *.cpp
	$(CC) $(CFLAG) -c $< -o $@

clean:
	-rm -f *.o

CC = gcc
FLAGS = -g -W -Wall
LIBS = -lm

all: input

input: input.c tree.o
	@(echo "compiling $< ...")
	@($(CC) $(FLAGS) $^ -o $@ $(LIBS))

tree.o: tree.c
	@(echo "compiling $< ...")
	@($(CC) -c $(FLAGS) $^ -o $@ $(LIBS))	

.PHONY : clean
clean:
	rm -f *.o input


CC = gcc
FLAGS = -g -W -Wall

all: input

input: input.c	        
	@(echo "compiling $< ...")
	@($(CC) $(FLAGS) $^ -o $@ $(LIBS))

.PHONY : clean
clean:
	rm -f *.o input


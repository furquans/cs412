CC = gcc
FLAGS = -g -W -Wall
LIBS = -lm

all: decision_tree random_forest

decision_tree: decision_tree.c common.o tree.o common.h tree.h
	@(echo "compiling $< ...")
	@($(CC) $(FLAGS) $^ -o $@ $(LIBS))

random_forest: random_forest.c common.o tree.o common.h tree.h
	@(echo "compiling $< ...")
	@($(CC) $(FLAGS) $^ -o $@ $(LIBS))

common.o: common.c
	@(echo "compiling $< ...")
	@($(CC) -c $(FLAGS) $^ -o $@ $(LIBS))

tree.o: tree.c
	@(echo "compiling $< ...")
	@($(CC) -c $(FLAGS) $^ -o $@ $(LIBS))	

.PHONY : clean
clean:
	rm -f *.o decision_tree random_forest


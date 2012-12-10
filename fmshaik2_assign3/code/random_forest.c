#include <stdio.h>
#include <stdlib.h>
#include "common.h"

int main(int argc,
	 char **argv)
{

	if (argc != 3) {
		printf("Usage:%s train_file test_file\n",argv[0]);
		exit(1);
	}
	
	pre_processing(argv[1],
		       argv[2]);

	create_trees(50);
	calculate_label();

	post_processing();

	return(0);
}

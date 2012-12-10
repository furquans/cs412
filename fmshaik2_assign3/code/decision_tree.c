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

	/* Do preprocessing to read data, find unique values of attributes */
	pre_processing(argv[1],argv[2]);

	/* Generate decision tree */
	create_trees(1);

	/* Calculate labels for test data */
	calculate_label();

	/* Post processing */
	post_processing();
	return(0);
}

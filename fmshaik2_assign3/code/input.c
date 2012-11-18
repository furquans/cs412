#include <stdio.h>
#include <stdlib.h>

#define MAX_STR_LENGTH 4096

int num_attr;

struct data {
	char actual_label;
	char calc_label;
	char *attr;
};

struct data *train_data, *test_data;
int train_inst, test_inst;

unsigned int calc_number_attr(char *str)
{
	char *start, *end;
	int count = 0;

	end = start = str;
	while (1) {
		strtol(start, &end, 10);
		count++;
		if (*end == '\n')
			break;
		start = end + 1;
	}
	printf("\nNumber of attributes:%d\n",count-1);
	return (count - 1);
}

struct data* get_data(FILE *fp,
		      int *inst)
{
	int count = 0, max = 0;
	char str[MAX_STR_LENGTH];
	struct data *data = NULL, *dataptr;

	/* Read one instance to identify number of attributes */
	fgets(str, MAX_STR_LENGTH, fp);

	/* Calcuate number of attributes */
	if (num_attr == 0) {
		num_attr = calc_number_attr(str);
	}

	/* Parse data to get individual tuples */
	while (1) {
		char *start, *end;
		int i = 0;

		/* Check we have sufficient memory for samples */
		if (count == max) {

			max += 4096;
			
			if (data == NULL) {
				data = malloc(max * sizeof(*data));
			} else {
				data = realloc(data, max * sizeof(*data));
			}

			if (data == NULL) {
				printf("Mem alloc error\n");
				exit(1);
			}
		}

		start = end = str;

		dataptr = data + count;
		dataptr->actual_label = strtol(start, &end, 10);
		dataptr->attr = malloc(num_attr * sizeof(char));

		while (1) {
			start = end + 1;
			dataptr->attr[i++] = (char)strtol(start, &end, 10);
			if (*end == '\n')
				break;
			if (i == num_attr) {
				printf("Ooops...overload\n");
				exit(1);
			}
		}

		count++;
		if (fgets(str, MAX_STR_LENGTH, fp) == NULL)
			break;
	}

	*inst = count;
	return data;
}

void print_data(struct data *data,
		int inst)
{
	int i;

	for (i=0; i<inst; i++) {
		int count = 0;
		struct data *dataptr = data+i;

		printf("%c%d",dataptr->actual_label>0?'+':'-',1);
		while(count < num_attr) {
			printf("\t%d",dataptr->attr[count]);
			count++;
		}
		printf("\n");
	}
}

void free_data(struct data *data,
	       int inst)
{
	int i;

	for (i=0;i<inst;i++) {
		free((data+i)->attr);
	}
	free(data);
}

struct node *gen_decision_tree()
{
}

int main(int argc,
	 char **argv)
{
	FILE *trainfp, *testfp;

	if (argc != 3) {
		printf("Usage:%s train_file test_file\n",argv[0]);
		exit(1);
	}

	trainfp = fopen(argv[1], "r");
	if (trainfp == NULL) {
		printf("File opening error\n");
		exit(1);
	}

	testfp = fopen(argv[2], "r");
	if (testfp == NULL) {
		printf("File 2 opening error\n");
		exit(1);
	}

	train_data = get_data(trainfp,&train_inst);
	test_data = get_data(testfp,&test_inst);

	/* print_data(train_data, train_inst); */
	/* print_data(test_data, test_inst); */

	printf("Number of train instances:%d\n",train_inst);
	printf("Number of test instances:%d\n",test_inst);

	dec_tree = gen_decision_tree();

	free_data(train_data,train_inst);
	free_data(test_data,test_inst);

	fclose(trainfp);
	fclose(testfp);
	return(0);
}

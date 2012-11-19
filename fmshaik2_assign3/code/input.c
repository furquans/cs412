#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "tree.h"

#define MAX_STR_LENGTH 4096

int num_attr, attr_considered;

typedef char bool;
#define TRUE 1
#define FALSE 0

unsigned long long attr_bitmap;

struct node {
        int attr;
	char *attr_split;
	char label;
};

struct data {
	char actual_label;
	char calc_label;
	char *attr;
};

struct data *train_data, *test_data;
int train_inst, test_inst;
struct tree_node *dec_tree;
int curr_attr;

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
	attr_bitmap = (1ULL << (count - 1)) - 1;
	printf("\nNumber of attributes:%d, %llx\n",count-1,attr_bitmap);
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

bool data_belongs_to_same_class(struct data *data,
				int inst)
{
	int i = 1;
	bool ret = TRUE;
	char class = data[0].actual_label;

	while (i<inst) {
		if (class != data[i].actual_label) {
			ret = FALSE;
			break;
		}
		i++;
	}
	return ret;	
}

char find_majority_class(struct data *data,
			 int inst)
{
	int countp = 0,countn = 0;
	int i = 0;

	while (i < inst) {
		if (data[i].actual_label == 1) {
			countp++;
		} else if (data[i].actual_label == -1) {
			countn++;
		}
		if ((countp*2) > inst) {
			return 1;
		} else if ((countn*2) > inst) {
			return -1;
		}
	}

	if (countp > countn)
		return 1;
	return -1;
}

int compare_data(const void *d1, const void *d2)
{
	struct data *dataptr1 = (struct data*)d1;
	struct data *dataptr2 = (struct data*)d2;

	if( dataptr1->attr[curr_attr] < dataptr2->attr[curr_attr])
		return -1;
	else if (dataptr1->attr[curr_attr] > dataptr2->attr[curr_attr])
		return 1;
	else
		return 0;
}

void sort_on_attr(struct data *data,
		  int inst,
		  int attr)
{
	curr_attr = attr;
	qsort(data,
	      inst,
	      sizeof(*data),
	      compare_data);
}

int get_next_split_point(struct data *data,
			 int inst,
			 int attr,
			 double *split)
{
	int i = 0;

	if (*split != -1) {
		while (i < inst) {
			if (data[i].attr[attr] > *split)
				break;
			i++;
		}
	}

	while (i<(inst-1)) {
		if (data[i].attr[attr] !=
		    data[i+1].attr[attr])
			break;
		i++;
	}

	if (i >= (inst-1))
		return -1;

	
	*split = (data[i].attr[attr] + data[i+1].attr[attr])/2;
	return (i+1);
}

void get_class_count(struct data *data,
		     int inst,
		     double *p,
		     double *n)
{
	int i = 0;
	double countp = 0, countn = 0;

	while (i<inst) {
		if (data[i].actual_label == 1)
			countp++;
		else if (data[i].actual_label == -1)
			countn++;
		i++;
	}
	*p = countp;
	*n = countn;
}

int find_num_of_categories(struct data *data,
			   int inst,
			   int attr)
{
	int i = 0,count = 1;

	while (i<(inst-1)) {
		if (data[i].attr[attr] !=
		    data[i+1].attr[attr])
			count++;
		i++;
	}
	return count;
}

void get_unique_values(struct data *data,
		       int inst,
		       int attr,
		       char *attr_split,
		       int *part_size,
		       int num_values)
{
	int i = 0;
	int count  = 0;
	attr_split[count] = data[0].attr[attr];
	part_size[count] = 0;

	while ((count < num_values) &&
	       (i < inst)) {
		if (data[i].attr[attr] != attr_split[count]) {
			count++;
			attr_split[count] = data[i].attr[attr];
			part_size[count] = 0;
		}
		part_size[count]++;
		i++;
	}
}

void calc_gain_ratio(struct data *data,
		     int inst,
		     int attr,
		     double *attr_gain,
		     int *num_splits)
{
	char *attr_split;
	int num_values;
	int *part_size;
	int i = 0;
	double countp, countn,total,entropy=0,info_needed=0,info_gain,split_info=0,gain_ratio;
	double *countp_part, *countn_part;
	struct data *dataptr;
	int inc=0;

	total = inst;

	/* Calculate entropy */
	get_class_count(data,inst,&countp,&countn);
	if (countp) {
		entropy += (countp/total)*log2(countp/total);
	}
	if (countn) {
		entropy += (countn/total)*log2(countn/total);
	}
	entropy = (-1)*entropy;
	printf("entropy:%f\n",entropy);

	/* Find number of unique values in the attribute */
	num_values = find_num_of_categories(data,
					    inst,
					    attr);

	/* Obtain the unique values of attributes */
	attr_split = malloc(num_values*sizeof(*attr_split));
	part_size = malloc(num_values*sizeof(*part_size));
	get_unique_values(data,inst,attr,attr_split,part_size,num_values);

	/* Obtain the counts of each class for each part */
	countp_part = malloc(num_values*sizeof(*countp_part));
	countn_part = malloc(num_values*sizeof(*countn_part));

	for (i=0;i<num_values;i++) {
		double p,n;
		dataptr = data+inc;
		get_class_count(dataptr,part_size[i],&p,&n);
		countp_part[i] = p;
		countn_part[i] = n;
		inc += part_size[i];
	}

	/* Calculate info needed */
	for (i=0;i<num_values;i++) {
		double tmp=0;
		printf("info_needed:%f,p:%f,n:%f\n",info_needed,countp_part[i],countn_part[i]);
		if (countp_part[i]) {
			tmp += countp_part[i]/part_size[i] * log2(countp_part[i]/part_size[i]);
		}
		if (countn_part[i]) {
			tmp +=  countn_part[i]/part_size[i] * log2(countn_part[i]/part_size[i]);
		}
		info_needed += (part_size[i]/total) * tmp;
	}

	info_needed = (-1)*info_needed;

	/* Calculate info gain */
	info_gain = entropy - info_needed;

	/* Calculate split info */
	for (i=0;i<num_values;i++)
		split_info += (part_size[i]/total) * log2(part_size[i]/total);
	split_info = (-1) * split_info;
	printf("split_info:%f\n",split_info);

	/* Calculate gain ratio */
	gain_ratio = info_gain / split_info;
	printf("info gain:%f\n",info_gain);
	printf("gain ratio:%f\n",gain_ratio);

	*attr_gain = gain_ratio;
	*num_splits = num_values;

	free(attr_split);
	free(part_size);
	free(countp_part);
	free(countn_part);
}

int attr_selection(struct data *data,
		   int inst,
		   int *attr)
{
	int i = 0;
	double max_gain = 0, gain = 0;
	int num_splits, max_splits = 0;

	while (i<num_attr) {
		if(attr_bitmap & (1ULL << i)) {
			/* Sort on a particular attribute */
			sort_on_attr(data,inst,i);
			/* Calculate Gain ratio */
			calc_gain_ratio(data,inst,i,&gain,&num_splits);
			if (gain > max_gain) {
				max_gain = gain;
				max_splits = num_splits;
				*attr = i;
			}
		}
		i++;
	}
	attr_considered++;
	attr_bitmap = attr_bitmap & (~(1ULL << (*attr)));
	return max_splits;
}

struct tree_node *gen_decision_tree(struct data *data,
				    int inst)
{
	struct node *tmp;
	struct tree_node *node;
	int num_split;
	int *part_size;
	struct data *dataptr;
	int inc = 0;
	int i;

	/* Step 1: Create a node N */
	tmp = malloc(sizeof(*tmp));
	if (tmp == NULL) {
		printf("Memory allocation error\n");
		exit(1);
	}

	/* Step 2: If tuples in D are all of same class C */
	if (data_belongs_to_same_class(data,inst)) {
		/* Step 3: Make the node a leaf node labeled with class C
		   and return */
		tmp->label = data[0].actual_label;
		node = create_tree_node(tmp,0);
		return node;
	}

	/* Step 4: If attribute list is empty */
	if (attr_considered == num_attr) {
		/* Step 5: Make the node a leaf node labeled with majority
		   class in D and return */
		tmp->label = find_majority_class(data, inst);
		node = create_tree_node(tmp,0);
		return node;
	}

	/* Step 6 : Attribute selection method
	   Step 7: Label node with split criteria */
	num_split = attr_selection(data, inst, &tmp->attr);
	node = create_tree_node(tmp,num_split);

	/* Sort the data on the selected attribute */
	sort_on_attr(data,inst,tmp->attr);

        /* Obtain the unique values of attributes */
        tmp->attr_split = malloc(num_split*sizeof(*(tmp->attr_split)));
        part_size = malloc(num_split*sizeof(*part_size));
        get_unique_values(data,inst,tmp->attr,tmp->attr_split,part_size,num_split);

	printf("splitting on %d with %d splits\n",tmp->attr,num_split);

	for (i=0;i<num_split;i++) {
		dataptr = data + inc;
		if (part_size[i]) {
			add_child(node, gen_decision_tree(dataptr,part_size[i]));
		} else {
			struct node *new_tmp;
			new_tmp = malloc(sizeof(*new_tmp));
			new_tmp->label = find_majority_class(data, inst);
			add_child(node, create_tree_node(new_tmp,0));
		}
	}

	return node;
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

	printf("%s Number of train instances:%d\n",__func__,train_inst);
	printf("Number of test instances:%d\n",test_inst);

	dec_tree = gen_decision_tree(train_data, train_inst);

	free_data(train_data,train_inst);
	free_data(test_data,test_inst);

	fclose(trainfp);
	fclose(testfp);
	return(0);
}

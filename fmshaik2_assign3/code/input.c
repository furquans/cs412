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
double TP,TN,FP,FN;
struct data *curr_data;

char *unique_values;
char **unique_array;

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

int get_size(struct data *data,
	     int inst,
	     int attr,
	     int val)
{
	int i;
	int count = 0;

	for (i=0; i<inst; i++) {
		if (data[i].attr[attr] == val)
			count++;
	}
	return count;
}

void get_part_size(struct data *data,
		   int inst,
		   int attr,
		   double *part_size)
{
	int i;

	for (i=0;i<unique_values[attr];i++) {
		part_size[i] = get_size(data,
					inst,
					attr,
					unique_array[attr][i]);
		printf("part_size:s%f\n",part_size[i]);
	}
}

void calc_gain_ratio(struct data *data,
		     int inst,
		     int attr,
		     double *attr_gain)
{
	int num_values;
	double *part_size;
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
	printf("------------entropy:%f-------------\n",entropy);

	/* Find number of unique values in the attribute */
	num_values = unique_values[attr];

	/* Obtain the unique values of attributes */
	part_size = malloc(num_values*sizeof(*part_size));
	get_part_size(data,inst,attr,part_size);

	/* Obtain the counts of each class for each part */
	countp_part = malloc(num_values*sizeof(*countp_part));
	countn_part = malloc(num_values*sizeof(*countn_part));

	for (i=0;i<num_values;i++) {
		if (part_size[i]) {
			double p,n;
			dataptr = data+inc;
			get_class_count(dataptr,part_size[i],&p,&n);
			countp_part[i] = p;
			countn_part[i] = n;
			inc += part_size[i];
		}
	}

	/* Calculate info needed */
	for (i=0;i<num_values;i++) {
		if (part_size[i]) {
			double tmp=0;
			printf("p:%f n:%f t:%f\n",countp_part[i],
			       countn_part[i],
			       part_size[i]);
			if (countp_part[i]) {
				tmp += countp_part[i]/part_size[i] * log2(countp_part[i]/part_size[i]);
			}
			if (countn_part[i]) {
				tmp +=  countn_part[i]/part_size[i] * log2(countn_part[i]/part_size[i]);
			}
			info_needed += (part_size[i]/total) * tmp;
		}
	}

	info_needed = (-1)*info_needed;
	printf("info needed:%f\n",info_needed);

	/* Calculate info gain */
	info_gain = entropy - info_needed;

	/* Calculate split info */
	for (i=0;i<num_values;i++) {
		if (part_size[i]) {
			split_info += (part_size[i]/total) * log2(part_size[i]/total);
		}
	}
	split_info = (-1) * split_info;
	printf("split_info:%f\n",split_info);

	/* Calculate gain ratio */
	if( info_gain ) {
		gain_ratio = info_gain / split_info;
	} else {
		gain_ratio = 0;
	}
	printf("info gain:%f\n",info_gain);
	printf("gain ratio:%f\n",gain_ratio);

	*attr_gain = gain_ratio;

	free(part_size);
	free(countp_part);
	free(countn_part);
}

void attr_selection(struct data *data,
		    int inst,
		    int *attr)
{
	int i = 0;
	double max_gain = 0, gain = 0;

	*attr = -1;
	printf("Attribute selection\n");
	while (i<num_attr) {
		if(attr_bitmap & (1ULL << i)) {
			if (unique_values[i] == 1) {
				attr_considered++;
				attr_bitmap = attr_bitmap & (~(1ULL << i));
			} else {
				/* Sort on a particular attribute */
				sort_on_attr(data,inst,i);
				printf("calc gain on %d\n",i);
				/* Calculate Gain ratio */
				calc_gain_ratio(data,inst,i,&gain);
				if (gain == 0) {
					attr_considered++;
					attr_bitmap = attr_bitmap & (~(1ULL << i));
				} else if (gain > max_gain) {
					max_gain = gain;
					*attr = i;
				}
			}
		}
		i++;
	}
	attr_considered++;
	if (*attr != -1) {
		attr_bitmap = attr_bitmap & (~(1ULL << (*attr)));
	}
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

void read_unique_array(struct data *data,
		       int attr)
{
	int i = 0;
	int count = 1;
	int val;

	val = unique_array[attr][0] = data[0].attr[attr];

	while (count < unique_values[attr]) {
		if(val != data[i].attr[attr]) {
			val = unique_array[attr][count] = data[i].attr[attr];
			count++;
		}
		i++;
	}
}

void get_unique_values(struct data *data,
		       int inst)
{
	int i;

	for (i=0; i<num_attr; i++) {
		sort_on_attr(data,inst,i);
		unique_values[i] = find_num_of_categories(data,inst,i);
		unique_array[i] = malloc(unique_values[i] * sizeof(*unique_array[i]));
		read_unique_array(data,i);
	}
}

struct tree_node *gen_decision_tree(struct data *data,
				    int inst)
{
	struct node *tmp;
	struct tree_node *node;
	int num_split;
        double *part_size;
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
		printf("creating leaf..all same class\n");
		tmp->label = data[0].actual_label;
		node = create_tree_node(tmp,0);
		return node;
	}

	/* Step 4: If attribute list is empty */
	if (attr_considered >= num_attr) {
		/* Step 5: Make the node a leaf node labeled with majority
		   class in D and return */
		printf("no attr remaining..creating leaf\n");
		tmp->label = find_majority_class(data, inst);
		node = create_tree_node(tmp,0);
		return node;
	}

	/* Step 6 : Attribute selection method
	   Step 7: Label node with split criteria */
	attr_selection(data, inst, &tmp->attr);

	if (tmp->attr == -1) {
		printf("no attr remaining..creating leaf\n");
                tmp->label = find_majority_class(data, inst);
                node = create_tree_node(tmp,0);
                return node;
	}

	num_split = unique_values[tmp->attr];

	node = create_tree_node(tmp,num_split);

	/* Sort the data on the selected attribute */
	sort_on_attr(data,inst,tmp->attr);

        /* Obtain the unique values of attributes */
        /* tmp->attr_split = malloc(num_split*sizeof(*(tmp->attr_split))); */
	tmp->attr_split = unique_array[tmp->attr];
        part_size = malloc(num_split*sizeof(*part_size));
	get_part_size(data,inst,tmp->attr,part_size);

	printf("splitting on %d with %d splits\n",tmp->attr,num_split);

	for (i=0;i<num_split;i++) {
		dataptr = data + inc;
		if (part_size[i]) {
			printf("split:adding child\n");
			add_child(node, gen_decision_tree(dataptr,part_size[i]));
		} else {
			struct node *new_tmp;
			printf("split:adding leaf\n");
			new_tmp = malloc(sizeof(*new_tmp));
			new_tmp->label = find_majority_class(data, inst);
			add_child(node, create_tree_node(new_tmp,0));
		}
		inc += part_size[i];
	}

	return node;
}

void print_unique_array()
{
	int i = 0,j;

	while (i<num_attr) {
		printf("Attr :%d - ",i);

		for (j=0;j<unique_values[i];j++) {
			printf(" %d ",unique_array[i][j]);
		}
		i++;
		printf("\n");
	}
}

int traverse(void *data,
	     int num)
{
	int i;
	struct node *node = (struct node*)data;

	if (num == 0) {
		curr_data->calc_label = node->label;
		printf("actual label:%d, calc_label:%d\n",curr_data->actual_label,
		       curr_data->calc_label);
		if (curr_data->actual_label == curr_data->calc_label) {
			if (curr_data->actual_label == 1) {
				TP++;
			} else {
				TN++;
			}
		} else {
			if (curr_data->calc_label == 1) {
				FP++;
			} else {
				FN++;
			}
		}
		return -1;
	}
	for (i=0;i<num;i++) {
		if (curr_data->attr[node->attr] == node->attr_split[i])
			break;
	}
	if (i == num) {
		printf("oops..nothing found\n");
		return -1;
	}
	return i;
}

void calculate_label(struct data *data,
		     int inst)
{
	int i;
	double accuracy, error, sensitivity, specificity, precision, f1score, fbeta0_5, fbeta2;
	double P,N;

	for (i=0;i<inst;i++) {
		curr_data = data+i;
		traverse_path(dec_tree,
			      traverse);
	}
	get_class_count(data,inst,&P,&N);

	accuracy = (TP+TN)/(P+N);
	error = (FP+FN)/(P+N);
	sensitivity = TP/P;
	specificity = TN/N;
	precision = TP/(TP+FP);
	f1score = (2 * precision * sensitivity)/(precision+sensitivity);
	fbeta0_5 = ((1+pow(0.5,2)) * precision * sensitivity) / (pow(0.5,2) * precision + sensitivity);
	fbeta2 = ((1+pow(2,2)) * precision * sensitivity) / (pow(2,2) * precision + sensitivity);

	printf("accuracy:%f\n",accuracy);
	printf("error:%f\n",error);
	printf("sensitivity:%f\n",sensitivity);
	printf("specificity:%f\n",specificity);
	printf("precision:%f\n",precision);
	printf("f1score:%f\n",f1score);
	printf("f beta with 0.5:%f\n",fbeta0_5);
	printf("f beta with 2:%f\n",fbeta2);
}

int main(int argc,
	 char **argv)
{
	FILE *trainfp, *testfp;
	int i;

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

	unique_values = malloc(num_attr*sizeof(*unique_values));
	unique_array = malloc(num_attr*sizeof(*unique_array));

	get_unique_values(train_data, train_inst);
	print_unique_array();

	dec_tree = gen_decision_tree(train_data, train_inst);
	calculate_label(test_data,test_inst);

	free_data(train_data,train_inst);
	free_data(test_data,test_inst);
	for (i=0;i<num_attr;i++) {
		free(unique_array[i]);
	}
	free(unique_array);
	free(unique_values);
		
	fclose(trainfp);
	fclose(testfp);
	return(0);
}

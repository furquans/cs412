/*
 * File : tree.c
 * Desc : Generic implementation of a multiway tree
 * Auth : Furquan Shaikh
 * Date : 11/18/2012
 */
#include<stdio.h>
#include<stdlib.h>
#include "tree.h"

struct tree_node *create_tree_node(void *data,
				   int child_num)
{
	int i;
	struct tree_node *tmp = malloc(sizeof(*tmp));
	tmp->num_child = child_num;
	tmp->data = data;
	tmp->child = malloc(child_num*sizeof(*tmp->child));
	for (i = 0; i < child_num; i++) {
		tmp->child[i] = NULL;
	}
	return tmp;
}

void add_child(struct tree_node *node,
	       struct tree_node *child)
{
	int i;
	for (i=0; i< node->num_child; i++) {
		if (node->child[i] == NULL) {
			node->child[i] = child;
			return;
		}
	}
	printf("Error: Can't have more children!!\n");
}

void traverse_path(struct tree_node *node,
		   tree_trav_fn fnptr)
{
	int ret;

	if (fnptr == NULL) {
		return;
	}

	while(node) {
		ret = fnptr(node->data,
			    node->num_child);
		if (ret == -1)
			return;
		node = node->child[ret];
	}
	fnptr(NULL,0);
}

void destroy_tree(struct tree_node *node,
		  tree_fn fnptr)
{
	if (node) {
		int i;
		for (i=0;i<node->num_child;i++) {
			destroy_tree(node->child[i],fnptr);
		}

		if (fnptr) {
			fnptr(node->data);
		}
		free(node->child);
		free(node);
	}
}


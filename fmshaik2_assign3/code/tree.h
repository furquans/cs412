/*
 * File : tree.h
 * Desc : Header file for generic implementation of a multiway tree
 * Auth : Furquan Shaikh
 * Date : 11/18/2012
 */

#ifndef __TREE_H__
#define __TREE_H__

struct tree_node {
	int num_child;
	struct tree_node **child;
	void *data;
};

typedef void (*tree_fn)(void *);
typedef int (*tree_trav_fn)(void *,int);

struct tree_node *create_tree_node(void *,int);
void add_child(struct tree_node*, struct tree_node *);
void traverse_path(struct tree_node *,tree_trav_fn);
void destroy_tree(struct tree_node *,tree_fn);

#endif //__TREE_H__

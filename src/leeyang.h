/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 *          File header goes here?
 *              In theory?
 *
 *******************************************/

#ifndef LEEYANG_H
#define LEEYANG_H

typedef struct n_qnode_s
{
    /* Base 4 'index' based on Lee-Young notation:
     *          23
     *          01
     */

    /* NOTE: For various reasons, this is easier to do as an array,
     * rather than as struct "attributes". Makes certain aspects of
     * the code simpler, but keeping the idea of which direction is
     * which is important.
     */
    struct n_qnode_s*   child[4];

    /* There should be a better way to do this, IIRC, but I haven't
     * written in C in quiiiiiiite a while. Regardless, this is used
     * if and only if the node is a leaf.
     */
    void*       data;

    /* 
     * TODO: Write a verification function that checks every node in a qtree
     *       to ensure no nodes have sub-nodes *and* a pointer to data.
     *
     * TODO: Consider writing another verification function that checks to
     *       see if all leaf nodes of a tree are on the same level.
     */
} n_qnode;

typedef struct n_qtree_s
{
    /* Canonical depth of tree, based on size given at initialisation,
     * for insertion/queries based on co-ordinates (x,y).
     *
     * Note, a depth of 0 means a null tree (a tree without any branches,
     * ergo just a leaf.) Handily, this means that a tree will have a
     * canonoical size of 2^depth by 2^depth.
     */
    unsigned int    depth;
    n_qnode*        root;
} n_qtree;

typedef struct link_node_s
{
    unsigned int            x;
    unsigned int            y;
    struct link_node_s*     n;
    void*                   data;
} link_node;

n_qnode* new_qnode(void*);
n_qtree* new_qtree(unsigned int);
link_node* new_link_node(void*, unsigned int, unsigned int);
void insert_coord(n_qtree*, void*, unsigned int, unsigned int, int);
void* query_coord(n_qtree*, unsigned int, unsigned int);
void* get_morton_lowest(n_qtree* tree);
void print_qtree_integerwise(n_qtree*, int);

long unsigned int lee_yang(n_qtree*, unsigned int, unsigned int, unsigned int,
        unsigned int);

link_node* get_dp(n_qtree*, unsigned int, unsigned int);
link_node* get_dp_mcode(n_qtree*, long unsigned int);
link_node* get_dp_rec(n_qnode*, long unsigned int, unsigned int, unsigned int);
link_node* get_morton_highest(n_qnode*);

void link_nodes_morton(n_qtree*);
void link_nodes_morton_rec(n_qnode*, link_node**);

void* query_coord_rec(n_qnode*, unsigned int, unsigned int,
        unsigned int, unsigned int);
void insert_coord_rec(n_qnode*, void*, unsigned int, unsigned int,
        unsigned int, unsigned int, int);
void* get_morton_lowest_rec(n_qnode* node);

long unsigned int get_e_from_dp(unsigned int*, unsigned int*, unsigned int,
        unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
long unsigned int get_e_from_dp_rec(long unsigned int, unsigned int*,
        unsigned int*, unsigned int, unsigned int, unsigned int, unsigned int);
long unsigned int get_fp_from_dp_e(unsigned int*, unsigned int*,
        long unsigned int, long unsigned int, unsigned int, unsigned int,
        unsigned int, unsigned int, unsigned int);
long unsigned int get_fp_from_dp(unsigned int, unsigned int, unsigned int,
        unsigned int, unsigned int, unsigned int, unsigned int);

#endif /* LEEYANG_H */

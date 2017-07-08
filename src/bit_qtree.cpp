/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 * Extends sdsl/int_vector to serve as a
 * compact, serializable quad-tree.
 *
 *******************************************/

#include "bit_qtree.hpp"

#include "morton.h"

#include <algorithm>

#define BITMASK 15

BitQTree::BitQTree() : sdsl::int_vector<4>(0,0) {}

BitQTree::BitQTree(n_qtree* tree) : BitQTree()
{
    if (tree != NULL && tree->root != NULL)
    {
        this->append_qnode(tree->root);
    }
}

n_qtree*
BitQTree::remake_tree() const
{
    n_qtree* tree = new_qtree(0);
    tree->depth = this->remake_tree(tree->root);
    return tree;
}

unsigned int
BitQTree::remake_tree(n_qnode* n) const
{
    long unsigned int pos = 0;
    return this->remake_tree(n, &pos, 0);
}

/* Proceeds along the int_vector<4> and builds a series of q_nodes.
 * params:
 *  n:      Current node to build down from
 *  pos:    Position currently considered in the int_vector
 *  m:      Morton code of the current tree node
 *  returns cumulative distance from the lowest child leaf
 */
unsigned int
BitQTree::remake_tree(n_qnode* n, long unsigned int* pos, long unsigned int m)
    const
{
    /* a is a 4-bit int. Each bit indicates the existence of a child on that
     * branch (in order, from MSB to LSB, of 3, 2, 1, 0). Thus, if one proceeds
     * down to the leaves in Morton order, as that is the order that the
     * branches were recorded in this->append_qnode(), the tree can be
     * reconstructed.
     */
    unsigned int a = (*this)[(*pos)++];
    long unsigned int x, y;
    unsigned int d[] = {0,0,0,0,0}; // d[4] is a flag thing because sure why
                                    // not just make things weird like that
    if (a & 1)
    {
        n->child[0] = new_qnode(NULL);
        d[0] = this->remake_tree(n->child[0], pos, (m<<2)+0);
        d[4] = 1;
    }
    if (a & 2)
    {
        n->child[1] = new_qnode(NULL);
        d[1] = this->remake_tree(n->child[1], pos, (m<<2)+1);
        d[4] = 1;
    }
    if (a & 4)
    {
        n->child[2] = new_qnode(NULL);
        d[2] = this->remake_tree(n->child[2], pos, (m<<2)+2);
        d[4] = 1;
    }
    if (a & 8)
    {
        n->child[3] = new_qnode(NULL);
        d[3] = this->remake_tree(n->child[3], pos, (m<<2)+3);
        d[4] = 1;
    }
    if (a == 0)
    {
        morton_ZtoP(m, &x, &y);
        n->data = new_link_node(NULL, x, y);
    }
    return (*std::max_element(d, d+4) + d[4]);
}

sdsl::int_vector<4>::size_type
BitQTree::serialize(std::ostream& out)
{
    this->truncate_vec();
    return sdsl::int_vector<4>::serialize(out);
}

/*
 */
void
BitQTree::load(std::istream& in)
{
    sdsl::int_vector<4>::load(in);
    this->length = this->size();
    return;
}

void
BitQTree::double_size()
{
    // 2*0 = 1
    if (this->empty())
    {
        this->resize(1);
    }
    else
    {
        this->resize(2*this->size());
    }
    return;
}

void
BitQTree::truncate_vec()
{
    this->resize(this->length);
    return;
}

void
BitQTree::append_int(unsigned int a)
{
    a &= BITMASK;
    if (this->length + 1 >= this->size())
    {
        this->double_size();
    }
    (*this)[this->length++] = a;
    return;
}

void
BitQTree::append_qnode(n_qnode* n)
{
    unsigned int a = 0;
    if (n == NULL)
    {
        return;
    }
    if (n->child[0] != NULL) {a+=1;}
    if (n->child[1] != NULL) {a+=2;}
    if (n->child[2] != NULL) {a+=4;}
    if (n->child[3] != NULL) {a+=8;}
    this->append_int(a);
    if (n->child[0] != NULL) {this->append_qnode(n->child[0]);}
    if (n->child[1] != NULL) {this->append_qnode(n->child[1]);}
    if (n->child[2] != NULL) {this->append_qnode(n->child[2]);}
    if (n->child[3] != NULL) {this->append_qnode(n->child[3]);}
    return;
}

/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 * Extends sdsl/int_vector to serve as a
 * compact, serializable quad-tree.
 *
 *******************************************/

#ifndef OFFSET_QTREE_HPP
#define OFFSET_QTREE_HPP

#include <stdio.h>
#include <iostream>
#include <vector>

#include "morton.h"

typedef uint64_t size_type;

//! A class serving as an immutable QTree, serializable to disk.
/*! \author James Denholm
 *  
 *      Interface based on sdsl::int_vector by Simon Gog.
 *
 *      This implementation uses nodes storing integer offsets
 *      referencing an internal array of nodes, rather than
 *      pointers. Effectively a vector of nodes.
 *
 *      For technical reasons, can store at most (maximum value
 *      of int_type) - 1 nodes.
 *
 *  \tparam int_type Type of integer to use for offsets in nodes.
 *          One would be well advised to use an unsigned integer
 *          type.
 */
template <class int_type = size_type>
class
OffsetQTree
{
    public:
        typedef struct oqt_node
        {
            /* Base 4 'index' based on Lee-Young notation:
             *          23
             *          01
             */
            int_type    child[4];
        } oqt_node;

    private:
        std::vector<oqt_node>   vec;

        size_type   length  = 0;
        size_type   c_depth = 0;

    public:
        OffsetQTree() : vec(std::vector<oqt_node>(0)) {};
        OffsetQTree(BitQTree* tree)
            : vec(std::vector<oqt_node>(tree->len()))
        {
            this->append_bitqtree(tree);
            this->fix_c_depth();
            printf("Depth of new OffsetQTree: %lu\n", this->c_depth);
        };

        //TODO: Work out if this needs to actually be public or not.
        //      If not, oqt_node can be made private.
        oqt_node& operator[](size_type i);

        unsigned char query_coord(size_type x, size_type y);
        void pprint();

    private:
        /* Append a node, if possible, and return the index of that node.
         *
         * If appending fails (for whatever reason), returns
         * OffsetQTree->size().
         */
        size_type append_node();
        size_type append_node(size_type c0, size_type c1, size_type c2, 
                size_type c3);

        /* Append the content of a BitQTree, treated as a quadtree.
         */
        void append_bitqtree(BitQTree* tree);
        void append_bitqtree(BitQTree* tree, size_type* i);

        /* See if the vector needs to be resized in order to take num more
         * elements, and if so, resizes the vector.
         */
        void check_fix_size(size_type num = 1);

        /* Run through the tree and work out what the canonical depth is.
         */
        void fix_c_depth();
};

template<class int_type>
typename OffsetQTree<int_type>::oqt_node&
OffsetQTree<int_type>::operator[](size_type i)
{
    return this->vec[i];
}

/* Return vals reserved for the possibility of incorporating some feedback
 * about errors later (eg -- not being able to resize the vector).
 */
template<class int_type>
void
OffsetQTree<int_type>::check_fix_size(size_type num)
{
    if (this->length + num <= this->vec.size())
    {
        return;
    }
    this->vec.resize((2*this->vec.size() > this->length + num)?
            2*this->vec.size():this->length+num);
    return;
}

template<class int_type>
size_type
OffsetQTree<int_type>::append_node()
{
    return this->append_node(0, 0, 0, 0);
}

template<class int_type>
size_type
OffsetQTree<int_type>::append_node(size_type c0, size_type c1, size_type c2,
        size_type c3)
{
    this->check_fix_size();
    (*this)[this->length].child[0] = c0;
    (*this)[this->length].child[1] = c1;
    (*this)[this->length].child[2] = c2;
    (*this)[this->length].child[3] = c3;
    return this->length++;
}

template<class int_type>
void
OffsetQTree<int_type>::append_bitqtree(BitQTree* tree)
{
    size_type i = 0;
    this->append_bitqtree(tree, &i);
}

template<class int_type>
void
OffsetQTree<int_type>::append_bitqtree(BitQTree* tree, size_type* i)
{
    unsigned int node_flags = (*tree)[*i];
    size_type self = this->append_node();
    if (node_flags & 1)
    {
        *i += 1;
        (*this)[self].child[0] = *i;
        this->append_bitqtree(tree, i);
    }
    if (node_flags & 2)
    {
        *i += 1;
        (*this)[self].child[1] = *i;
        this->append_bitqtree(tree, i);
    }
    if (node_flags & 4)
    {
        *i += 1;
        (*this)[self].child[2] = *i;
        this->append_bitqtree(tree, i);
    }
    if (node_flags & 8)
    {
        *i += 1;
        (*this)[self].child[3] = *i;
        this->append_bitqtree(tree, i);
    }
}

template<class int_type>
void
OffsetQTree<int_type>::fix_c_depth()
{
    size_type i;
    this->c_depth = this->length;
    for (i = 0; i < this->length; i++)
    {
        if ((*this)[i].child[0] != 0) { continue; }
        if ((*this)[i].child[1] != 0) { continue; }
        if ((*this)[i].child[2] != 0) { continue; }
        if ((*this)[i].child[3] != 0) { continue; }
        this->c_depth = i;
        break;
    }
}

template<class int_type>
unsigned char
OffsetQTree<int_type>::query_coord(size_type x, size_type y)
{
    size_type depth;
    int_type i;
    uint64_t m;
    unsigned char c;

    depth = 0;
    i = 0;
    morton_PtoZ(x, y, &m);
    for (depth = 0; depth < this->c_depth; depth++)
    {
        c = (m >> ((this->c_depth - (depth + 1)) * 2)) & 3;
        if ((*this)[i].child[c] == 0)
        {
            return 0;
        }
        if ((this->c_depth != 0) && (depth < (this->c_depth - 1)))
        {
            i = (*this)[i].child[c];
        }
    }
    return (i != 0) ? 1 : 0;
}

template<class int_type>
void
OffsetQTree<int_type>::pprint()
{
    size_type x, y;

    y = (size_type)(1 << this->c_depth);
    do
    {
        for (x = 0; x < (size_type)(1 << this->c_depth); x++)
        {
            printf("%d ", this->query_coord(x, y));
        }
        printf("\n");
    }
    while (y-- > 0);
}

//TODO: Serialize function

#endif /* OFFSET_QTREE_HPP */

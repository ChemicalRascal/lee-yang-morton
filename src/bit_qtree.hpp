/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 * Extends sdsl/int_vector to serve as a
 * compact, serializable quad-tree.
 *
 *******************************************/

#ifndef BIT_QTREE_HPP
#define BIT_QTREE_HPP

#include <stdio.h>
#include <iostream>

#include <sdsl/int_vector.hpp>

#include "leeyang.hpp"  // n_qnode

class BitQTree : public sdsl::int_vector<4>
{
    private:
        unsigned int length = 0;

    public:
        BitQTree();
        BitQTree(n_qtree* tree);
        n_qtree* remake_tree() const;
        sdsl::int_vector<4>::size_type serialize(std::ostream& out);
        void load(std::istream& in);
        unsigned int len();

    private:
        unsigned int remake_tree(n_qnode* n) const;
        unsigned int remake_tree(n_qnode* n, long unsigned int* pos, long
                unsigned int m) const;
        void double_size();
        void truncate_vec();
        void append_int(unsigned int a);
        void append_qnode(n_qnode* n);
};

#endif /* BIT_QTREE_HPP */

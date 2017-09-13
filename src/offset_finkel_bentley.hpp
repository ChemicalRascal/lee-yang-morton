/* vim: set et sts=4 sw=4 cc=80 tw=80: */ 
/*******************************************
 *
 * Implements Finkel & Bentley's quadtree,
 * in a manner that can be written to disk.
 *
 *******************************************/

#ifndef OFFSET_FINKEL_BENTLEY_HPP
#define OFFSET_FINKEL_BENTLEY_HPP

#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <tuple>
#include <algorithm>

typedef uint64_t size_type;

// TODO RANGE SEARCH
// TODO FIX UP QUERY RETURN VAR?

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
template <class int_type = size_type, class attr_type = long unsigned int>
class
OffsetFBTree
{
    public:
        typedef std::tuple<attr_type, attr_type> key_type;
        typedef std::vector<key_type> vec_type;
        typedef struct ofb_node
        {
            /* Base 4 'index' based on Lee-Young notation:
             *          23
             *          01
             */
            int_type    child[4];
            key_type    key;
        } ofb_node;

    private:
        std::vector<ofb_node>   vec;
        size_type               length  = 0;
        attr_type               min_x;
        attr_type               max_x;
        attr_type               min_y;
        attr_type               max_y;

    public:
        OffsetFBTree() : vec(std::vector<ofb_node>(0)) {};
        OffsetFBTree(vec_type& key_vec)
        {
            this->append_keys_optimal(key_vec);
            this->truncate_vec();
        };

        OffsetFBTree<int_type, attr_type>::ofb_node& operator[](int_type i);

        /* Append the contents of a vector of keys
         */
        void append_keys(vec_type& keys);
        int_type append_keys_optimal(vec_type& keys);

        int_type append_key(key_type& key);
        int_type query_key(key_type& key);
        int_type query_coord(attr_type& x, attr_type& y);
        void pprint();
        void pprint(int_type depth, int_type offset);

        size_type count_nodes(int_type offset);

        void serialize(std::ostream& out);
        void load(std::istream& in);

        size_type range_count(attr_type x1, attr_type x2, attr_type y1,
                attr_type y2);

    private:
        /* Append a node, if possible, and return the index of that node.
         *
         * If appending fails (for whatever reason), returns
         * OffsetFBTree->size().
         */
        int_type append_node(key_type& key);
        int_type append_node(key_type key, int_type c0, int_type c1,
                int_type c2, int_type c3);

        size_type range_count(int_type offset, attr_type x1, attr_type x2,
                attr_type y1, attr_type y2);

        /* See if the vector needs to be resized in order to take num more
         * elements, and if so, resizes the vector.
         */
        void check_fix_size(size_type num = 1);

        void truncate_vec();

        /* Compare two keys.
         */
        unsigned int compare(key_type& a, key_type& b);

};

/* Returns the subtree of b that a would, in theory, be in.
 *
 * Returns 4 if the keys are equal. Otherwise:
 * a.x <= b.x:
 *   a.y <= b.y: 0
 *   a.y >  b.y: 2
 * a.x >  b.x:
 *   a.y <= b.y: 1
 *   a.y >  b.y: 3
 *
 * Ergo, z-order, with bias towards 0 on borders.
 */
template<class int_type, class attr_type>
unsigned int
OffsetFBTree<int_type, attr_type>::compare(key_type& a, key_type& b)
{
    if (std::get<0>(a) == std::get<0>(b) && std::get<1>(a) == std::get<1>(b))
    {
        return 4;
    }
    if (std::get<0>(a) <= std::get<0>(b))
    {
        if (std::get<1>(a) <= std::get<1>(b))
        {
            return 0;
        }
        return 2;
    }
    if (std::get<1>(a) <= std::get<1>(b))
    {
        return 1;
    }
    return 3;
}

template<class int_type, class attr_type>
typename OffsetFBTree<int_type, attr_type>::ofb_node&
OffsetFBTree<int_type, attr_type>::operator[](int_type i)
{
    return this->vec[i];
}

/* Return vals reserved for the possibility of incorporating some feedback
 * about errors later (eg -- not being able to resize the vector).
 */
template<class int_type, class attr_type>
void
OffsetFBTree<int_type, attr_type>::check_fix_size(size_type num)
{
    if (this->length + num <= this->vec.size())
    {
        return;
    }
    this->vec.resize((2*this->vec.size() > this->length + num)?
            2*this->vec.size():this->length+num);
    return;
}

template<class int_type, class attr_type>
void
OffsetFBTree<int_type, attr_type>::truncate_vec()
{
    this->vec.resize(this->length);
    return;
}

/* Pass through reference to key, as the full function will copy the key
 * argument
 */
template<class int_type, class attr_type>
int_type
OffsetFBTree<int_type, attr_type>::append_node(key_type& key)
{
    if (this->length == 0 || std::get<0>(key) < this->min_x)
        this->min_x = std::get<0>(key);
    if (this->length == 0 || std::get<0>(key) > this->max_x)
        this->max_x = std::get<0>(key);
    if (this->length == 0 || std::get<1>(key) < this->min_y)
        this->min_y = std::get<1>(key);
    if (this->length == 0 || std::get<1>(key) > this->max_y)
        this->max_y = std::get<1>(key);
    return this->append_node(key, 0, 0, 0, 0);
}

template<class int_type, class attr_type>
int_type
OffsetFBTree<int_type, attr_type>::append_node(key_type key, int_type c0,
        int_type c1, int_type c2, int_type c3)
{
    this->check_fix_size();
    (*this)[this->length].key = key;
    (*this)[this->length].child[0] = c0;
    (*this)[this->length].child[1] = c1;
    (*this)[this->length].child[2] = c2;
    (*this)[this->length].child[3] = c3;
    return this->length++;
}

/* Returns the index to the key if it is in the tree, otherwise 0.
 *
 * FIXME: This doesn't work conceptually, as the index to the root node is 0.
 *        Find a better way to do this.
 */
template<class int_type, class attr_type>
int_type
OffsetFBTree<int_type, attr_type>::query_key(key_type& key)
{
    unsigned char c;
    int_type i;

    if (this->length == 0)
    {
        return 0;
    }

    i = 0;
    c = this->compare(key, (*this)[0].key);
    while (c != 4)
    {
        i = (*this)[i].child[c];
        if (i == 0)
        {
            // Tried to go into a non-existant tree.
            return 0;
        }
        c = this->compare(key, (*this)[i].key);
    }

    return (c == 4) ? i : 0;
}

template<class int_type, class attr_type>
size_type
OffsetFBTree<int_type, attr_type>::range_count(attr_type x1, attr_type x2,
        attr_type y1, attr_type y2)
{
    if (x2 < min_x || max_x < x1 || y2 < min_y || max_y < y1) return 0;
    return this->range_count(0, x1, x2, y1, y2);
}

template<class int_type, class attr_type>
size_type
OffsetFBTree<int_type, attr_type>::range_count(int_type offset,
        attr_type x1, attr_type x2, attr_type y1, attr_type y2)
{
    int_type count = 0;
    key_type key;
    // InRegion
    auto ir = [x1, x2, y1, y2](const key_type& key)
    {
        return ((std::get<0>(key) >= x1) && (std::get<0>(key) <= x2) &&
            (std::get<1>(key) >= y1) && (std::get<1>(key) <= y2));
    };

    key = (*this)[offset].key;
    if (ir(key)) { /*Output*/ count++; }

    if ((*this)[offset].child[0] != 0 &&
            x1 <= std::get<0>(key) && y1 <= std::get<1>(key))
    { count += this->range_count((*this)[offset].child[0], x1, x2, y1, y2); }

    if ((*this)[offset].child[1] != 0 &&
            (
                (std::get<0>(key) <  x2 && y1 <= std::get<1>(key))
                //FIXME x eq is here because F&B got it wrong, and their
                //      optimised construction assumes that nothing has
                //      the same x-value as the midpoint.
             || (std::get<0>(key) == x2 && y1 <= std::get<1>(key))
            )
       )
    { count += this->range_count((*this)[offset].child[1], x1, x2, y1, y2); }

    if ((*this)[offset].child[2] != 0 &&
            x1 <= std::get<0>(key) && std::get<1>(key) <  y2)
    { count += this->range_count((*this)[offset].child[2], x1, x2, y1, y2); }

    if ((*this)[offset].child[3] != 0 &&
            (
                (std::get<0>(key) <  x2 && std::get<1>(key) <  y2)
                //FIXME x eq is here because F&B got it wrong, and their
                //      optimised construction assumes that nothing has
                //      the same x-value as the midpoint.
             || (std::get<0>(key) == x2 && std::get<1>(key) <  y2)
            )
       )
    { count += this->range_count((*this)[offset].child[3], x1, x2, y1, y2); }

    return count;
}

template<class int_type, class attr_type>
size_type
OffsetFBTree<int_type, attr_type>::count_nodes(int_type offset)
{
    int_type count = 1;

    if ((*this)[offset].child[0] != 0)
    { count += this->count_nodes((*this)[offset].child[0]); }
    if ((*this)[offset].child[1] != 0)
    { count += this->count_nodes((*this)[offset].child[1]); }
    if ((*this)[offset].child[2] != 0)
    { count += this->count_nodes((*this)[offset].child[2]); }
    if ((*this)[offset].child[3] != 0)
    { count += this->count_nodes((*this)[offset].child[3]); }

    return count;
}

/* Returns the index to the key (regardless of if it was actually inserted or
 * just found).
 */
template<class int_type, class attr_type>
int_type
OffsetFBTree<int_type, attr_type>::append_key(key_type& key)
{
    unsigned char c;
    int_type i, j;

    if (this->vec.size() == 0)
    {
        this->append_node(key);
        return 0;
    }

    i = 0;
    c = this->compare(key, (*this)[i].key);
    while (c != 4)
    {
        j = (*this)[i].child[c];
        if (j == 0)
        {
            j = this->append_node(key);
            return ((*this)[i].child[c] = j);
        }
        i = j;
        c = this->compare(key, (*this)[i].key);
    }

    return (c == 4) ? i : 0;
}

/* Append a vector of keys. Woo.
 */
template<class int_type, class attr_type>
void
OffsetFBTree<int_type, attr_type>::append_keys(vec_type& keys)
{
    typename vec_type::iterator key_i;
    for (key_i = keys.begin(); key_i != keys.end(); key_i++)
    {
        this->append_key(*key_i);
    }
}

/* It's a complete rewrite of append_keys_optimal_old().
 */
template<class int_type, class attr_type>
int_type
OffsetFBTree<int_type, attr_type>::append_keys_optimal(vec_type& keys)
{
    vec_type c_high, c_low;
    typename vec_type::iterator::difference_type m;
    typename vec_type::iterator r_itr;
    int_type r_offset;
    size_type i;
    int_type offsets[4] = {0, 0, 0, 0};

    std::sort(keys.begin(), keys.end());
    m = keys.size()/2;
    r_offset = this->append_node(keys[m]);

    c_high = vec_type();
    c_low = vec_type();
    for (i = 0; i < (size_type)m; i++)
    {
        if (std::get<1>(keys[i]) > std::get<1>(keys[m]))
            // C2
            c_high.push_back(keys[i]);
        else
            // C0
            c_low.push_back(keys[i]);
    }
    if (c_low.size() > 0)
        offsets[0] = append_keys_optimal(c_low);
    if (c_high.size() > 0)
        offsets[2] = append_keys_optimal(c_high);

    c_high = vec_type();
    c_low = vec_type();
    for (i = m+1; i < keys.size(); i++)
    {
        if (std::get<1>(keys[i]) > std::get<1>(keys[m]))
            // C1
            c_high.push_back(keys[i]);
        else
            // C3
            c_low.push_back(keys[i]);
    }
    if (c_low.size() > 0)
        offsets[1] = append_keys_optimal(c_low);
    if (c_high.size() > 0)
        offsets[3] = append_keys_optimal(c_high);

    (*this)[r_offset].child[0] = offsets[0];
    (*this)[r_offset].child[1] = offsets[1];
    (*this)[r_offset].child[2] = offsets[2];
    (*this)[r_offset].child[3] = offsets[3];

    return r_offset;
}

template<class int_type, class attr_type>
int_type
OffsetFBTree<int_type, attr_type>::query_coord(attr_type& x, attr_type& y)
{
    return this->query_key(key_type(x, y));
}

template<class int_type, class attr_type>
void
OffsetFBTree<int_type, attr_type>::pprint()
{
    if (this->length != 0)
    {
        this->pprint(0,0);
    }
    std::cout << this->count_nodes(0) << std::endl;
    std::cout << this->length << std::endl;
    std::cout << this->vec.size() << std::endl;
}

template<class int_type, class attr_type>
void
OffsetFBTree<int_type, attr_type>::pprint(int_type depth, int_type offset)
{
    int_type i;

    std::cout
        << std::get<0>((*this)[offset].key) << ", "
        << std::get<1>((*this)[offset].key)
        << std::endl;
    if ((*this)[offset].child[0] != 0)
    {
        for (i = 0; i < (depth*2); i++)
            std::cout << " ";
        std::cout << "0:";
        this->pprint(depth + 1, (*this)[offset].child[0]);
    }
    if ((*this)[offset].child[1] != 0)
    {
        for (i = 0; i < (depth*2); i++)
            std::cout << " ";
        std::cout << "1:";
        this->pprint(depth + 1, (*this)[offset].child[1]);
    }
    if ((*this)[offset].child[2] != 0)
    {
        for (i = 0; i < (depth*2); i++)
            std::cout << " ";
        std::cout << "2:";
        this->pprint(depth + 1, (*this)[offset].child[2]);
    }
    if ((*this)[offset].child[3] != 0)
    {
        for (i = 0; i < (depth*2); i++)
            std::cout << " ";
        std::cout << "3:";
        this->pprint(depth + 1, (*this)[offset].child[3]);
    }
}

template<class int_type, class attr_type>
void
OffsetFBTree<int_type, attr_type>::serialize(std::ostream& out)
{
    this->truncate_vec();
    out.write((char*)&this->length, sizeof(size_type))
        .write((char*)&this->min_x, sizeof(attr_type))
        .write((char*)&this->max_x, sizeof(attr_type))
        .write((char*)&this->min_y, sizeof(attr_type))
        .write((char*)&this->max_y, sizeof(attr_type))
        .write((char*)this->vec.data(), sizeof(ofb_node) * this->length);
}

template<class int_type, class attr_type>
void
OffsetFBTree<int_type, attr_type>::load(std::istream& in)
{
    in.read((char*)&this->length, sizeof(size_type))
        .read((char*)&this->min_x, sizeof(attr_type))
        .read((char*)&this->max_x, sizeof(attr_type))
        .read((char*)&this->min_y, sizeof(attr_type))
        .read((char*)&this->max_y, sizeof(attr_type));
    this->check_fix_size(this->length);
    in.read((char*)this->vec.data(), sizeof(ofb_node) * this->length);
    this->truncate_vec();
}

#endif /* OFFSET_FINKEL_BENTLEY_HPP */

/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 * Extends sdsl/k2_tree to support range queries
 *
 *******************************************/

#ifndef K2_RANGE_HPP
#define K2_RANGE_HPP

#include <sdsl/k2_tree.hpp>
#include <sdsl/bit_vectors.hpp>

typedef uint64_t size_type;
typedef int64_t sign_type;

template<uint8_t k,
    typename t_bv=sdsl::bit_vector,
    typename t_rank=typename t_bv::rank_1_type>
class
k2_range : public sdsl::k2_tree<k, t_bv, t_rank>
{
    public:
        using sdsl::k2_tree<k, t_bv, t_rank>::k2_tree;
        size_type range_count(size_type p1, size_type p2, size_type q1,
                size_type q2);
    //private:
        void range_count(size_type n, size_type p1, size_type p2,
                size_type q1, size_type q2,// size_type dp, size_type dq,
                sign_type z, size_type* count);
};

/* size_type size: As was passed to the edges-based constructor.
 */
template<uint8_t k, typename t_bv, typename t_rank>
size_type
k2_range<k, t_bv, t_rank>::range_count(size_type p1, size_type p2,
        size_type q1, size_type q2)
{
    size_type c;
    size_type n, p1a, p2a, q1a, q2a;

    c = 0;
    if (this->k_l.size() == 0 && this->k_t.size() == 0)
    {
        return c;
    }

    // n: The size of each child subgrid, not *this* grid.
    n = static_cast<size_type>(std::pow(this->k_k, this->k_height))/this->k_k;
    size_type y = 0;

    for (unsigned i = std::floor(p1/n); i <= std::floor(p2/n); i++)
    {
        if (i == std::floor(p1/n)) { p1a = p1 % n; }
        else { p1a = 0; }
        if (i == std::floor(p2/n)) { p2a = p2 % n; }
        else { p2a = n - 1; }
        for (unsigned j = std::floor(q1/n); j <= std::floor(q2/n); j++)
        {
            if (j == std::floor(q1/n)) { q1a = q1 % n; }
            else { q1a = 0; }
            if (j == std::floor(q2/n)) { q2a = q2 % n; }
            else { q2a = n - 1; }
            range_count(n/this->k_k, p1a, p2a, q1a, q2a,
                    //TODO: Get dp, dq working for acc{} version
                    // dp+n*i, dq+n*j,
                    y+this->k_k*i+j, &c);
        }
    }
    return c;
}

/* Brisaboa-Ladra-Navarro's range algorithm.
 *
 * One additional param: size_type* count, which keeps track of how many leaves
 * have been seen.
 */
template<uint8_t k, typename t_bv, typename t_rank>
void
k2_range<k, t_bv, t_rank>::range_count(size_type n, size_type p1,
        size_type p2, size_type q1, size_type q2,// size_type dp, size_type dq,
        sign_type z, size_type* count)
{
    size_type y, i, j, p1a, p2a, q1a, q2a;
    if (z >= (sign_type)this->k_t.size())
    {
        if (this->k_l[z - this->k_t.size()] == 1)
        {
            //Output
            (*count) += 1;
        }
    }
    else
    {
        if (this->k_t[z] == 1)
        {
            //This only works with changes to sdsl::k2_tree -- changing private
            // methods and attributes to public (or protected)
            y = this->k_t_rank(z+1) * std::pow(this->k_k, 2);
            //FIXME: When n < k, arithmetic error. Need to work out what the
            //code is *supposed* to do.
            for (i = std::floor(p1/n); i <= std::floor(p2/n); i++)
            {
                if (i == std::floor(p1/n)) { p1a = p1 % n; }
                else { p1a = 0; }
                if (i == std::floor(p2/n)) { p2a = p2 % n; }
                else { p2a = n-1; }
                for (j = std::floor(q1/n); j <= std::floor(q2/n); j++)
                {
                    if (j == std::floor(q1/n)) { q1a = q1 % n; }
                    else { q1a = 0; }
                    if (j == std::floor(q2/n)) { q2a = q2 % n; }
                    else { q2a = n-1; }
                    range_count(n/this->k_k, p1a, p2a, q1a, q2a,
                            //TODO: Get dp, dq working for acc{} version
                            //dp+(n/this->k_k)*i, dq+(n/k)*j,
                            y+k*i+j, count);
                }
            }
        }
    }
}

/*
template<uint8_t k, typename t_bv, typename t_rank>
void
k2_range<k, t_bv, t_rank>::pprint()
{
}
*/

#endif /* K2_RANGE_HPP */

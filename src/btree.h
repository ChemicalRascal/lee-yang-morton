/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 * A really quick-and-dirty binary tree lib.
 *
 *******************************************/

#ifndef BTREE_H
#define BTREE_H

typedef struct btree_s
{
    /* Base 2 'index', mimicking leeyang.h. 0:L, 1:R.
     *
     * We don't need no data pointer! Existence of a node with two NULL
     * children indicates a leaf, and we're only concerned at the moment
     * with existence.
     */
    struct btree_s*     child[2];
} btree;

btree* new_btree();
void free_btree(btree* tree);

#endif /* BTREE_H */

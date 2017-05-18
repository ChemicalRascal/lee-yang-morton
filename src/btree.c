/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 *          File header goes here?
 *              In theory?
 *
 *******************************************/

#include "btree.h"
#include <stdlib.h>
#include <assert.h>

/* Makes a btree node.
 */
btree*
new_btree()
{
    btree* tree;
    tree = malloc(sizeof(btree));
    assert(tree != NULL);
    tree->child[0]  = NULL;
    tree->child[1]  = NULL;
    return tree;
}

/* Frees a given btree node and all subtrees.
 */
void
free_btree(btree* tree)
{
    if (tree != NULL)
    {
        free_btree(tree->child[0]);
        free_btree(tree->child[1]);
    }
    free(tree);
    return;
}

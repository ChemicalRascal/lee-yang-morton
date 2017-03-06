/* vim: set et sts=4 sw=4 cc=80 tw=80: */

/*******************************************
 *
 *          File header goes here?
 *              In theory?
 *
 *******************************************/

typedef struct qsiseq_s
{
    bitseq* hi; // High-bits sequence
    bitseq* lo; // Low-bits sequence
} qsiseq;

qsiseq* new_qsiseq();

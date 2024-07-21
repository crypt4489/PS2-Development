#ifndef PS_AVLTREE_H
#define PS_AVLTREE_H

#include "ps_global.h"

AVLTree *CreateAVLNode(void *data, int val);
AVLTree *InsertAVLNode(AVLTree *tree, AVLTree *node);
AVLTree *DeleteAVLNode(AVLTree *tree, int val);
AVLTree *GetAVLNode(AVLTree *tree, int val);


#endif
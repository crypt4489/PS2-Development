#include "util/ps_avltree.h"

#include <stdlib.h>
static int max(int a, int b)
{
    if (a > b)
        return a;
    return b;
}

static int HeightOfTree(AVLTree *node)
{
    if (!node)
        return 0;

    return node->height;
}

static int BalanceOfTree(AVLTree *node)
{
    if (!node)
        return 0;
    return HeightOfTree(node->left) - HeightOfTree(node->right);
}

static AVLTree* RightRotate(AVLTree *tree)
{
    AVLTree *x = tree->left;
    AVLTree *y = x->right;
    x->right = tree;
    tree->left = y;
    tree->height = 1 + 
                    max(HeightOfTree(tree->left), 
                    HeightOfTree(tree->right));
    x->height = 1 + 
                    max(HeightOfTree(x->left), 
                    HeightOfTree(x->right));
    return x;
}

static AVLTree* LeftRotate(AVLTree *tree)
{
    AVLTree *x = tree->right;
    AVLTree *y = x->left;
    x->left= tree;
    tree->right = y;
    tree->height = 1 + 
                    max(HeightOfTree(tree->left), 
                    HeightOfTree(tree->right));
    x->height = 1 + 
                    max(HeightOfTree(x->left), 
                    HeightOfTree(x->right));

    return x;
}

AVLTree *CreateAVLNode(void *data, int val)
{
    AVLTree *node = (AVLTree*)malloc(sizeof(AVLTree));
    if (!node) return NULL;
    node->node = val;
    node->left = node->right = NULL;
    node->data = data;
    node->height = 1;
    return node; 
}
AVLTree *InsertAVLNode(AVLTree *tree, AVLTree *node)
{

    if (!tree)
    {
        tree = node;
        return tree;
    }

    u64 val = node->node;
    u64 comp = tree->node;
    
    if (val > comp)
    {
        tree->right = InsertAVLNode(tree->right, node);
    } else if (val < comp) {
        tree->left = InsertAVLNode(tree->left, node);
    } else {
        node->height = tree->height;
        node->left = tree->left;
        tree->left = node;
    }

    tree->height =  1 + 
                    max(HeightOfTree(tree->left), 
                    HeightOfTree(tree->right));

    int balanceOfTree = BalanceOfTree(tree);

    if (balanceOfTree > 1 && val <= tree->left->node)
    {
        return RightRotate(tree);
    }

    if (balanceOfTree < -1 && val > tree->right->node)
    {
        return LeftRotate(tree);
    }

    if (balanceOfTree > 1 && val > tree->left->node)
    {
        tree->left = LeftRotate(tree->left);
        return RightRotate(tree);

    }

    if (balanceOfTree < -1 && val < tree->right->node)
    {
        tree->right = RightRotate(tree->right);
        return LeftRotate(tree);
    }

    return tree;
}
AVLTree *DeleteAVLNode(AVLTree *tree, int val)
{

    if(!tree)
        return tree;


    int comp = tree->node;


    if (comp > val)
    {
        tree->left = DeleteAVLNode(tree->left, val);
    }

    if (comp < val)
    {
        tree->right = DeleteAVLNode(tree->right, val);
    }
    else 
    {
        
        AVLTree *temp = NULL;
        if (!tree->left || !tree->right)
        {
            if (tree->left)
            {
                temp = tree->left;
            } 
            else 
            {
                temp = tree->right;
            }

            if (!temp)
            {
                temp = tree;
                tree = NULL;
            } 
            else
            {
                *tree = *temp;
            }

            free(temp);
        } 
        
        else 
        {
            temp = tree->right;
            while(temp->left)
            {
                temp = temp->left;
            }

            tree->node = temp->node;
            tree->data = temp->data;

            tree->right = DeleteAVLNode(tree->right, temp->node);
        }
        
    }

    if (!tree)
        return tree;

    tree->height =  1 + 
                    max(HeightOfTree(tree->left), 
                    HeightOfTree(tree->right));

    int balanceOfTree = BalanceOfTree(tree);



    if (balanceOfTree > 1 && BalanceOfTree(tree->left) >= 0)
    {
        return RightRotate(tree);
    }

    if (balanceOfTree < -1 && BalanceOfTree(tree->right) <= 0)
    {
        return LeftRotate(tree);
    }

    if (balanceOfTree > 1 && BalanceOfTree(tree->left) < 0)
    {
        tree->left = LeftRotate(tree->left);
        return RightRotate(tree);
    }

    if (balanceOfTree < -1 && BalanceOfTree(tree->right) > 0)
    {
        tree->right = RightRotate(tree->right);
        return LeftRotate(tree);
    }

    return tree;
}

AVLTree *GetNode(AVLTree *tree, int val);

void CleanAVLTree(AVLTree *tree, bool freeData)
{
    if (tree)
    {
        if (freeData)
            free(tree->data);
        CleanAVLTree(tree->left, freeData);
        CleanAVLTree(tree->right, freeData);
        free(tree);
    }
}
#include "util/ps_hashmap.h"

#include <stdlib.h>
#include <string.h>

HashMap* CreateHashMap(int capacity, HashFunction func)
{
    HashMap *mapContainer = (HashMap*)malloc(sizeof(HashMap));
    mapContainer->cap = capacity;
    mapContainer->func = func;
    mapContainer->trees = (AVLTree*)calloc(capacity, sizeof(AVLTree));
    return mapContainer;
}


static HashEntry *CreateHashEntry(const char *fKey, int size, void *fData)
{
    HashEntry *entry = (HashEntry*)malloc(sizeof(HashEntry));
    entry->key = fKey;
    entry->data = fData;
    entry->sizeKey = size;
    return entry;
}


static int DescendHashIndex(AVLTree *root, HashEntry *entry, u32 code)
{
    if (!root)
        return 0;
    
    HashEntry *current = (HashEntry*)root->data;
    if (current->sizeKey == entry->sizeKey)
    {
        if (!memcmp(current->key, entry->key, current->sizeKey))
        {
            return -1;
        }
    }

    AVLTree *next = root->left;

    if (code > root->node)
    {
        next = root->right;
    }

    return DescendHashIndex(next, entry, code);
}

void InsertHashMap(HashMap *hashmap, const void *key, int lenKey, void *data)
{
    u32 hashCode = hashmap->func(key, lenKey);
    int index = hashCode % hashmap->cap;

    AVLTree *tree = &hashmap->trees[index];
    HashEntry *entry = CreateHashEntry(key, lenKey, data);
    if (!tree->height)
    {
        
        tree->height = 1;
        tree->node = hashCode;
        tree->data = entry;
        return;
    } 
    int ret = DescendHashIndex(tree, entry, hashCode);
    if (ret < 0)
    {
        free(entry);
        return;
    } 

    AVLTree *newNode = CreateAVLNode(entry, hashCode);
    tree = InsertAVLNode(tree, newNode);
    return;
}

HashEntry* GetFromHashMap(HashMap *hashmap, const void *key, int lenKey)
{
    u32 hashCode = hashmap->func(key, lenKey);
    
    int index = hashCode % hashmap->cap;

    AVLTree *tree = &hashmap->trees[index];
    while(tree)
    {
        int code = tree->node;
        
        if (hashCode == code)
        {   
            HashEntry *found = tree->data;
            if (!memcmp(found->key, key, lenKey))
            {
                return found;
            }
        }
    
        if (hashCode > code)
        {
            tree = tree->right;
        } else {
            tree = tree->left;
        }
    }

    return NULL;
}

HashEntry* GetFromHashMapByCode(HashMap *hashmap, u32 hashCode)
{
    
    int index = hashCode % hashmap->cap;

    AVLTree *tree = &hashmap->trees[index];
    while(tree)
    {
        int code = tree->node;
        
        if (hashCode == code)
        {   
            return tree->data;
        }
    
        if (hashCode > code)
        {
            tree = tree->right;
        } else {
            tree = tree->left;
        }
    }

    return NULL;
}
#include "util/ps_hashmap.h"
#include "log/ps_log.h"

#include <stdlib.h>
#include <string.h>

HashMap* CreateHashMap(int capacity, HashFunction func)
{
    if (capacity <= 0 || !func) return NULL;

    HashMap *mapContainer = (HashMap*)malloc(sizeof(HashMap));
    if (!mapContainer) return NULL;

    mapContainer->cap = capacity;
    mapContainer->func = func;
    mapContainer->trees = (AVLTree*)calloc(capacity, sizeof(AVLTree));
    if (!mapContainer->trees)
    {
        free(mapContainer);
        return NULL;
    }

    return mapContainer;
}


static HashEntry *CreateHashEntry(const void *fKey, int size, void *fData)
{
    HashEntry *entry = (HashEntry*)malloc(sizeof(HashEntry));
    if (!entry) return NULL;
    entry->key = fKey;
    entry->data = fData;
    entry->sizeKey = size;
    return entry;
}


static int DescendHashIndex(AVLTree *root, HashEntry *entry, u64 code)
{
    if (!root)
        return 0;
    
    HashEntry *current = (HashEntry*)root->data;
    if (current->sizeKey == entry->sizeKey && !memcmp(current->key, entry->key, current->sizeKey))
    {
        return -1;
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
    if (!hashmap || !key || lenKey <= 0 || !data) return;
    u64 hashCode = hashmap->func(key, lenKey);
    int index = hashCode % hashmap->cap;

    AVLTree *tree = &hashmap->trees[index];
    HashEntry *entry = CreateHashEntry(key, lenKey, data);
    if (!entry) return;
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

static HashEntry *FindHashEntry(AVLTree *tree, const void *key, int lenKey, u64 hashCode)
{
    while(tree)
    {
        u64 code = tree->node;
        HashEntry *found = tree->data;

        if (hashCode == code && (!key || (lenKey == found->sizeKey && !memcmp(found->key, key, lenKey))))
                return found;           
    
        if (hashCode > code)
            tree = tree->right;
        else 
            tree = tree->left;  
    }
    return NULL;
}

HashEntry* GetFromHashMap(HashMap *hashmap, const void *key, int lenKey)
{
    u64 hashCode = hashmap->func(key, lenKey);
    
    int index = hashCode % hashmap->cap;

    AVLTree *tree = &hashmap->trees[index];

    return FindHashEntry(tree, key, lenKey, hashCode);
}

HashEntry* GetFromHashMapByCode(HashMap *hashmap, u64 hashCode)
{
    int index = hashCode % hashmap->cap;

    AVLTree *tree = &hashmap->trees[index];
    
    return FindHashEntry(tree, NULL, 0, hashCode);

}

static void CleanHashEntries(AVLTree *tree, bool root, bool freeData)
{
    if (tree && tree->data)
    {
        if (freeData)
        {
            HashEntry *entry = (HashEntry*)tree->data;
            free(entry->data);
        }
       
        free(tree->data);
        CleanHashEntries(tree->left, false, freeData);
        CleanHashEntries(tree->right, false, freeData);
        if (!root)
            free(tree);
    }
}

void ClearHashMap(AVLTree *trees, bool freeData, int cap)
{
    if (!trees) return;
    for (int i = 0; i < cap; i++)
    {
        CleanHashEntries(&trees[i], true, freeData);
    }

}

void CleanHashMap(HashMap *hashmap, bool freeData)
{
    if (!hashmap) return;
    ClearHashMap(hashmap->trees, freeData, hashmap->cap);

    free(hashmap->trees);
    free(hashmap);
}
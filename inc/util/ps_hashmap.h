#ifndef PS_HASHMAP_H
#define PS_HASHMAP_H
#include "ps_global.h"
#include "util/ps_avltree.h"


HashMap* CreateHashMap(int capacity, HashFunction func);
void InsertHashMap(HashMap *hashmap, const void *key, int lenKey, void *data);
HashEntry* GetFromHashMap(HashMap *hashmap, const void *key, int lenKey);
HashEntry* GetFromHashMapByCode(HashMap *hashmap, u64 hashCode);
void CleanHashMap(HashMap *hashmap, bool freeData);
void ClearHashMap(AVLTree *trees, bool freeData, int cap);
#endif 
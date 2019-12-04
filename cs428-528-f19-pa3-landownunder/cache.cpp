#include <unordered_map>
#include <string>
#include <climits>
#include <iostream>

using namespace std;

extern string getContentFromResponse(string);

class LRUCache {
  int maxSize;
  int currentSize;
  unordered_map<string, char*> map;
  unordered_map<string, int> lru;
  
  public: 
  
  LRUCache(int capacity) {
    maxSize = capacity;
	currentSize = 0;
  }
  void evict() {
    int maxNum = INT_MIN;
	string maxStr = "";
	while(currentSize > maxSize) {
	  for(auto i = lru.begin(); i != lru.end(); i++) {
	    if(i->second > maxNum) {
		  maxNum = i->second;
		  maxStr = i->first;
		}
	  }
	  currentSize -= stoi(getContentFromResponse(map[maxStr]));
	  map.erase(maxStr);
	  lru.erase(maxStr);
	}
  }
  void updateOnHit(string key) {
    int val = lru[key];
	for(auto i = lru.begin(); i != lru.end(); i++) {
	  if(i->second < val)
	    i->second++;
	}
	lru[key] = 0;
  }
  void insert(string key, char* data, int size) {
	currentSize += size;
	if(currentSize > maxSize) {
	  evict();
	}
	map[key] = data;
	for(auto i = lru.begin(); i != lru.end(); i++) {
	  i->second++;
	}
	lru[key] = 0;
  }
  bool contains(string key) {
    return map.count(key);
  }
  char* get(string key) {
    return map[key];
  }
};

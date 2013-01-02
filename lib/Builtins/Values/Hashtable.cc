#include <5D/Allocators>
#include "Values/Hashtable"
namespace Values {

static inline NodeT listFromHashtable(Hashtable::const_iterator iter, Hashtable::const_iterator endIter) {
	if(iter == endIter)
		return(NULL);
	else {
		NodeT k = symbolFromStr(iter->first);
		NodeT v = iter->second;
		++iter;
		return(makeCons(makePair(k, v), listFromHashtable(iter, endIter)));
	}
}
NodeT keysOfHashtable(Hashtable::const_iterator iter, Hashtable::const_iterator endIter) {
	if(iter == endIter)
		return(NULL);
	else {
		NodeT k = symbolFromStr(iter->first);
		++iter;
		return(makeCons(k, keysOfHashtable(iter, endIter)));
	}
}

}

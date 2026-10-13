// autopools.cpp — real ObjectPoolClass storage for AutoPoolClass statics (WWLib types).
// DEFINE_AUTO_POOL in the owning .cpp was not emitting under MinGW/GCC the way MSVC did;
// provide constructed pools here so list nodes are not allocated from zero-filled stubs.

#include "always.h"
#include "mempool.h"
#include "multilist.h"
#include "slnode.h"

DEFINE_AUTO_POOL(MultiListNodeClass, 256);
DEFINE_AUTO_POOL(GenericSLNode, 256);

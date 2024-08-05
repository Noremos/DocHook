
#include "CachedBarcode.h"


CachedBarline* CachedBarline::getChild(int cid) const
{
	return root->getRItem(children[cid]);
}

CachedBarline* CachedBarline::getParent() const
{
	return root->getRItem(parentId);
}

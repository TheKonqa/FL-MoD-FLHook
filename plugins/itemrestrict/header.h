#include "binarytree.h"

struct UINT_WRAP
{
	UINT_WRAP(uint u) {val = u;}
	bool operator==(UINT_WRAP uw) { return uw.val==val; }
	bool operator>=(UINT_WRAP uw) { return uw.val>=val; }
	bool operator<=(UINT_WRAP uw) { return uw.val<=val; }
	bool operator>(UINT_WRAP uw) { return uw.val>val; }
	bool operator<(UINT_WRAP uw) { return uw.val<val; }
	uint val;
};


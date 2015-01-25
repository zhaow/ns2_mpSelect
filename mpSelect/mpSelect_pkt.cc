#include "mpSelect_pkt.h"

char* addr2str(const nsaddr_t& addr, char* res)
{
	int a;
	char *tmp = res;
	
	for(int i=1; i<=ADDR_LEVEL; i++) {
		a = addr >> Address::instance().NodeShift_[i];
		a = a & Address::instance().NodeMask_[i];
		tmp += sprintf(tmp, "%d.", a);
	}
	(*tmp) = '\0';
	return res;
}


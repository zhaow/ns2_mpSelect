#ifndef MYPING_H
#define MYPING_H

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"

struct hdr_myping {
	char ret;
	double send_time;

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static hdr_myping* access(const Packet* p){
		return (hdr_myping*) p->access(offset_);
	}
};

class myPingAgent : public Agent {
public:
	myPingAgent();
	virtual int command(int argc, const char*const* argv);
	virtual void recv(Packet*, Handler*);
};


#endif

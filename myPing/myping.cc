#include "myping.h"

int hdr_myping::offset_;
static class myPingHeaderClass:public PacketHeaderClass{
public:
	myPingHeaderClass() : PacketHeaderClass("PacketHeader/myPing", sizeof(hdr_myping)) {
		bind_offset(&hdr_myping::offset_);
	}
}class_pinghdr;

static class myPingClass : public TclClass{
public:
	myPingClass() : TclClass("Agent/myPing") {}
	TclObject* create(int, const char*const*){
		return (new myPingAgent());
	}
}class_myping;

myPingAgent::myPingAgent() : Agent(PT_MYPING)
{
	bind("packetSize_", &size_);
}

int
myPingAgent::command(int argc, const char*const* argv)
{
	if (argc == 2) {
	    if (strcmp(argv[1], "send") == 0) {
	    	// Create a new packet
	    	Packet* pkt = allocpkt();
	        // Access the Ping header for the new packet:
	        hdr_myping* hdr = hdr_myping::access(pkt);
	        // Set the 'ret' field to 0, so the receiving node knows
	        // that it has to generate an echo packet
	        hdr->ret = 0;
	        // Store the current time in the 'send_time' field
	        hdr->send_time = Scheduler::instance().clock();
	        // Send the packet
	        send(pkt, 0);
	        // return TCL_OK, so the calling function knows that the
	        // command has been processed
	        return (TCL_OK);
	     }
	 }
	 // If the command hasn't been processed by myPingAgent()::command,
	 // call the command() function for the base class
	 return (Agent::command(argc, argv));
}

void
myPingAgent::recv(Packet* pkt, Handler* )
{
	hdr_ip* hdrip = hdr_ip::access(pkt);
	hdr_myping* hdr= hdr_myping::access(pkt);

	if (hdr->ret == 0){
		double stime = hdr->send_time;
		Packet::free(pkt);
		//return ping packet
		Packet* pktret = allocpkt();
		hdr_myping* hdrret = hdr_myping::access(pktret);
		hdrret->ret = 1;
		hdrret->send_time = stime;

		send(pktret, 0);
	} else {
		char message[200];
		sprintf(message, "%s recv %d %3.1f", name(), 
		hdrip->src_.addr_ >> Address::instance().NodeShift_[1], 
		(Scheduler::instance().clock() - hdr->send_time) * 1000);

		Tcl& tcl = Tcl::instance();
		tcl.eval(message);
		Packet::free(pkt);
	}
}
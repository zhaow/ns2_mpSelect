#ifndef MP_SELECT_PKT_H_
#define MP_SELECT_PKT_H_

#include <cstring>
#include "packet.h"
#include "address.h"
#include "agent.h"
#include "tclcl.h"
#include "ip.h"
#include "scheduler.h"
#include "classifier/classifier-port.h"
#include "trace.h"
#include "trace/cmu-trace.h"

#define SEND_PING_INTERVAL 1.0
#define ADDR_LEVEL 3
#define CURRENT_TIME Scheduler::instance().clock()

#define MPSELECT_HDR_LEN 40
#define HDR_MPSELECT_PKT(p) hdr_mpSelect_pkt::access(p)
#define DATA_PKT 0
#define PING_PKT 1
#define PING_BAK 2

char* addr2str(const nsaddr_t&, char*);

/*协议报头*/
struct hdr_mpSelect_pkt {
	int			type_; 			//协议报文类型0:DATA 1:PING 2:PING_BAK
	nsaddr_t 	former_addr_;	//原目的ip地址
	int32_t  	former_port_;	//原目的端口号
	packet_t 	former_ptype_;	//原协议类型
	double   	send_time_;		//转发时间
	nsaddr_t 	s_intf_;		//选路起始接口ip
	nsaddr_t 	d_intf_;		//选路目的接口ip
	nsaddr_t 	src_select_;	//选路协议所在ip
	unsigned int ping_seq_;		//ping包序号

	inline int& type() {return type_;}
	inline nsaddr_t& former_addr() {return former_addr_;}
	inline int32_t&  former_port() {return former_port_;}
	inline packet_t& former_ptype(){return former_ptype_;}
	inline double& send_time(){return send_time_;}
	inline nsaddr_t& s_intf() {return s_intf_;}
	inline nsaddr_t& d_intf() {return d_intf_;}
	inline nsaddr_t& src_select() {return src_select_;}
	inline unsigned int& ping_seq() {return ping_seq_;}

	static int 	offset_;		//报头起始位
	inline static int& offset() {return offset_;}
	inline static hdr_mpSelect_pkt* access(const Packet* p) {
		return (hdr_mpSelect_pkt*)p->access(offset_);
	}

};


#endif


#ifndef MP_SELECT_H_
#define MP_SELECT_H_

#include "mpSelect_pkt.h"
#include "mpSelect_rtable.h"
class mpSelect;

//ping pkt timer
class mpPing_PktTimer : public TimerHandler {
public:
	mpPing_PktTimer(mpSelect *agent) : TimerHandler() {
		agent_ = agent;
	}
protected:
	mpSelect* agent_;
	void expire(Event *e);
};

#define SELECT_CORE 0
#define SELECT_INTF 1

//mpSelect class
class mpSelect : public Agent {
friend class mpPing_PktTimer;
protected:
	int 			select_type_;		//core:0 intf:1
	nsaddr_t		ra_addr_;	//协议运行的ip地址
	mpRtable		rtable_;	//路由表
	PortClassifier* dmux_;		//本地上层接口
	mpPing_PktTimer	ping_timer_;//ping包计时器

	inline nsaddr_t& ra_addr() {return ra_addr_;}
	inline int& select_type() {return select_type_;}
	/*转发分组*/
	void forward_data(Packet*);
	/*处理收到的ping包*/
	void recv_ping_pkt(Packet*);
	/*处理收到的数据包*/
	void recv_data_pkt(Packet*);
	/*计时器触发时按路由表向所有协议节点发出ping包*/
	void send_ping_handler();
	/*重置计时器*/
	void reset_ping_timer();
	/*接口节点转发函数*/
	void forward_mpS_pkt(Packet*);
public:
	/*构造函数*/
	mpSelect(nsaddr_t);
	/*tcl命令接口*/
	int command(int, const char* const*);
	/*分组入口(覆盖)*/
	void recv(Packet*, Handler*);
};

#endif


#include "mpSelect.h"

int hdr_mpSelect_pkt::offset_;

static class mpSelectHeaderClass : public PacketHeaderClass {
public:
	mpSelectHeaderClass() : PacketHeaderClass("PacketHeader/mpSelect",
	sizeof(hdr_mpSelect_pkt)) {
		bind_offset(&hdr_mpSelect_pkt::offset_);
	}
}class_rtProtompSelect_hdr;

static class mpSelectClass : public TclClass {
public:
	mpSelectClass() : TclClass("Agent/mpSelect") {}
	TclObject* create(int argc, const char* const* argv) {
		return (new mpSelect((nsaddr_t)Address::instance().str2addr(argv[4])));
	}
}class_rtProtompSelect;
/*ping 计时器超时后处理函数*/
void mpPing_PktTimer::expire(Event *e) {
	agent_->send_ping_handler();
	agent_->reset_ping_timer();
}
/*调用路由表的发送ping函数*/
void mpSelect::send_ping_handler() {
	mpath_item path;
	nsaddr_t dest_select;
	char strIP[16];

	while (rtable_.get_path_item(dest_select, path)) {
		Packet* pkt = allocpkt();
		hdr_cmn *hdrcmn = hdr_cmn::access(pkt);
		hdr_ip  *hdrip  = hdr_ip::access(pkt);
		hdr_mpSelect_pkt *hdrmpS = hdr_mpSelect_pkt::access(pkt);
		
		hdrmpS->type()      = PING_PKT;
		hdrmpS->s_intf()    = path.s_intf();
		hdrmpS->d_intf()    = path.d_intf();
		hdrmpS->src_select()= ra_addr();
		hdrmpS->send_time() = CURRENT_TIME;
		rtable_.update_ping_info(dest_select, path.s_intf(), path.d_intf());
		
		hdrip->saddr() = ra_addr();
		hdrip->sport() = RT_PORT;
		hdrip->daddr() = dest_select;
		hdrip->dport() = RT_PORT;
		hdrcmn->ptype()     = PT_MPSELECT;
		hdrcmn->direction() = hdr_cmn::DOWN;
		hdrcmn->size()      = MPSELECT_HDR_LEN + IP_HDR_LEN;
		hdrcmn->error()     = 0;
		hdrcmn->next_hop()  = hdrmpS->d_intf();
		hdrcmn->addr_type() = NS_AF_INET;
		
		printf("[%f] %s", CURRENT_TIME, addr2str(ra_addr(), strIP));
		printf("SEND PING[%u] to %s ", 
			hdrmpS->ping_seq(),
			addr2str(hdrip->daddr(), strIP));
		printf("sintf%s ", addr2str(hdrmpS->s_intf(), strIP));
		printf("dintf%s\n", addr2str(hdrmpS->d_intf(), strIP));

		Scheduler::instance().schedule(target_, pkt, 0.0);
	}
	return;
}
/*重置ping计时器*/
void mpSelect::reset_ping_timer() {
	ping_timer_.resched(SEND_PING_INTERVAL);
}
/*协议构造函数*/
mpSelect::mpSelect(nsaddr_t addr) : Agent(PT_MPSELECT), rtable_(), ping_timer_(this) {
	ra_addr_ = addr;
}
/*覆盖原recv函数*/
void mpSelect::recv(Packet* p, Handler* h) {
	hdr_cmn *hdrcmn	= hdr_cmn::access(p); 
	hdr_ip 	*hdrip	= hdr_ip::access(p);
	
	if (hdrip->saddr() == ra_addr_) {
		if (hdrcmn->num_forwards() > 0) {//本地发出且被转发过（产生路由环路，丢弃）
			drop(p, DROP_RTR_ROUTE_LOOP);
			return;
		} else if (hdrcmn->num_forwards() == 0)
			hdrcmn->size() += IP_HDR_LEN;//添加ip报头
	}

	if (hdrcmn->ptype() == PT_MPSELECT) {
		hdr_mpSelect_pkt * hdrmpS = hdr_mpSelect_pkt::access(p);
		if (hdrmpS->type() == DATA_PKT) {//收到从其他选路协议发出的封装数据包
			recv_data_pkt(p);
		} else {//收到ping包
			recv_ping_pkt(p);
		}
	} else {
		if (--hdrip->ttl_ < 0) {//当TTL为0时，丢弃
			drop(p, DROP_RTR_TTL);
			return;
		}
		forward_data(p);
	}	
}
/*转发选路函数*/
void mpSelect::forward_data(Packet* p)
{
	hdr_cmn *hdrcmn	= hdr_cmn::access(p); 
	hdr_ip 	*hdrip	= hdr_ip::access(p);
	
	if (hdrcmn->direction() == hdr_cmn::UP &&
		((u_int32_t)hdrip->daddr() == IP_BROADCAST || hdrip->daddr() == ra_addr())) {
		dmux_->recv(p, NULL);//目的地址是本地，传给上层
		return;
	} else {
		hdrcmn->direction() = hdr_cmn::DOWN;
		hdrcmn->addr_type() = NS_AF_INET;

		if ((u_int32_t)hdrip->daddr() == IP_BROADCAST) {
			hdrcmn->next_hop() = IP_BROADCAST;
		} else {
			if (rtable_.find_MP(hdrip->daddr()) == TRUE) {//使用多路径传输
				nsaddr_t dest_select;
				nsaddr_t s_intf_ip;
				nsaddr_t d_intf_ip;
				/*选择路径*/
				dest_select = rtable_.lookup(hdrip->daddr(), s_intf_ip, d_intf_ip);

				if ((u_int32_t)dest_select == IP_BROADCAST) {//未找到合适的路径
					char strIP[16];
					printf("[%f] %s:can not forward packet ", 
						CURRENT_TIME, 
						addr2str(ra_addr(), strIP));
					printf("from %s", addr2str(hdrip->saddr(), strIP));
					printf("to %s by MP", addr2str(hdrip->daddr(), strIP));
					drop(p, DROP_RTR_NO_ROUTE);
					return;
				} else {//找到路径，添加mpSelect报头
					hdrcmn->size() += MPSELECT_HDR_LEN;
					hdr_mpSelect_pkt *hdrmpS = hdr_mpSelect_pkt::access(p);
					hdrmpS->type()        = DATA_PKT;
					hdrmpS->former_addr() = hdrip->daddr();
					hdrmpS->former_port() = hdrip->dport();
					hdrmpS->former_ptype()= hdrcmn->ptype();
					hdrmpS->send_time()   = CURRENT_TIME;
					hdrmpS->s_intf()      = s_intf_ip;
					hdrmpS->d_intf()      = d_intf_ip;
					hdrmpS->src_select()  = ra_addr();
					/*将ip的目的地址转为选路协议的地址*/
					hdrip->daddr() = dest_select;
					hdrip->dport() = RT_PORT;
					hdrcmn->ptype()= PT_MPSELECT;
					hdrcmn->next_hop() = s_intf_ip;
				}
			} else {//没有多路径，正常转发
				nsaddr_t next_hop = rtable_.lookup(hdrip->daddr());
				if ((u_int32_t)next_hop == IP_BROADCAST) {
					char strIP[16];
					printf("[%f] %s:can not forward packet ", 
						CURRENT_TIME, 
						addr2str(ra_addr(), strIP));
					printf("from %s", addr2str(hdrip->saddr(), strIP));
					printf("to %s by SP", addr2str(hdrip->daddr(), strIP));
					drop(p, DROP_RTR_NO_ROUTE);
					return;
				} else {
					hdrcmn->next_hop() = next_hop;
				}
			}
		}
		Scheduler::instance().schedule(target_, p, 0.0);
	}
}
/*处理封装好的数据包*/
void mpSelect::recv_data_pkt(Packet *p)
{
	hdr_cmn *hdrcmn	= hdr_cmn::access(p); 
	hdr_ip 	*hdrip	= hdr_ip::access(p);
	hdr_mpSelect_pkt *hdrmpS = hdr_mpSelect_pkt::access(p);

	char strIP[16];
	printf("[%f] %s: recv DATA ", 
		CURRENT_TIME, 
		addr2str(ra_addr(), strIP));
	printf("from %s ", addr2str(hdrip->saddr(), strIP));
	printf("to %s ",   addr2str(hdrmpS->former_addr(), strIP));
	printf("by %s ",   addr2str(hdrmpS->s_intf(), strIP));
	printf("->%s", 	   addr2str(hdrmpS->d_intf(), strIP));
	printf("srcS:%s\n",  addr2str(hdrmpS->src_select(), strIP));
	/*更新该路径的活动时间*/
	rtable_.update_rtime(hdrmpS->src_select(), hdrmpS->s_intf(), hdrmpS->d_intf(), CURRENT_TIME);
	/*还原ip报头*/
	hdrip->daddr() = hdrmpS->former_addr();
	hdrip->dport() = hdrmpS->former_port();
	hdrcmn->ptype()= hdrmpS->former_ptype();
	hdrcmn->size() -= MPSELECT_HDR_LEN;
	/*如果目的地址是本地，向上转发*/
	if (hdrip->daddr() == ra_addr())
		hdrcmn->direction() = hdr_cmn::UP;
	/*调用转发函数，再次转发*/
	forward_data(p);
	return;
}
/*处理ping包*/
void mpSelect::recv_ping_pkt(Packet *p)
{
	hdr_ip 	*hdrip	= hdr_ip::access(p);
	hdr_mpSelect_pkt *hdrmpS = hdr_mpSelect_pkt::access(p);

	char strIP[16];
	printf("[%f] %s: recv PING ", 
		CURRENT_TIME, 
		addr2str(ra_addr(), strIP));
	printf("from %s ", addr2str(hdrip->saddr(), strIP));
	printf("to %s ",   addr2str(hdrmpS->former_addr(), strIP));
	printf("by %s ",   addr2str(hdrmpS->s_intf(), strIP));
	printf("->%s", 	   addr2str(hdrmpS->d_intf(), strIP));
	printf("srcS:%s\n",  addr2str(hdrmpS->src_select(), strIP));

	/*更新该路径的活动时间*/
	rtable_.update_rtime(hdrmpS->src_select(), hdrmpS->s_intf(), hdrmpS->d_intf(), CURRENT_TIME);
	if (hdrmpS->type() == PING_PKT) {//收到ping包，向源地址响应
		/*更新接收时延*/
		rtable_.update_rdelay(hdrmpS->src_select(), hdrmpS->s_intf(), hdrmpS->d_intf(), hdrmpS->send_time());
		/*申请响应ping包*/
		Packet* pecho = allocpkt();
		hdr_cmn *phdrcmn= hdr_cmn::access(pecho); 
		hdr_ip 	*phdrip	= hdr_ip::access(pecho);
		hdr_mpSelect_pkt *phdrmpS = hdr_mpSelect_pkt::access(pecho);
		/*给mpSelect报头赋值*/
		phdrmpS->type() 	= PING_BAK;
		phdrmpS->s_intf() 	= hdrmpS->d_intf();
		phdrmpS->d_intf() 	= hdrmpS->s_intf();
		phdrmpS->src_select() = ra_addr();
		phdrmpS->ping_seq() = hdrmpS->ping_seq();
		phdrmpS->send_time()= CURRENT_TIME;
		
		/*给ip报头赋值*/
		phdrip->saddr() = ra_addr();
		phdrip->sport() = RT_PORT;
		phdrip->daddr() = hdrmpS->src_select();
		phdrip->dport() = RT_PORT;
		phdrip->ttl()   = IP_DEF_TTL;

		/*给common报头赋值*/
		phdrcmn->ptype() 	= PT_MPSELECT;
		phdrcmn->direction()= hdr_cmn::DOWN;
		phdrcmn->size() 	= MPSELECT_HDR_LEN + IP_HDR_LEN;
		phdrcmn->error() 	= 0;
		phdrcmn->next_hop() = hdrmpS->d_intf();
		phdrcmn->addr_type()= NS_AF_INET;

		Scheduler::instance().schedule(target_, pecho, 0.0);
	} else {
		/*更新发送时延*/
		rtable_.update_sdelay(hdrmpS->src_select(), hdrmpS->s_intf(), hdrmpS->d_intf(), hdrmpS->send_time(), hdrmpS->ping_seq());
	}
	Packet::free(p);
}

int mpSelect::command(int argc, const char* const* argv) 
{
	/*
	for (int i=0; i<argc; i++) {
		printf("%s\n", argv[i]);
	}
	*/
	if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			ping_timer_.resched(0.0);
			return TCL_OK;
		} else if (strcmp(argv[1], "print-rtable") == 0) {
			rtable_.print();
			return TCL_OK;
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "delete-select") == 0) {
			rtable_.delete_select((nsaddr_t)Address::instance().str2addr(argv[2]));
			return TCL_OK;
		} 
	} else if (argc == 4) {
		if (strcmp(argv[1], "delete-route-item") == 0) {
			nsaddr_t dest_seg = Address::instance().str2addr(argv[2]);
			nsaddr_t mask 	  = Address::instance().str2addr(argv[3]);
			rtable_.delete_item(dest_seg, mask);
			return TCL_OK;
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "add-route-item") == 0) {
			nsaddr_t dest_seg = Address::instance().str2addr(argv[2]);
			nsaddr_t mask 	  = Address::instance().str2addr(argv[3]);
			nsaddr_t next_hop = Address::instance().str2addr(argv[4]);
			rtable_.add_item(dest_seg, mask, next_hop);
			return TCL_OK;
		} else if (strcmp(argv[1], "add-mp-path") == 0) {
			nsaddr_t select = Address::instance().str2addr(argv[2]);
			nsaddr_t s_intf	= Address::instance().str2addr(argv[3]);
			nsaddr_t d_intf = Address::instance().str2addr(argv[4]);
			rtable_.add_mpath(select, s_intf, d_intf);
			return TCL_OK;
		} else if (strcmp(argv[1], "delete-mp-path") == 0) {
			nsaddr_t select = Address::instance().str2addr(argv[2]);
			nsaddr_t s_intf	= Address::instance().str2addr(argv[3]);
			nsaddr_t d_intf = Address::instance().str2addr(argv[4]);
			rtable_.delete_mpath(select, s_intf, d_intf);
			return TCL_OK;
		} else if (strcmp(argv[1], "attach-select") == 0) {
			nsaddr_t dest_seg = Address::instance().str2addr(argv[2]);
			nsaddr_t mask 	  = Address::instance().str2addr(argv[3]);
			nsaddr_t select	  = Address::instance().str2addr(argv[4]);
			rtable_.attach_select(dest_seg, mask, select);
			return TCL_OK;
		} else if (strcmp(argv[1], "detach-select") == 0) {
			nsaddr_t dest_seg = Address::instance().str2addr(argv[2]);
			nsaddr_t mask 	  = Address::instance().str2addr(argv[3]);
			nsaddr_t select	  = Address::instance().str2addr(argv[4]);
			rtable_.detach_select(dest_seg, mask, select);
			return TCL_OK;
		}
	}
	return Agent::command(argc, argv);
}


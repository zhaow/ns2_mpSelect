#ifndef MP_SELECT_RTABLE_H
#define MP_SELECT_RTABLE_H
/*
#include "trace.h"
#include "mpSelect.h"
#include "mpSelect_pkt.h"
#include "cstring"
*/

#include "mpSelect_pkt.h"

#define ALIVE_INTERVAL 2.0

class mpRtable;

/*多路径结构*/
struct mpath_item {
	nsaddr_t 		s_intf_; 		//路径起始接口ip
	nsaddr_t 		d_intf_;		//路径目的接口ip
	double 			recv_delay_;	//接收时延(收到ping时间-ping包发送时间)
	double 			send_delay_;	//发送时延(收到返ping时间-正ping包发送时间)
	double			ping_send_time_;//正ping包发送时间
	double 			last_recv_time_;//上次收到数据（包括ping）时间
	unsigned int 	ping_seq_;		//最后一次ping包发送序号
	int 			cnt_;			//使用路径次数
	struct mpath_item*   next_path_;	//指向下一条路径
	struct mpath_item*   prev_path_;	//指向上一条路径

	inline nsaddr_t& s_intf() {return s_intf_;}
	inline nsaddr_t& d_intf() {return d_intf_;}
	inline double& recv_delay() {return recv_delay_;}
	inline double& send_delay() {return send_delay_;}
	inline double& ping_send_time() {return ping_send_time_;}
	inline double& last_recv_time() {return last_recv_time_;}
	inline unsigned int& ping_seq() {return ping_seq_;}
	inline int& cnt() {return cnt_;}
	inline struct mpath_item*& next_path() {return next_path_;}
	inline struct mpath_item*& prev_path() {return prev_path_;}

};

class mpath {

friend class mpRtable;

protected:
	mpath_item* pathHead_;		//多路径表
	nsaddr_t 	select_addr_;	//选路协议地址
	mpath*      next_select_;
	mpath*      prev_select_;
	
	inline mpath_item*& pathHead() {return pathHead_;}
	inline nsaddr_t& select_addr() {return select_addr_;}
	inline mpath*& next_select() {return next_select_;}
	inline mpath*& prev_select() {return prev_select_;}
public:
	/*构造函数*/
	mpath(nsaddr_t addr) : pathHead_(NULL) {
		select_addr_ = addr;
	}
	/*打印多路径表*/
	void print();
	/*清空表项*/
	void clear();
	/*添加一条路径*/
	void add_mpath_item(nsaddr_t, nsaddr_t);
	/*删除-条路径*/
	void delete_mpath_item(nsaddr_t, nsaddr_t);
	/*更新发送时延*/
	void update_send_delay(nsaddr_t, nsaddr_t, double, unsigned int);
	/*更新接收时延*/
	void update_recv_delay(nsaddr_t, nsaddr_t, double);
	/*更新最近接受数据的时间*/
	void update_recv_time(nsaddr_t, nsaddr_t, double);
	/*该路径是否有效（最近接受数据的时间没有超过预估时间）*/
	bool is_alive(nsaddr_t, nsaddr_t);

	nsaddr_t select_path(nsaddr_t&, nsaddr_t&);

	void update_ping(nsaddr_t, nsaddr_t);
};

/*路由表项*/
struct route_item {
	nsaddr_t 		daddr_seg_;		//目的网段地址 
	nsaddr_t 		mask_;			//子网掩码
	nsaddr_t 		next_hop_;		//下一条地址
	bool 			mp_flag_;		//是否有多路径
	mpath* 			mptable_;		//多路径路由表
	struct route_item* next_;		//下一表项
	struct route_item* prev_;		//上一表项

	inline nsaddr_t& daddr_seg() {return daddr_seg_;}
	inline nsaddr_t& mask() {return mask_;}
	inline nsaddr_t& next_hop() {return next_hop_;}
	inline bool& mp_flag() {return mp_flag_;}
	inline mpath*& mptable() {return mptable_;}
	inline route_item*& next() {return next_;}
	inline route_item*& prev() {return prev_;}
};
/*路由表*/
class mpRtable {
protected:
	route_item* rtable_head_;	//路由表
	mpath*		mptable_head_;	//多路径表

	inline route_item*& rtable_head() {return rtable_head_;}
	inline mpath*& mptable_head() {return mptable_head_;}

public:
	mpRtable() : rtable_head_(NULL) , mptable_head_(NULL){}
	/*打印路由*/
	void print();
	/*清空路由*/
	void clear();
	/*添加路由表项*/
	void add_item(nsaddr_t, nsaddr_t, nsaddr_t);
	/*添加多路径表项*/
	void add_mpath(nsaddr_t, nsaddr_t, nsaddr_t);
	/*删除路由表项*/
	void delete_item(nsaddr_t, nsaddr_t);
	/*删除多路径表项*/
	void delete_mpath(nsaddr_t, nsaddr_t, nsaddr_t);
	/*删除选路协议表项*/
	void delete_select(nsaddr_t);
	/*给路由表项绑定多路径表*/
	void attach_select(nsaddr_t, nsaddr_t, nsaddr_t);
	/*解邦多路径表*/
	void detach_select(nsaddr_t, nsaddr_t, nsaddr_t);
	/*到该地址是否需要多路径*/
	bool find_MP(nsaddr_t);
	/*返回下一跳（非多路径）*/
	nsaddr_t lookup(nsaddr_t);
	/*返回下一跳（多路径）*/
	nsaddr_t lookup(nsaddr_t, nsaddr_t&, nsaddr_t&);
	/*更新发送时延*/
	void update_sdelay(nsaddr_t, nsaddr_t, nsaddr_t, double, unsigned int);
	/*更新接收时延*/
	void update_rdelay(nsaddr_t, nsaddr_t, nsaddr_t, double);
	/*更新最近接受数据的时间*/
	void update_rtime(nsaddr_t, nsaddr_t, nsaddr_t, double);

	bool get_path_item(nsaddr_t&, mpath_item&);

	void update_ping_info(nsaddr_t, nsaddr_t, nsaddr_t);
};

#endif


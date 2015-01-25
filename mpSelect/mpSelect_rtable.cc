#include "mpSelect_rtable.h"

void mpath::print()
{
	int i;
	mpath_item *p = NULL;
	char strIP[16];

	printf("Select IP : %s\n", addr2str(select_addr(), strIP));
	
	for (i=0, p=pathHead(); p!=NULL; p=p->next_path(), i++) {
		printf("PATH%d:s_intf:%s ", i, addr2str(p->s_intf(), strIP));
		printf("d_intf:%s, ping_seq:%u, cnt:%d, sdalay:%.1f, rdelay:%.1f, ptime:%.1f, rtime:%.1f\n",
		addr2str(p->d_intf(), strIP),
		p->ping_seq(), p->cnt(),
		p->recv_delay()*1000, p->send_delay()*1000,
		p->ping_send_time()*1000,
		p->last_recv_time()*1000);
	}
	return;
}

void mpath::clear()
{
	mpath_item *p = NULL;
	while (pathHead() != NULL) {
		p = pathHead();
		pathHead() = pathHead()->next_path();
		free(p);
	}
	return;
}

void mpath::add_mpath_item(nsaddr_t s_intf, nsaddr_t d_intf)
{
	mpath_item *p = new mpath_item;

	p->s_intf() = s_intf;
	p->d_intf() = d_intf;
	p->recv_delay() = 0.0;
	p->send_delay() = 0.0;
	p->ping_send_time() = CURRENT_TIME;
	p->last_recv_time() = CURRENT_TIME;
	p->ping_seq() = 0;
	p->cnt() = 0;
	p->next_path() = pathHead();
	p->prev_path() = NULL;
	if (pathHead() != NULL)
		pathHead()->prev_path() = p;
	pathHead() = p;

}

void mpath::delete_mpath_item(nsaddr_t s_intf, nsaddr_t d_intf)
{
	mpath_item *p = pathHead();

	while (p != NULL) {
		if (p->s_intf()==s_intf && p->d_intf()==d_intf) {
			if (p == pathHead()) {
				pathHead() = p->next_path(); 
				if (p->next_path() != NULL)
					p->next_path()->prev_path() = NULL;
			} else {
				p->prev_path()->next_path() = p->next_path();
				if (p->next_path() != NULL)
					p->next_path()->prev_path() = p->prev_path();
			}
			free(p);
			break;
		} else
			p = p->next_path();
	}
	return;
}

void mpath::update_send_delay(nsaddr_t s_intf, nsaddr_t d_intf, double t, unsigned int seq)
{
	
	mpath_item *p = pathHead();

	while (p != NULL) {
		if (p->s_intf()==s_intf && p->d_intf()==d_intf) {
			if (p->ping_seq() > seq) {
				p->send_delay() = t + (p->ping_seq()-seq-1)*SEND_PING_INTERVAL- p->ping_send_time();
			} else {
				printf("[%f] seq:%d is wrong, current ping_seq:%d\n", CURRENT_TIME, seq, p->ping_seq());
			}
			break;
		} else {
			p = p->next_path();
		}
	}
	return;
}

void mpath::update_recv_delay(nsaddr_t s_intf, nsaddr_t d_intf, double t)
{
	mpath_item *p = pathHead();

	while (p != NULL) {
		if (p->s_intf()==s_intf && p->d_intf()==d_intf) {
			p->recv_delay() = CURRENT_TIME - t;
			break;
		} else {
			p = p->next_path();
		}
	}
	return;
}
/*
void mpath::send_ping_by_path(nsaddr_t addr)
{
	mpath_item *p = pathHead();

	while (p != NULL) {
		Packet* pkt = allocpkt();
		hdr_cmn *hdrcmn = hdr_cmn::access(pkt);
		hdr_ip  *hdrip  = hdr_ip::access(pkt);
		hdr_mpSelect_pkt *hdrmpS = hdr_mpSelect_pkt::access(pkt);

		hdrmpS->type() 		= PING_PKT;
		hdrmpS->s_intf() 	= p->s_intf();
		hdrmpS->d_intf() 	= p->d_intf();
		hdrmpS->src_select()= addr;
		hdrmpS->ping_seq() 	= p->ping_seq()++;
		hdrmpS->send_time()	= CURRENT_TIME;
		p->ping_send_time() = hdrmpS->send_time();

		hdrip->saddr() = addr;
		hdrip->sport() = RT_PORT;
		hdrip->daddr() = select_addr();
		hdrip->dport() = RT_PORT;
		hdrip->ttl()   = IP_DEF_TTL;

		hdrcmn->ptype() 	= PT_MPSELECT;
		hdrcmn->direction() = hdr_cmn::DOWN;
		hdrcmn->size() 		= MPSELECT_HDR_LEN + IP_HDR_LEN;
		hdrcmn->error() 	= 0;
		hdrcmn->next_hop() 	= hdrmpS->d_intf();
		hdrcmn->addr_type()	= NS_AF_INET;

		Scheduler::instance().schedule(target_, pkt, 0.0);

		p = p->next_path();
	}
	return;
}
*/
void mpath::update_recv_time(nsaddr_t s_intf, nsaddr_t d_intf, double t)
{	
	mpath_item *p = pathHead();

	while (p != NULL) {
		if (p->s_intf()==s_intf && p->d_intf()==d_intf) {
			p->last_recv_time() = t;
			break;
		} else {
			p = p->next_path();
		}
	}
	return;
}

bool mpath::is_alive(nsaddr_t s_intf, nsaddr_t d_intf)
{
	mpath_item *p = pathHead();

	while (p != NULL) {
		if (p->s_intf()==s_intf && p->d_intf()==d_intf) {
			return ((CURRENT_TIME - p->last_recv_time()) > ALIVE_INTERVAL);
		}
	}
	return FALSE;
}

void mpRtable::print()
{
	route_item *r = rtable_head();
	mpath *p = mptable_head();
	char strIP[16];

	for (; r!=NULL; r=r->next()) {
		printf("dest:%s mask:%08x ", 
			addr2str(r->daddr_seg(), strIP), 
			r->mask() 
		);
		printf("next_hop:%s MP_FLAG:%c ", 
			addr2str(r->next_hop(), strIP),
			(r->mp_flag() ? 'T' : 'F')
		);
		if (r->mp_flag()) {
			printf("dest_select:%s\n", addr2str(r->mptable()->select_addr(), strIP));
		} else {
			printf("\n");
		}
	}

	for (; p!=NULL; p=p->next_select()) {
		p->print();
	}
	return;
}

void mpRtable::clear() 
{
	route_item *r = NULL;
	mpath *p = NULL;
	
	while (rtable_head() != NULL) {
		r = rtable_head();
		rtable_head() = rtable_head()->next();
		free(r);
	}

	while (mptable_head() != NULL) {
		p = mptable_head();
		mptable_head() = mptable_head()->next_select();
		p->clear();
		free(p);
	}
	return;
}

void mpRtable::add_item(nsaddr_t dest_seg, nsaddr_t mask, nsaddr_t next_hop)
{
	route_item *r = new route_item;

	r->daddr_seg() 	= dest_seg;
	r->mask() 		= mask;
	r->next_hop() 	= next_hop;
	r->next()		= rtable_head();
	r->prev()		= NULL;
	r->mp_flag()	= FALSE;
	if (rtable_head() != NULL)
		rtable_head()->prev()=r;
	rtable_head()	= r;

	return;
}	

void mpRtable::add_mpath(nsaddr_t select, nsaddr_t s_intf, nsaddr_t d_intf)
{
	mpath *ptr = mptable_head();
	
	for (; ptr!=NULL; ptr=ptr->next_select()) {
		if (ptr->select_addr() == select)
			break;
	}

	if (ptr == NULL) {
		mpath *p = new mpath(select);
		p->add_mpath_item(s_intf, d_intf);
		p->next_select() = mptable_head();
		p->prev_select() = NULL;
		mptable_head() = p;

	}  else {
		ptr->add_mpath_item(s_intf, d_intf);
	}
	return ;
}

void mpRtable::delete_item(nsaddr_t dest_seg, nsaddr_t mask)
{
	route_item *r = rtable_head();

	while (r != NULL) {
		if (r->daddr_seg()==dest_seg && r->mask()==mask) {
			if (r == rtable_head()) {
				rtable_head() = r->next();
				if (r->next() != NULL)
					r->next()->prev() = NULL;
			} else {
				r->prev()->next() = r->next();
				if (r->next() != NULL)
					r->next()->prev() = r->prev();
			}
			free(r);
			break;
		} else 
			r = r->next();
	}
	return ;
}

void mpRtable::delete_mpath(nsaddr_t select, nsaddr_t s_intf, nsaddr_t d_intf)
{
	mpath *p = mptable_head();
	for(; p!=NULL; p=p->next_select()) {
		if (p->select_addr() == select) {
			p->delete_mpath_item(s_intf, d_intf);
		}
	}
	return ;
}

void mpRtable::delete_select(nsaddr_t select)
{
	route_item *r = rtable_head();

	for (; r!=NULL; r=r->next()) {
		if (r->mp_flag()==TRUE && r->mptable()->select_addr()==select) {
			r->mp_flag()=FALSE;
		}
	}
	
	mpath *p = mptable_head();
	for(; p!=NULL; p=p->next_select()) {
		if (p->select_addr() == select) {
			if (p == mptable_head()) {
				mptable_head() = p->next_select();
				if (p->next_select() != NULL)
					p->next_select()->prev_select() = NULL;
			} else {
				p->prev_select()->next_select() = p->next_select();
				if (p->next_select() != NULL)
					p->next_select() = p->prev_select();
			}	
			p->clear();
			free(p);
		}
	}
	return ;
}

void mpRtable::attach_select(nsaddr_t dest_seg, nsaddr_t mask, nsaddr_t select)
{
	route_item *r = rtable_head();
	for (; r!=NULL; r=r->next()) {
		if (r->daddr_seg()==dest_seg && r->mask()==mask) {
			break;
		}
	}

	mpath *p = mptable_head();
	for (; p!=NULL; p=p->next_select()) {
		if (p->select_addr() == select) {
			break;
		}
	}

	if (r!=NULL && p!=NULL) {
		r->mp_flag() = TRUE;
		r->mptable() = p;
	}
}

void mpRtable::detach_select(nsaddr_t dest_seg, nsaddr_t mask, nsaddr_t select)
{
	route_item *r = rtable_head();
	for (; r!=NULL; r=r->next()) {
		if (r->daddr_seg()==dest_seg && r->mask()==mask) {
			break;
		}
	}

	mpath *p = mptable_head();
	for (; p!=NULL; p=p->next_select()) {
		if (p->select_addr() == select) {
			break;
		}
	}

	if (r!=NULL && p!=NULL) {
		r->mp_flag() = FALSE;
		r->mptable() = NULL;
	}
}

bool mpRtable::find_MP(nsaddr_t daddr)
{
	route_item *r = rtable_head();
	for (; r!=NULL; r=r->next()) {
		if ((daddr&r->mask()) == r->daddr_seg())
			return r->mp_flag();
	}
	return FALSE;
}

nsaddr_t mpRtable::lookup(nsaddr_t daddr)
{
	route_item *r = rtable_head();
	for (; r!=NULL; r=r->next()) {
		if ((daddr&r->mask()) == r->daddr_seg()) {
			return r->next_hop();
		}
	}
	return IP_BROADCAST;
}

nsaddr_t mpath::select_path(nsaddr_t& s_intf, nsaddr_t& d_intf )
{
	mpath_item* tmp = pathHead();
	mpath_item* ans;
	int next;
	bool tag;

	if (pathHead() == NULL)
		return IP_BROADCAST;

	for (ans=NULL, tag=FALSE, next=0; tmp!=NULL; tmp=tmp->next_path()) {
		if (!is_alive(tmp->s_intf(), tmp->d_intf()))
			continue;
		if (!tag || next>tmp->cnt()) {
			tag = TRUE;
			ans = tmp;
		}
	}

	if (ans == NULL)
		return IP_BROADCAST;
	else {
		s_intf = ans->s_intf();
		d_intf = ans->d_intf();
		ans->cnt()++;
		return select_addr();
	}
}

nsaddr_t mpRtable::lookup(nsaddr_t daddr, nsaddr_t& s_intf, nsaddr_t& d_intf)
{
	route_item *r = rtable_head();
	for (; r!=NULL; r=r->next()) {
		if ((daddr&r->mask()) == r->daddr_seg()) {
			if (r->mp_flag()==TRUE && r->mptable()!=NULL) {
				return r->mptable()->select_path(s_intf, d_intf);
			}
		}
	}
	return IP_BROADCAST;
}

void mpRtable::update_sdelay(nsaddr_t select, nsaddr_t s_intf, nsaddr_t d_intf, double t, unsigned int seq)
{
	mpath *p = mptable_head();
	for (; p!=NULL; p=p->next_select()) {
		if (p->select_addr() == select) {
			p->update_send_delay(s_intf, d_intf, t, seq);
		}
	}
}

void mpRtable::update_rdelay(nsaddr_t select, nsaddr_t s_intf, nsaddr_t d_intf, double t)
{
	mpath *p = mptable_head();
	for (; p!=NULL; p=p->next_select()) {
		if (p->select_addr() == select) {
			p->update_recv_delay(s_intf, d_intf, t);
		}
	}

}
/*
void mpRtable::send_ping_pkt(nsaddr_t addr)
{
	mpath *p =mptable_head();
	for (; p!=NULL; p=p->next_select()) {
		p->send_ping_by_path(addr);
	}
}
*/

bool mpRtable::get_path_item(nsaddr_t& select, mpath_item& path)
{
	static mpath *s = NULL;
	static mpath_item *p = NULL;

	if (mptable_head() == NULL) {
		return FALSE;
	}

	if (p == NULL && s == NULL) {
		s = mptable_head();
		p = s->pathHead(); 
	}

	for (; s!=NULL; s=s->next_select()) {
		if (p == NULL)
			p = s->pathHead();
		else 
			p = p->next_path();
		if (p!=NULL) {
			select = s->select_addr();
			path = (*p);
			char strIP[16];
			printf("( %s ", addr2str(s->select_addr(), strIP));
			printf("%s)->", addr2str(p->s_intf(), strIP));
			printf("( %s ", addr2str(select, strIP));
			printf("%s)",   addr2str(path.s_intf(), strIP));
			return TRUE;
		}
	}
	return FALSE;
}

void mpath::update_ping(nsaddr_t s_intf, nsaddr_t d_intf)
{
	mpath_item *p = pathHead();

	while (p != NULL) {
		if (p->s_intf()==s_intf && p->d_intf()==d_intf) {
			p->ping_send_time() = CURRENT_TIME;
			p->ping_seq()++;
			break;
		} else {
			p = p->next_path();
		}
	}

}

void mpRtable::update_ping_info(nsaddr_t select, nsaddr_t s_intf, nsaddr_t d_intf)
{
	mpath *p = mptable_head();
	for (; p!=NULL; p=p->next_select()) {
		if (p->select_addr() == select) {
			p->update_ping(s_intf, d_intf);
		}
	}
}

void mpRtable::update_rtime(nsaddr_t select, nsaddr_t s_intf, nsaddr_t d_intf, double t)
{
	mpath *p = mptable_head();
	for (; p!=NULL; p=p->next_select()) {
		if (p->select_addr() == select) {
			p->update_recv_time(s_intf, d_intf, t);
		}
	}
	
}


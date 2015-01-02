#if !SONIC_KERNEL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <linux/socket.h>
#include <net/ethernet.h>
#else /* SONIC_KERNEL */
#include <asm/i387.h>
#include <linux/crc32.h>        // ether_crc
#include <linux/if_ether.h>     //struct ethhdr
#include <linux/slab.h>
#include <linux/time.h>
#endif /* SONIC_KERNEL */
#include <linux/types.h>
#include "sonic.h"

int num_to_send = 1000000;
int need_to_ack = 0;

typedef struct tcp_header tcp_header_t;

/* added variables */
unsigned int client_state, server_state;
uint32_t seq = 0;
uint32_t ack = 0;
int window = 1000;
void pack_uint16(uint16_t val, uint8_t* buf) {
    val = htons(val);
    memcpy(buf, &val, sizeof(uint16_t));
}

uint16_t unpack_uint16(const uint8_t* buf) {
    uint16_t val;
    memcpy(&val, buf, sizeof(uint16_t));
    return ntohs(val);
}

void pack_uint32(uint32_t val, uint8_t* buf) {
    val = htonl(val);
    memcpy(buf, &val, sizeof(uint32_t));
}

uint32_t unpack_uint32(const uint8_t* buf) {
    uint32_t val;
    memcpy(&val, buf, sizeof(uint32_t));
    return ntohl(val);
}

inline tcp_header_t* get_tcph(struct sonic_packet* packet)
{
    uint8_t *eth = packet->buf+8;
    uint8_t *ip = eth + ETH_HLEN + SONIC_VLAN_ADD;
    struct tcp_header_t * tcp = (struct tcp_header_t *) (ip+IP_HLEN);
    return tcp;
}

/*end of tcp variables */
inline int sonic_update_csum_dport_id(uint8_t *p, int id, 
	int num_queue, int port_base)
{
    struct iphdr *iph = (struct iphdr *) (p + PREAMBLE_LEN + ETH_HLEN + SONIC_VLAN_ADD);
    struct udphdr *uh = (struct udphdr *) (((uint8_t *) iph) + IP_HLEN);
    uint32_t *pid = (uint32_t *) (((uint8_t *) uh) + TCP_HLEN);
    int xid=*pid;
    uint64_t tmp;
    short pdest = uh->dest, pcsum = uh->check;

    uh->dest = htons(port_base + id % num_queue);
    *pid = htonl(id);

    uh->check = 0;

    tmp = (~pcsum &0xffff)+ (~pdest&0xffff )+ htons(port_base + id % num_queue)
	+ (~(xid >> 16) &0xffff)+ (~(xid & 0xffff)&0xffff) + htons(id>>16) + htons(id &0xffff);

    while (tmp>>16)
	tmp = (tmp & 0xffff) + (tmp >> 16);
    if (tmp == 0xffff)
	tmp = 0;

    uh->check = ~tmp;

#if !SONIC_KERNEL
    //    SONIC_DDPRINT("xid = %x id = %x, num_queue = %d, port_base = %d (%x)\n", xid, id, num_queue, port_base, port_base);
    //    SONIC_DDPRINT("%.4x %.4x (%.4x) %.4x \n", ntohs(pcsum), ntohs(pdest), ~ntohs(pdest), port_base + id%num_queue);
    //    SONIC_DDPRINT("pdest = %.4x, pid = %x, pcheck = %x, dest = %x, id = %x, check = %x, tmp = %x (%x)\n",
    //            pdest, xid, pcsum, uh->dest, id, uh->check,  (uint32_t) tmp, (uint32t) ~tmp);

    //  if ((ncsum &0xffff )!= (uh->check & 0xffff)) {
    //      SONIC_ERROR("csum error %x %x %x %x\n", xid, id, ncsum, uh->check);
    //      exit(1);
    //  }
    //  if (id == 0xfffff)
    //  if (id == 0x3)
    //      exit(1);
#endif
    return 0;
}

static inline int retrieve_bit(char * msg, int msg_len, int id)
{
    int offset,byte_offset, bit_offset, bit;

    if (id==0)
	return 0;

    offset = (id-1) % (msg_len * 8);
    byte_offset = offset >> 3;
    bit_offset = offset & 0x7;
    bit = (msg[byte_offset] >> bit_offset) & 0x1;

    SONIC_DDPRINT("id = %d offset = %d byte_offset = %d bit_offset = %d, bit = %d\n",
	    id, offset, byte_offset, bit_offset, bit);

    return bit;
}

/**
inline int sonic_update_fifo_pkt_gen(struct sonic_packets *packets, 
	struct sonic_port_info *info, int tid, uint64_t default_idle)
{

    struct sonic_packet *packet;
    int i, covert_bit;
    uint32_t *pcrc, crc, idle;

    FOR_SONIC_PACKETS(i, packets, packet) {
	if (unlikely(tid == info->pkt_cnt)) {
	    packet->len = 0;
	    packet->idle = default_idle;
	    packets->pkt_cnt = i == 0? 1 : i; 
	    return tid;
	}

	idle = info->idle;
	switch(info->gen_mode) {
	    case SONIC_PKT_GEN_CHAIN:
		if (tid % info->chain_num == 0) 
		    idle = info->chain_idle;
		break;
	    case SONIC_PKT_GEN_COVERT:
		covert_bit = retrieve_bit(info->covert_msg, strlen(info->covert_msg), tid);
		if (covert_bit)
		    idle = ((info->idle - info->delta)< 12) ? 12 : (info->idle - info->delta);
		else
		    idle = info->idle + info->delta;
		break;
	}
	if (unlikely(i == 0)){
	    packet->len = info->pkt_len + 8;
	    packet->idle = tid == 0 ? info->delay : idle;
	} else{
	    packet->idle = idle;
	}
	pcrc = (uint32_t *) CRC_OFFSET(packet->buf, packet->len);
	*pcrc = 0;
	tid++;
    }

    packets->pkt_cnt = i;

    return tid;
}
*/
inline int sonic_tcp_send(struct sonic_packets *packets, 
	struct sonic_port_info *info, int tid, uint64_t default_idle, uint8_t flag, uint32_t s, uint32_t a)
{

    struct sonic_packet *packet;
    int i, covert_bit;
    uint32_t *pcrc, crc, idle;
    FOR_SONIC_PACKETS(i, packets, packet) {
	if (unlikely(tid == info->pkt_cnt)) {
	    packet->len = 0;
	    packet->idle = default_idle;
	    packets->pkt_cnt = i == 0? 1 : i; 
	    return tid;
	}
	//info->flag = flag;
	//info->seq_number = s;
	//info->ack_number = a;
	//sonic_fill_frame(info, packet->buf, info->pkt_len);
	uint8_t *data = packet->buf;
	struct ethhdr *eth = (struct ethhdr *) (data + PREAMBLE_LEN);       // preamble
	struct iphdr *ip = (struct iphdr *) (((uint8_t *) eth) + ETH_HLEN);
	struct tcphdr *tcp = (struct tcphdr *) (((uint8_t *) ip) + IP_HLEN);
	tcp->seq = htonl(s);
	tcp->ack_seq = htonl(a);
	memcpy(((uint8_t*)tcp)+13,&flag,sizeof(uint8_t));
	s++;
	idle = info->idle;
	switch(info->gen_mode) {
	    case SONIC_PKT_GEN_CHAIN:
		if (tid % info->chain_num == 0) 
		    idle = info->chain_idle;
		break;
	    case SONIC_PKT_GEN_COVERT:
		covert_bit = retrieve_bit(info->covert_msg, strlen(info->covert_msg), tid);
		if (covert_bit)
		    idle = ((info->idle - info->delta)< 12) ? 12 : (info->idle - info->delta);
		else
		    idle = info->idle + info->delta;
		break;
	}

	if (unlikely(i == 0)){
	    packet->len = info->pkt_len + 8;
	    packet->idle = tid == 0 ? info->delay : idle;
	} else
	    packet->idle = idle;
	pcrc = (uint32_t *) CRC_OFFSET(packet->buf, packet->len);
	*pcrc = 0;
	tid++;
    }
    packets->pkt_cnt = i;

    return tid;
}

inline int sonic_update_fifo_pkt_gen(struct sonic_packets *packets,
        struct sonic_port_info *info, int tid, uint64_t default_idle)
{
    struct sonic_packet *packet;
    int i, covert_bit;
    uint32_t *pcrc, crc, idle;

    FOR_SONIC_PACKETS(i, packets, packet) {
        if (unlikely(tid == info->pkt_cnt)) {
            packet->len = 0;
            packet->idle = default_idle;
            packets->pkt_cnt = i == 0? 1 : i;
            return tid;
        }

//        SONIC_PRINT("%d\n", tid);

        sonic_update_csum_dport_id(packet->buf, tid, info->multi_queue,
                info->port_dst);

        idle = info->idle;
        switch(info->gen_mode) {
        case SONIC_PKT_GEN_CHAIN:
            if (tid % info->chain_num == 0)
                idle = info->chain_idle;
            break;
        case SONIC_PKT_GEN_COVERT:
            covert_bit = retrieve_bit(info->covert_msg, strlen(info->covert_msg), tid);
            if (covert_bit)
                idle = ((info->idle - info->delta)< 12) ? 12 : (info->idle - info->delta);
            else
                idle = info->idle + info->delta;
            break;
        }

        if (unlikely(i == 0)){
            packet->len = info->pkt_len + 8;
            packet->idle = tid == 0 ? info->delay : idle;
        } else
            packet->idle = idle;

        pcrc = (uint32_t *) CRC_OFFSET(packet->buf, packet->len);
        *pcrc = 0;
        crc = SONIC_CRC(packet) ^ 0xffffffff;
        *pcrc = crc;

        tid++;
    }

    packets->pkt_cnt = i;

    return tid;
}


//tcp send
int sonic_mac_pkt_generator_loop(void *args)
{   
    int count;
    SONIC_THREAD_COMMON_VARIABLES(mac, args);
    int isClient = (mac->port_id == 0);
    unsigned int state;
    if (mac->port_id == 0){
	client_state = WAITING_FOR_SYNACK;
	SONIC_DPRINT("CLIENT\n");
    } else {
	SONIC_DPRINT("SERVER\n");
	server_state = WAITING_FOR_SYN;
    }



    struct sonic_fifo *out_fifo = mac->out_fifo;
    struct sonic_port_info *info = &mac->port->info;
    struct sonic_packets *packets;
    struct sonic_mac_stat *stat = &mac->stat;

    int tid=0, tcnt=0;
    uint64_t default_idle = power_of_two(out_fifo->exp) * 496;

    SONIC_DPRINT("\n");

    tcnt = sonic_prepare_pkt_gen_fifo(out_fifo, info);


    //START_CLOCK();

    if (sonic_gen_idles(mac->port, out_fifo, info->wait * 100000))
	goto end;

    if (isClient) { //client
	
	while (client_state == WAITING_FOR_SYNACK){
	    packets = (struct sonic_packets *) get_write_entry(out_fifo);
	    if (!packets) goto end;
	    packets->pkt_cnt = 1;
	    tid = sonic_tcp_send(packets, info, tid, default_idle, FLAG_SYN, seq, 0);
	    put_write_entry(out_fifo, packets);
	    if (sonic_gen_idles(mac->port, out_fifo, 1)) goto end;

	}
	packets = (struct sonic_packets *) get_write_entry(out_fifo);
	if (!packets) goto end;
	packets->pkt_cnt = 3;
	tid = sonic_tcp_send(packets, info, tid, default_idle, FLAG_ACK, seq, 0);

	put_write_entry(out_fifo, packets);
	
	START_CLOCK();
	unsigned int len = num_to_send;
	int num_sent = 0;
	int start_seq = seq;
		//test
		SONIC_DPRINT("tcnt = %d\n", tcnt);
		//while (num_sent < len) {
	/**
	begin:
	    packets = (struct sonic_packets *) get_write_entry(out_fifo);
		    if (!packets) 
			goto end;
		    packets->pkt_cnt = tcnt;
		    //tid = sonic_update_fifo_pkt_gen(packets, info, tid, default_idle);
		    tid = sonic_tcp_send(packets, info, tid, default_idle, 0, seq, 0);
		    put_write_entry(out_fifo, packets);
		    count++;
		//}
	    if (*stopper == 0)
	        goto begin;
	    else
		goto end;
	*/

	
	while (num_sent < len) {
	    int i;
	    int s = seq;
	    int start = s;
	    int sum = 0;
	    while (sum< window) {
		packets = (struct sonic_packets *) get_write_entry(out_fifo);
		if (!packets) 
		    goto end;
		if (window - sum > tcnt) {
		    packets->pkt_cnt = tcnt;
		}else {
		    packets->pkt_cnt = window - sum;
		}
		tid = sonic_tcp_send(packets, info, tid, default_idle, 0, s, 0);
		put_write_entry(out_fifo, packets);
		s += packets->pkt_cnt;
		sum += packets->pkt_cnt;
	    }	

	    if (sonic_gen_idles(mac->port, out_fifo, 40)) 
		goto end;
	    
	    SONIC_DPRINT("window = %d\n", window);
	    
	    if (sum == seq - start) {
		window = window + 1;
	    } else {
		window = window / 2;
		if (window == 0)
		    window = 1;
	    }
	    num_sent = seq - start_seq;
	}  
	
	client_state = WAITING_FOR_FINACK;

	while (client_state == WAITING_FOR_FINACK) {
	    packets = (struct sonic_packets *) get_write_entry(out_fifo);

	    if (!packets) 
		goto end;

	    packets->pkt_cnt = 1;
	    tid = sonic_tcp_send(packets, info, tid, default_idle, FLAG_FIN, seq, 0);
	    put_write_entry(out_fifo, packets);
	    if (sonic_gen_idles(mac->port, out_fifo, 1)) goto end;
	}
    }else{ //server
	while (server_state == WAITING_FOR_SYN){
	    if (sonic_gen_idles(mac->port, out_fifo, 10)) goto end;
	}
	while (server_state == WAITING_FOR_ACK){
	    packets = (struct sonic_packets *) get_write_entry(out_fifo);
	    if (!packets) goto end;
	    packets->pkt_cnt = 1;
	    tid = sonic_tcp_send(packets, info, tid, default_idle, FLAG_SYNACK,0, ack);
	    put_write_entry(out_fifo, packets);
	}

	START_CLOCK();

	while (server_state == CONNECTED) {
	    packets = (struct sonic_packets *) get_write_entry(out_fifo);
	    if (!packets) goto end;
	    tid = sonic_tcp_send(packets, info, tid, default_idle, FLAG_ACK, 0, ack);
	    put_write_entry(out_fifo, packets);
	}
	packets = (struct sonic_packets *) get_write_entry(out_fifo);
	if (!packets) goto end;
	packets->pkt_cnt = tcnt;
	tid = sonic_tcp_send(packets, info, tid, default_idle, FLAG_FINACK, 0, ack);
	put_write_entry(out_fifo, packets);
    }

    if (sonic_gen_idles(mac->port, out_fifo, 1000*1000))
	goto end;
end:
    STOP_CLOCK(stat);
    stat->total_bytes = (uint64_t) tid * (info->pkt_len +8);
    stat->total_packets = tid;

    return 0;
}

int sonic_mac_tx_loop(void *args)
{
    SONIC_THREAD_COMMON_VARIABLES(mac, args);
    struct sonic_fifo * in_fifo = mac->in_fifo;
    struct sonic_fifo * out_fifo = mac->out_fifo;
    struct sonic_packets *packets;
    struct sonic_packet *packet;
    struct sonic_mac_stat *stat = &mac->stat;

    int i;
    uint32_t *pcrc, crc;

    SONIC_DPRINT("\n");

    START_CLOCK();

begin:
    packets = (struct sonic_packets *) get_read_entry(in_fifo);
    if (!packets)
	goto end;

    FOR_SONIC_PACKETS(i, packets, packet) {
	if (packet->len == 0)
	    continue;

	pcrc = (uint32_t *) CRC_OFFSET(packet->buf, packet->len);

	stat->total_bytes += packet->len;
	stat->total_packets ++;

	*pcrc = 0;
	crc = SONIC_CRC(packet) ^ 0xffffffff;
	*pcrc = crc;
    }

    push_entry(in_fifo, out_fifo, packets);

    if (*stopper == 0)
	goto begin;
end:
    STOP_CLOCK(stat);

    return 0;
}

//tcp receive
int sonic_mac_rx_loop(void *args)
{
    SONIC_THREAD_COMMON_VARIABLES(mac, args);
    int type;
    int isClient = (mac->port_id == 0);
    if (mac->port_id == 0){
	type = TYPE_CLIENT;
	SONIC_DPRINT("CLIENT\n");
    } else {
	type = TYPE_SERVER;
	SONIC_DPRINT("SERVER\n");
    }   
    struct sonic_fifo *in_fifo = mac->in_fifo;
    struct sonic_packets *packets;
    struct sonic_packet *packet;
    struct sonic_mac_stat *stat = &mac->stat;
    int i;
    uint32_t crc;

    SONIC_DPRINT("\n");

    START_CLOCK();
    /* variables for tcp */
    uint16_t src_port = 0;
    uint16_t dst_port = 0;
    uint32_t ack_l = 0;
    uint32_t seq_l = 0;
    int resend = 0;
    int size;
    /*end of declaration */
begin:
    packets = (struct sonic_packets *) get_read_entry(in_fifo);
    if (!packets)
	goto end;

    FOR_SONIC_PACKETS(i, packets, packet) {
	if (packet->len < 72) {         // FIXME
	    continue;
	}

	stat->total_bytes += packet->len;
	stat->total_packets ++;

	// TODO update TCP state machine
	tcp_header_t* tcph = get_tcph(packet);
	src_port = unpack_uint16(tcph->src_port);
	dst_port = unpack_uint16(tcph->dst_port);
	seq_l = unpack_uint32(tcph->seq_num); 
	ack_l = unpack_uint32(tcph->ack_num_b); 
	if(tcph->flags == FLAG_RST)
	{
	    client_state = CLOSED;
	    server_state = CLOSED;
	    continue;
	}
	//server:
	if(type == TYPE_SERVER)
	{
	    if(tcph->flags == FLAG_SYN && server_state == WAITING_FOR_SYN)
	    {   
		server_state = WAITING_FOR_ACK;
		if(ack != seq_l) 
		    continue;
		ack  = seq+1;
		continue;
	    }
	    else if(tcph->flags == FLAG_ACK && server_state == WAITING_FOR_ACK)
	    {	
		if(ack != seq_l) 
		    continue; 
		server_state = CONNECTED;
		continue;
	    }
	    else if(tcph->flags == 0 && server_state == CONNECTED)
	    {
		if(ack == seq_l)
		    ack++;
		need_to_ack = 1;
		continue;
	    }
	    else if (tcph->flags == FLAG_FIN) {
		server_state = CLOSED;
		if(ack == seq_l) 
		    ack++;
	    }
	}
	if(type == TYPE_CLIENT)
	{
	    if(tcph->flags == FLAG_SYNACK && client_state ==WAITING_FOR_SYNACK)
	    {   
		seq = seq+1;
		client_state = CONNECTED;
		continue;
	    }
	    else if(tcph->flags == FLAG_ACK )
	    {
		if(ack_l <= seq) continue; 
		seq = ack_l;
		continue;
	    }
	    else if(tcph->flags == FLAG_FINACK){
		client_state = CLOSED;
	    } 
	}
    }

    put_read_entry(in_fifo, packets);

    if (*stopper == 0)
	goto begin;
end:
    STOP_CLOCK(stat);

    return 0;
}

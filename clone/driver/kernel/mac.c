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

int num_to_send = 10000;
int need_to_ack = 0;

typedef struct tcp_header tcp_header_t;

/* added variables */
unsigned int client_state, server_state;
uint32_t seq = 0;
uint32_t ack = 0;
int window = 1;
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
    //SONIC_DPRINT("CHECKSUM\n");
    struct iphdr *iph = (struct iphdr *) (p + PREAMBLE_LEN + ETH_HLEN + SONIC_VLAN_ADD);
    struct udphdr *uh = (struct udphdr *) (((uint8_t *) iph) + IP_HLEN);
    uint32_t *pid = (uint32_t *) (((uint8_t *) uh) + TCP_HLEN);
    int xid=*pid;
    uint64_t tmp;
    short pdest = uh->dest, pcsum = uh->check;

    uh->dest = htons(port_base + id % num_queue);
    *pid = htonl(id);

//    SONIC_PRINT("%d\n", ntohl(*pid));

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

    //:SONIC_DPRINT("CHECKSUM END\n");
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


inline int sonic_update_fifo_pkt_gen(struct sonic_packets *packets, 
        struct sonic_port_info *info, int tid, uint64_t default_idle)
{
    
    //SONIC_DPRINT("update begin\n");
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

        //sonic_update_csum_dport_id(packet->buf, tid, info->multi_queue, 
        //        info->port_dst);

	//SONIC_DPRINT("update A\n");
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
	//SONIC_DPRINT("update B\n");

        if (unlikely(i == 0)){
	//SONIC_DPRINT("update C\n");
            packet->len = info->pkt_len + 8;
	//SONIC_DPRINT("update D\n");
            packet->idle = tid == 0 ? info->delay : idle;
	//SONIC_DPRINT("update E\n");
        } else
	//SONIC_DPRINT("update F\n");
            packet->idle = idle;
	//SONIC_DPRINT("update G\n");

        pcrc = (uint32_t *) CRC_OFFSET(packet->buf, packet->len);
	//SONIC_DPRINT("update H\n");
        *pcrc = 0;
	//SONIC_DPRINT("update I\n");
        //crc = SONIC_CRC(packet) ^ 0xffffffff;
	//SONIC_DPRINT("update J\n");
        //*pcrc = crc;
	//SONIC_DPRINT("update K\n");

        tid++;
	//SONIC_DPRINT("update L\n");
    }
    //SONIC_DPRINT("update M\n");

    packets->pkt_cnt = i;
    //SONIC_DPRINT("update end\n");

    return tid;
}

inline int sonic_tcp_send(struct sonic_packets *packets, 
        struct sonic_port_info *info, int tid, uint64_t default_idle, uint8_t flag, uint32_t s, uint32_t a)
{
   
    //SONIC_DPRINT("update begin\n");
    struct sonic_packet *packet;
    int i, covert_bit;
    uint32_t *pcrc, crc, idle;
    //SONIC_DPRINT("packets->pkt_cnt = %d %d\n", packets->pkt_cnt, tid);
    int j = 0;
    FOR_SONIC_PACKETS(i, packets, packet) {
        if (unlikely(tid == info->pkt_cnt)) {
            packet->len = 0;
            packet->idle = default_idle;
            packets->pkt_cnt = i == 0? 1 : i; 
            return tid;
        }
	    info->flag = flag;
	    info->seq_number = s;
	    info->ack_number = a;
	    j ++;
	    sonic_fill_frame(info, packet->buf, info->pkt_len);
	s++;
	//SONIC_DPRINT("add packet %d seq = %d, ack = %d, flag = %d\n",i,s,a,info->flag);
        //sonic_update_csum_dport_id(packet->buf, tid, info->multi_queue, 
        //        info->port_dst);

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
    //SONIC_PRINT("%d \n", packets->pkt_cnt);

    return tid;
}

//tcp send
int sonic_mac_pkt_generator_loop(void *args)
{   
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
    
    int tid=1, tcnt=0;
    uint64_t default_idle = power_of_two(out_fifo->exp) * 496;

    SONIC_DPRINT("\n");

    tcnt = sonic_prepare_pkt_gen_fifo(out_fifo, info);
    

    //START_CLOCK();

    if (sonic_gen_idles(mac->port, out_fifo, info->wait))
        goto end;

    if (isClient) { //client
	    //packets = (struct sonic_packets *) get_write_entry(out_fifo);
	    //packets->pkt_cnt = 1;
	    //tid = sonic_tcp_send(packets, info, tid, default_idle, 99, seq, 0);
	    //put_write_entry(out_fifo, packets);
	//SONIC_DPRINT("client initial state = %d\n", client_state);
	while (client_state == WAITING_FOR_SYNACK){
	    SONIC_DPRINT("CLIENT sending SYN\n");
	    packets = (struct sonic_packets *) get_write_entry(out_fifo);
	    if (!packets) goto end;
	    packets->pkt_cnt = 1;
	    //tid = sonic_update_fifo_pkt_gen(packets, info, tid, default_idle);
	    tid = sonic_tcp_send(packets, info, tid, default_idle, FLAG_SYN, seq, 0);

	    /**if (sonic_gen_idles(mac->port, out_fifo, )) {
		goto end;
	    }*/
	    put_write_entry(out_fifo, packets);
	    SONIC_DPRINT("stat->total_time = %ld, total_packets = %ld\n", stat->total_time, stat->total_packets); 
	    sonic_gen_idles(mac->port, out_fifo, 1);
	    
	}
	    //SONIC_DPRINT("CLIENT rdy to ACK(in handshake)\n");
	    //SONIC_DPRINT("CLIENT sending ACK(in handshake)\n");
	   //send ack in threeway handshake 
	    packets = (struct sonic_packets *) get_write_entry(out_fifo);
	    if (!packets) goto end;
	    packets->pkt_cnt = 3;
	    //tid = sonic_update_fifo_pkt_gen(packets, info, tid, default_idle);
	    tid = sonic_tcp_send(packets, info, tid, default_idle, FLAG_ACK, seq, 0);

	    /**if (sonic_gen_idles(mac->port, out_fifo, )) {
		goto end;
	    }*/
	    put_write_entry(out_fifo, packets);

    START_CLOCK();
	unsigned int len = num_to_send;
	int num_sent = 0;
	int start_seq = seq;
	while (num_sent < len) {
	    //SONIC_DPRINT("window = %d\n", window);
	    int i;
	    int s = seq;
	    int start = s;
	    int sum = 0;
	    //for ( i = 0; i < window; i++) {
		packets = (struct sonic_packets *) get_write_entry(out_fifo);
		if (!packets) goto end;
		//SONIC_DPRINT("sending data\n");
		if (len -  num_sent > window) {
		    packets->pkt_cnt = window;
		} else {
		    packets->pkt_cnt = len - num_sent;
		}
		tid = sonic_tcp_send(packets, info, tid, default_idle, 0, seq, 0);//first 0 is the flag
		put_write_entry(out_fifo, packets);
		s += window;
		sum += window;
		//if (sum + num_sent >= len){
		 //   break;
		//}
	    //}

	    sonic_gen_idles(mac->port, out_fifo, 10);
	    if (sum == seq - start) {
		if (window < tcnt) {
		    window = window +1;
	    }
	    } else {
		window = window / 2;
		if (window == 0) {
		    window = 1;
		}
	    }
	    num_sent = seq - start_seq;
	    //SONIC_DPRINT("num_sent = %d\n", num_sent);
	    //printf("[test_send] num_sent = %d\n", num_sent);
	}
	client_state = WAITING_FOR_FINACK;
	
	while (client_state == WAITING_FOR_FINACK) {
	    packets = (struct sonic_packets *) get_write_entry(out_fifo);
	    if (!packets) goto end;
	    SONIC_DPRINT("sever sending back fin %d\n", ack);
	    packets->pkt_cnt = 1;
	    tid = sonic_tcp_send(packets, info, tid, default_idle, FLAG_FIN, seq, 0);
	    put_write_entry(out_fifo, packets);
	    sonic_gen_idles(mac->port, out_fifo, 1);
	}
	SONIC_DPRINT("end of user send\n");
    }else{ //server
	    //packets = (struct sonic_packets *) get_write_entry(out_fifo);
	    //packets->pkt_cnt=1;
	    //tid = sonic_tcp_send(packets, info, tid, default_idle, 99, seq, 0);
	    //put_write_entry(out_fifo, packets);
	//SONIC_DPRINT("server initial state = %d\n", server_state);
	while (server_state == WAITING_FOR_SYN){
	    //SONIC_DPRINT("SERVER sending idles\n");
	    sonic_gen_idles(mac->port, out_fifo, 10);
	}
	while (server_state == WAITING_FOR_ACK){
	    //SONIC_DPRINT("SERVER sending SYNACK in state %d\n", server_state);
	    
	    packets = (struct sonic_packets *) get_write_entry(out_fifo);
	    if (!packets) goto end;
	    //tid = sonic_update_fifo_pkt_gen(packets, info, tid, default_idle);
	    packets->pkt_cnt = 1;
	    tid = sonic_tcp_send(packets, info, tid, default_idle, FLAG_SYNACK,0, ack);

	    /**if (sonic_gen_idles(mac->port, out_fifo, )) {
		goto end;
	    }*/
	    put_write_entry(out_fifo, packets);
	    //sonic_gen_idles(mac->port, out_fifo, 1);
	}
	START_CLOCK();
	while (server_state == CONNECTED) {
	    if (need_to_ack) {
		packets = (struct sonic_packets *) get_write_entry(out_fifo);
		if (!packets) goto end;
		//SONIC_DPRINT("sever sending back ack %d\n", ack);
		tid = sonic_tcp_send(packets, info, tid, default_idle, FLAG_ACK, 0, ack);
		put_write_entry(out_fifo, packets);
		sonic_gen_idles(mac->port, out_fifo, 1);
		need_to_ack = 0;
	    } else {
		sonic_gen_idles(mac->port, out_fifo, 1);
	    }
	}
	packets = (struct sonic_packets *) get_write_entry(out_fifo);
	if (!packets) goto end;
	packets->pkt_cnt = tcnt;
	//SONIC_DPRINT("sever sending backfinack %d\n", ack);
	tid = sonic_tcp_send(packets, info, tid, default_idle, FLAG_FINACK, 0, ack);
	put_write_entry(out_fifo, packets);

	//SONIC_DPRINT("end of server send\n");
    }
 
/**
begin:
    packets = (struct sonic_packets *) get_write_entry(out_fifo);
    if (!packets)
        goto end;
    
    packets->pkt_cnt = tcnt;
    //SONIC_DPRINT("pkt_cnt = %d\n", tcnt);
    //SONIC_DPRINT("XXX\n");
    tid = sonic_update_fifo_pkt_gen(packets, info, tid, default_idle);
    //SONIC_DPRINT("tid = %d\n", tid);
    //SONIC_DPRINT("YYY\n");

    put_write_entry(out_fifo, packets);

    if (*stopper == 0)
        goto begin;
*/
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

//        SONIC_PRINT("%d %d %d %d %p %p \n", i, packet->len, packet->idle, packet->next, packets, packet);

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
//    uint64_t total_pkt_cnt=0, total_bytes=0, crc_error=0;
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
    //for now, did not considering resend scheme here. 
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
    //SONIC_DPRINT("====ack = %u, seq_l = %u, src_port:%d, dst_port:%d, flag = %d\n", ack_l, seq_l, src_port,dst_port, tcph->flags);
    if(tcph->flags == FLAG_RST)
          {
            client_state = CLOSED;
            server_state = CLOSED;
            continue;
          }
        //server:
        if(type == TYPE_SERVER)
        {
	//SONIC_PRINT("SERVER with state %d received a packet with flag = %d \n", server_state, tcph->flags);
            if(tcph->flags == FLAG_SYN && server_state == WAITING_FOR_SYN)
            {   
		//SONIC_DPRINT("SERVER received SYN, changing state from waiting for syn to waiting for ack\n");
                //dst_port = unpack_uint16(tcph->src_port);
                server_state = WAITING_FOR_ACK;
                //seq_l = unpack_uint32(tcph->seq_num);
                //seq_l = tcph->seq_num;
		//SONIC_PRINT("ack = %ld, seq_l = %ld\n", ack, seq_l);
                if(ack != seq_l) continue;
                ack  = seq+1;
                //add_send_task("", 0 , FLAG_SYN | FLAG_ACK ,seq, ack,window);
                continue;
            }
            else if(tcph->flags == FLAG_ACK && server_state == WAITING_FOR_ACK)
            {	
		//SONIC_DPRINT("SERVER received ACK(handshake), changing state from waiting for ack to connected\n");
		
                //seq_l = unpack_uint32(tcph->seq_num);
                //seq_l = tcph->seq_num;
		//SONIC_PRINT("ack = %ld, seq_l = %ld\n", ack, seq_l);
                if(ack != seq_l) continue; 
                //ack  = seq_l+1;
                //add_send_task("", 0 , FLAG_ACK ,seq, ack,window);
                //printf("connected\n");
                server_state = CONNECTED;
		//SONIC_DPRINT("SERVER connection established; state = %d, ack = %d \n", client_state, ack);
                continue;
            }
	    else if(tcph->flags == 0 && server_state == CONNECTED)
            {
		//SONIC_DPRINT("data received seq = %d\n", seq_l);
                if(ack == seq_l) ack++;
		need_to_ack = 1;
                //add_send_task("", 0 , FLAG_ACK ,seq, ack,window);
                //printf("connected\n");
                //state = CONNECTED;
                continue;
            }
	    else if (tcph->flags == FLAG_FIN) {
		server_state = CLOSED;
                if(ack == seq_l) ack++;
	    }
        }
        if(type == TYPE_CLIENT)
        {
	    //SONIC_DPRINT("CLIENT with state %d received a packet with flag = %d \n", client_state, tcph->flags);
            if(tcph->flags == FLAG_SYNACK && client_state ==WAITING_FOR_SYNACK)
            {   
		//SONIC_DPRINT("CLIENT received SYNACK, changing state from waiting for synack to connected\n");
                seq = seq+1;
                //SONIC_DPRINT("XXXXXseq = %d\n", seq);
		//dst_port = unpack_uint16(tcph->src_port);
                client_state = CONNECTED;
                //ack_l = unpack_uint32(tcph->ack_num);
                //if(seq != ack_l) continue; 
                //ack  = seq_l+1;
                //add_send_task("", 0 , FLAG_SYN | FLAG_ACK ,seq, ack,window);
		//SONIC_DPRINT("CLIENT connection established; state = %d, seq = %d \n", client_state, seq);
                continue;
            }
	    else if(tcph->flags == FLAG_ACK )
            {
                if(ack_l <= seq) continue; 
                seq = ack_l;
                //add_send_task("", 0 , FLAG_ACK ,seq, ack,window);
                //printf("connected\n");
                //state = CONNECTED;
                continue;
            }
	    else if(tcph->flags == FLAG_FINACK){
		client_state = CLOSED;
	    } 
        }
/**
          if( (tcph->flags == FLAG_FINACK || tcph->flags == FLAG_FIN) && state == CONNECTED)
            {
                seq_l = unpack_uint32(tcph->seq_num);
                ack_l = unpack_uint32(tcph->ack_num);
                if(ack != seq_l) continue; 
                state = WAITING_FOR_FIN;
                //ack  = seq_l+1;
                //add_send_task("", 0 , FLAG_ACK ,seq, ack,window);
                //printf("connected\n");
                //state = CONNECTED;
                continue;
            }*/
            
/**
            if( (tcph->flags == FLAG_FINACK || tcph->flags == FLAG_FIN) && state == CONNECTED)
            {
                seq_l = unpack_uint32(tcph->seq_num);
                ack_l = unpack_uint32(tcph->ack_num);
                if(ack != seq_l) continue; 
                //ack  = seq_l+1;
                //add_send_task("", 0 , FLAG_ACK ,seq, ack,window);
                //printf("connected\n");
                state = CLOSED;
                continue;
            }
*/        
//        crc = ~SONIC_CRC(packet);

//        if (crc) {
//            stat->total_error_crc ++;
//        }
    }

    put_read_entry(in_fifo, packets);

    if (*stopper == 0)
        goto begin;
end:
    STOP_CLOCK(stat);

    return 0;
}

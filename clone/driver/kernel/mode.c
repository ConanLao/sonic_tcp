#include "sonic.h"
#if !SONIC_KERNEL
#include <stdlib.h>
#include <string.h>
#endif /* SONIC_KERNEL */

static void sonic_set_fifo_tx_pcs_mac (struct sonic_port * port) {
    struct sonic_fifo *fifo = port->fifo[0];
    port->tx_mac->out_fifo = fifo;
    port->tx_pcs->in_fifo = fifo;
}

static void sonic_set_fifo_tx_pcs_app (struct sonic_port * port) {
    struct sonic_fifo *fifo = port->fifo[0];
    port->tx_pcs->in_fifo = fifo;
    port->app->out_fifo = fifo;
}

static void sonic_set_fifo_tx_pcs_mac_app (struct sonic_port * port) {
    struct sonic_fifo *fifo1 = port->fifo[0];
    struct sonic_fifo *fifo2 = port->fifo[1];
    
    port->tx_pcs->in_fifo = fifo1;
    port->tx_mac->out_fifo = fifo1;
    port->tx_mac->in_fifo = fifo2;
    port->app->out_fifo = fifo2;
}

static void sonic_set_fifo_rx_pcs_mac (struct sonic_port * port) {
    struct sonic_fifo *fifo = port->fifo[0];
    port->rx_pcs->out_fifo = fifo;
    port->rx_mac->in_fifo = fifo;
}

static void sonic_set_fifo_rx_pcs_app (struct sonic_port * port) {
    struct sonic_fifo *fifo = port->fifo[0];
    port->rx_pcs->out_fifo = fifo;
    port->app->in_fifo = fifo;
}

static void sonic_set_fifo_tx_pcs_mac_rx_pcs_app (struct sonic_port * port) {
    struct sonic_fifo *tfifo = port->fifo[0];
    struct sonic_fifo *rfifo = port->fifo[1];

    port->tx_mac->out_fifo = tfifo;
    port->tx_pcs->in_fifo = tfifo;

    port->rx_pcs->out_fifo = rfifo;
    port->app->in_fifo = rfifo;
}

static void sonic_set_fifo_pcs_forwarding_port (struct sonic_port *port) {
    struct sonic_fifo *fifo1 = port->fifo[0];
    struct sonic_fifo *fifo2 = port->fifo[1];

    port->rx_pcs->out_fifo = fifo1;
    port->app->in_fifo = fifo1;

    port->app->out_fifo = fifo2;
    port->tx_pcs->in_fifo = fifo2;
}

static void sonic_set_fifo_tcp(struct sonic_port *port) {
    struct sonic_fifo *fifo1 = port->fifo[0];
    struct sonic_fifo *fifo2 = port->fifo[1];
#if !SONIC_KERNEL
    struct sonic_fifo *pipe = port->fifo[2];
    struct sonic_port *other_port = port->sonic->ports[port->port_id?0:1];
#endif /*SONIC_KERNEL*/

    port->rx_pcs->out_fifo = fifo1;
    port->rx_mac->in_fifo = fifo1;

    port->tx_pcs->in_fifo = fifo2;
    port->tx_mac->out_fifo = fifo2;
#if !SONIC_KERNEL
    port->tx_pcs->out_fifo = pipe;
    other_port->rx_pcs->in_fifo = pipe;
#endif /*SONIC_KERNEL*/
}

static struct sonic_runtime_thread_funcs null_port={};

#if !SONIC_KERNEL
static struct sonic_runtime_thread_funcs crc_tester = {
    .tx_mac = sonic_test_crc_performance,
    .set_fifo = sonic_set_fifo_tx_pcs_mac,
};
static struct sonic_runtime_thread_funcs encode_tester = {
    .tx_pcs = sonic_test_encoder,
    .set_fifo = sonic_set_fifo_tx_pcs_mac,
};
static struct sonic_runtime_thread_funcs decode_tester = {
    .rx_pcs = sonic_test_decoder,
    .set_fifo = sonic_set_fifo_rx_pcs_mac,
};
#endif /* SONIC_KERNEL */

static struct sonic_runtime_thread_funcs pkt_gen = {
    .tx_mac = sonic_mac_pkt_generator_loop,
    .tx_pcs = sonic_pcs_tx_loop,
    .set_fifo = sonic_set_fifo_tx_pcs_mac,
};

static struct sonic_runtime_thread_funcs pkt_rpt = {
    .app = sonic_app_rpt_loop,
    .tx_pcs = sonic_pcs_tx_loop,
    .set_fifo = sonic_set_fifo_tx_pcs_app,
};

static struct sonic_runtime_thread_funcs pkt_vrpt = {
    .app = sonic_app_vrpt_loop,
    .tx_mac = sonic_mac_tx_loop,
    .tx_pcs = sonic_pcs_tx_loop,
    .set_fifo = sonic_set_fifo_tx_pcs_mac_app,
};

#if 0
static struct sonic_runtime_thread_funcs pkt_crpt = {
    .app = sonic_app_crpt_loop,
    .tx_mac = sonic_mac_tx_loop,
    .tx_pcs = sonic_pcs_tx_loop,
    .set_fifo = sonic_set_fifo_tx_pcs_mac_app,
};
#endif

static struct sonic_runtime_thread_funcs pkt_rcv = {
    .rx_mac = sonic_mac_rx_loop,
    .rx_pcs = sonic_pcs_rx_loop,
    .set_fifo = sonic_set_fifo_rx_pcs_mac,
#if SONIC_SFP
    .tx_pcs = sonic_pcs_tx_idle_loop,
#endif
};

static struct sonic_runtime_thread_funcs pkt_cap = {
    .rx_pcs = sonic_pcs_rx_loop,
    .app = sonic_app_cap_loop,
    .set_fifo = sonic_set_fifo_rx_pcs_app,
#if SONIC_SFP
    .tx_pcs = sonic_pcs_tx_idle_loop,
#endif
};

static struct sonic_runtime_thread_funcs pkt_gencap = {
    .tx_mac = sonic_mac_pkt_generator_loop,
    .tx_pcs = sonic_pcs_tx_loop,
    .rx_pcs = sonic_pcs_rx_loop,
    .app = sonic_app_cap_loop,
    .set_fifo = sonic_set_fifo_tx_pcs_mac_rx_pcs_app,
};

static struct sonic_runtime_thread_funcs pcs_forward = {
    .tx_pcs = sonic_pcs_tx_loop,
    .app = sonic_app_cap_loop,
    .rx_pcs = sonic_pcs_rx_loop,
    .set_fifo = sonic_set_fifo_pcs_forwarding_port,
};

static struct sonic_runtime_thread_funcs pkt_tcp = {
    .tx_pcs = sonic_pcs_tx_loop,
    .rx_pcs = sonic_pcs_rx_loop,
    .tx_mac = sonic_mac_pkt_generator_loop,
    .rx_mac = sonic_mac_rx_loop,
    .set_fifo = sonic_set_fifo_tcp,
};

static struct sonic_runtime_thread_funcs arp = {
    .tx_pcs = sonic_pcs_tx_loop,
    .tx_mac = sonic_mac_arp_loop,
#if SONIC_SFP
    .rx_pcs = sonic_pcs_rx_idle_loop,
#endif
    .set_fifo = sonic_set_fifo_tx_pcs_mac,
};

#if 0
static struct sonic_runtime_thread_funcs intr_app = {
    .tx_pcs = sonic_pcs_tx_loop,
    .tx_mac = sonic_mac_tx_loop,
    .app = sonic_app_interactive_loop,
    .rx_pcs = sonic_pcs_rx_loop,
    .set_fifo = sonic_set_fifo_pcs_forwarding_port,
};
#endif

#if SONIC_SFP
static struct sonic_runtime_thread_funcs tx_idle = {
    .tx_pcs = sonic_pcs_tx_idle_loop,
};

static struct sonic_runtime_thread_funcs rx_idle = {
    .rx_pcs = sonic_pcs_rx_idle_loop,
};

static struct sonic_runtime_thread_funcs rxtx_idle = {
    .tx_pcs = sonic_pcs_tx_idle_loop,
    .rx_pcs = sonic_pcs_rx_idle_loop,
};

static struct sonic_runtime_thread_funcs tx_dma = {
    .tx_pcs = sonic_pcs_tx_dma_loop,
};

static struct sonic_runtime_thread_funcs rx_dma = {
    .rx_pcs = sonic_pcs_rx_dma_loop,
};

static struct sonic_runtime_thread_funcs rxtx_dma = {
    .tx_pcs = sonic_pcs_tx_dma_loop,
    .rx_pcs = sonic_pcs_rx_dma_loop,
};
#endif /* SONIC_SFP */

void sonic_set_runtime_threads(struct sonic_priv *sonic)
{
    int i;
    struct sonic_config_runtime_args *args = &sonic->runtime_args;
    struct sonic_config_system_args *sargs = &sonic->system_args;

    for ( i = 0 ; i < SONIC_PORT_SIZE ; i ++) {
#define SONIC_MODE(x,y)     \
        if (strcmp(args->ports[i].mode, #y) == 0) \
            sonic->ports[i]->runtime_funcs = &y;    \
        else
    SONIC_MODE_ARGS
#undef SONIC_MODE_ARGS
        {
            SONIC_PRINT("Unknown mode %s\n", args->ports[i].mode);
            sonic->ports[i]->runtime_funcs = &null_port;
        }
    }

#if 0
    switch(args->mode) {
#define SONIC_MODE(x,y,z,w)       \
    case (x) :                  \
        sonic->ports[0]->runtime_funcs = &z;    \
        if (SONIC_PORT_SIZE == 2)               \
            sonic->ports[1]->runtime_funcs = &w;\
        break;
    SONIC_MODE_ARGS;
#undef SONIC_MODE
    }
#endif
    
    for ( i = 0 ; i < SONIC_PORT_SIZE; i ++ ) {
        struct sonic_port *port = sonic->ports[i];
        unsigned int random;
        if (port->runtime_funcs->set_fifo)
            port->runtime_funcs->set_fifo(port);

        // Set random cpu ids
        random = sonic_random();

#define SONIC_THREADS(x,y,z)   \
        port->x->cpu = SONIC_CPU(sargs, i, (random++)%5+1);
        SONIC_THREADS_ARGS
#undef SONIC_THREADS
        SONIC_PRINT("port[%d] tx_pcs = %d tx_mac = %d rx_pcs = %d rx_mac = %d app = %d\n",
            port->port_id,
            port->tx_pcs->cpu, port->tx_mac->cpu, 
            port->rx_pcs->cpu, port->rx_mac->cpu,
            port->app->cpu);
    }
}

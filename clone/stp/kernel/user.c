#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "dma_cmd.h"
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

#define BUFFER_LENGTH 40
#define BUFF_SIZE 0x80000

#include <termios.h>
#include <unistd.h>
#include "regs.h"

void xgmii_loop(int f, int is_loop);
uint32_t get_errcnt(int f, int portno);

int stp_write_command(int fd, uint32_t offset, uint32_t value)
{
    struct stp_cmd cmd;
    cmd.offset = offset;
    cmd.value = value;

    ioctl(fd, STP_IOCX_WRITE_REG, &cmd);
}

int stp_reset_command(int fd)
{
    ioctl(fd, STP_IOCX_RESET);
}

int stp_read_command(int fd, uint32_t offset)
{
    struct stp_cmd cmd;

    cmd.offset = offset;

    ioctl(fd, STP_IOCX_READ_REG, &cmd);

    printf("%x read\n", cmd.value);

    return cmd.value;
}

void dma_test (int f) {
    FILE *out_f;
    if (f == -1) {
        printf ("Couldn't open the device.\n");
        return;
    } else {
        printf ("Opened the device: file handle #%lu!\n", (long unsigned int)f);
    }
    char *buf = malloc(1<<20);

	// Reset DMA engine
    stp_reset_command(f);
	// Disable STP
    stp_write_command(f, DATA_OUT_0, 0);
	// Enable STP
    stp_write_command(f, DATA_OUT_0, 0xf);

	// set p0 base log address
	uint32_t p0_base_addr = 0;
    stp_write_command(f, DATA_OUT_2, p0_base_addr);
	// set p1 base log address
	uint32_t p1_base_addr = 0x400000;
    stp_write_command(f, DATA_OUT_3, p1_base_addr);
	// set p2 base log address
	uint32_t p2_base_addr = 0x800000;
	stp_write_command(f, DATA_OUT_4, p2_base_addr);
	// set p3 base log address
	uint32_t p3_base_addr = 0xc00000;
	stp_write_command(f, DATA_OUT_5, p3_base_addr);

	// program init_timeout
	uint32_t init_timeout = 0x80;
    stp_write_command(f, DATA_OUT_7, init_timeout);
	// program sync_timeout p0
	uint32_t sync_timeout = 0x200;
    stp_write_command(f, DATA_OUT_8, sync_timeout);
	// program sync_timeout p1
	sync_timeout = 0x1200;
    stp_write_command(f, DATA_OUT_9, sync_timeout);
	// program sync_timeout p1
	sync_timeout = 0x100;
    stp_write_command(f, DATA_OUT_10, sync_timeout);
	// program sync_timeout p1
	sync_timeout = 0x100;
    stp_write_command(f, DATA_OUT_11, sync_timeout);
	// program buff_size
	uint32_t buff_size = BUFF_SIZE;
    stp_write_command(f, DATA_OUT_6, buff_size);
	uint32_t enable_log = 0xf;
    stp_write_command(f, DATA_OUT_1, enable_log);
	// READ delay
    stp_read_command(f, DATA_IN_16);
    stp_read_command(f, DATA_IN_17);

	// log for one second.
	sleep(1);

	// read log
    out_f = fopen("stp_log", "w");

    int bufsize=1<<12;
    int j = 0,k=0;
//    for (k = 0 ; k < 100; k ++) {
        for (j = 0 ;j < buff_size / bufsize  * 2; j ++) {

            int cnt = read(f, buf,bufsize);

            if (cnt < 0) {
                printf("Error");
                exit(EXIT_FAILURE);
            }

//            printf("%d read\n", cnt);

            int i=0;
            for ( ; i < bufsize / 4 ; i++) {
                unsigned int *p = ((unsigned int *) buf) + i;
                fprintf(out_f, "%.8x ", *p);

                if ( i % 8 == 7)
                    fprintf(out_f, "\n");
            }

        fprintf(out_f, "---------------------\n");
        }

        usleep(20000);
 //   }
    fclose(out_f);
}

void exp_1(int f) {
    // timestamp
    char str[80];
    struct timeval tv;
    FILE *out_f;

    char *buf = malloc(1<<20);
    gettimeofday(&tv, NULL);
    sprintf(str, "stp_log.%ld_%06ld", tv.tv_sec, tv.tv_usec);
    out_f = fopen(str, "w");

    int buff_size = BUFF_SIZE;
    int bufsize = 1 << 12;
    int j = 0;

    uint32_t err;

    err = get_errcnt(f, 0);
    fprintf(out_f, "!0 , %d\n", err);
    err = get_errcnt(f, 1);
    fprintf(out_f, "!1 , %d\n", err);

    for (j = 0 ;j < buff_size / bufsize  * 2; j ++) {

        int cnt = read(f, buf,bufsize);

        if (cnt < 0) {
            printf("Error");
            exit(EXIT_FAILURE);
        }

        //            printf("%d read\n", cnt);

        int i=0;
        for ( ; i < bufsize / 4 ; i++) {
            unsigned int *p = ((unsigned int *) buf) + i;
            fprintf(out_f, "%.8x ", *p);

            if ( i % 8 == 7)
                fprintf(out_f, "\n");
        }

        fprintf(out_f, "---------------------\n");
    }

    usleep(20000);
    fclose(out_f);
}

void run_exp (int f, int num) {
    switch (num) {
    case 1:
        exp_1(f);
        break;
    default:
        printf("unknown experiment, exit.\n");
        break;
    }
}

void reset_stp(int f){
    printf("Reset STP...");
    stp_write_command(f, DATA_OUT_0, 0);
    printf("Done\n");
}

void xgmii_loop(int f, int is_loop) {
    uint32_t val = 0;
    printf("Set loop to %d\n", is_loop);
    if (is_loop > 0) {
        val = stp_read_command(f, DATA_OUT_0);
        printf("val = %x\n", val);
        val |= 0x00000F00;
        printf("val = %x\n", val);
        stp_write_command(f, DATA_OUT_0, val);
    } else {
        val = stp_read_command(f, DATA_OUT_0);
        printf("val = %x\n", val);
        val &= 0xFFFFF0FF;
        printf("val = %x\n", val);
        stp_write_command(f, DATA_OUT_0, val);
    }
    printf("Done\n");
}

void reset_cnt(int f){
    uint32_t val;
    printf("Reset cnt...\n");
    val = stp_read_command(f, DATA_IN_1);
    val |= 0x10;
    stp_write_command(f, DATA_OUT_1, val);
    val = stp_read_command(f, DATA_IN_1);
    val &= ~0x10;
    stp_write_command(f, DATA_OUT_1, val);
    stp_read_command(f, DATA_IN_1);
    printf("Done\n");
}

void set_mode(int f, int mode) {
    uint32_t val;
    printf("read reg1..\n");
    val = stp_read_command(f, DATA_IN_1);
    if (mode == 1) {
        val |= 0x100;
        printf("val = %x\n", val);
    } else if (mode == 0) {
        val &= 0xFFFFFEFF;
        printf("val = %x\n", val);
    } else {
        printf("invalid mode!\n");
    }
    stp_write_command(f, DATA_OUT_1, val);
    val = stp_read_command(f, DATA_IN_1);
    printf("Done!\n");
}

void set_filter(int f, int set_filter) {
    uint32_t val;
    printf("read reg1..\n");
    val = stp_read_command(f, DATA_IN_1);
    if (set_filter == 1) {
        val |= 0x200;
        printf("val = %x\n", val);
    } else if (set_filter == 0) {
        val &= 0xFFFFFDFF;
        printf("val = %x\n", val);
    } else {
        printf("invalid value!\n");
    }
    stp_write_command(f, DATA_OUT_1, val);
    val = stp_read_command(f, DATA_IN_1);
    printf("Done!\n");
}

void set_thres(int f, int thres) {
    uint32_t val;
    stp_write_command(f, DATA_OUT_12, thres);
    val = stp_read_command(f, DATA_IN_12);
    printf("read reg 12 = 0x%x\n", val);
}

uint32_t get_errcnt(int f, int portno) {
    uint32_t val;
    switch (portno) {
    case 0:
        val = stp_read_command(f, DATA_IN_18);
        break;
    case 1:
        val = stp_read_command(f, DATA_IN_19);
        break;
    case 2:
        val = stp_read_command(f, DATA_IN_20);
        break;
    case 3:
        val = stp_read_command(f, DATA_IN_21);
        break;
    default:
        printf("Invalid port no.\n");
        break;
    }
    return val;
}

void print_registers(int f) {
    uint32_t val;
    int i;

    // Reg[0]
    val = stp_read_command(f, DATA_IN_0);

    // CLKSYNC_ENABLE
    for (i = CLKSYNC_ENABLE; i < CLKSYNC_ENABLE + 4; i ++) {
        printf("Port %d ENABLE: %s\n", i - CLKSYNC_ENABLE,
                               (val & (1 << i)) ? "true" : "false");
    }

    // CLKSYNC_BYPASS
    for (i = CLKSYNC_BYPASS; i < CLKSYNC_BYPASS + 4; i ++) {
        printf("Port %d BYPASS: %s\n", i - CLKSYNC_BYPASS,
                               (val & (1 << i)) ? "true" : "false");
    }

    // XGMII_LOOPBACK
    for (i = XGMII_LOOPBACK; i < XGMII_LOOPBACK + 4; i ++) {
        printf("Port %d XGMII_LPBK: %s\n", i - XGMII_LOOPBACK,
                               (val & (1 << i)) ? "true" : "false");
    }

    // Reg[1]
    val = stp_read_command(f, DATA_IN_1);
    for (i = LOG_ENABLE; i < LOG_ENABLE + 4; i ++) {
        printf("Port %d LOG_ENABLE: %s\n", i - LOG_ENABLE,
                               (val & (1 << i)) ? "true" : "false");
    }

    printf("Global counter reset: %s\n", (val & (1 << GLOBAL_RESET)) ? "true" : "false");

    printf("Device Mode: %s\n", (val & (1 << OP_MODE)) ? "SWITCH" : "NIC");

    printf("Filter Large Number: %s\n", (val & (1 << DISABLE_FILTER)) ? "false" : "true");

    // Reg[16]
    val = stp_read_command(f, DATA_IN_16);
    printf("P0 Delay 0x%x\n", val & 0xFFFF);
    printf("P1 Delay 0x%x\n", (val & 0xFFFF0000) >> 16);

    val = stp_read_command(f, DATA_IN_17);
    printf("P2 Delay 0x%x\n", val & 0xFFFF);
    printf("P3 Delay 0x%x\n", (val & 0xFFFF0000) >> 16);

    val = stp_read_command(f, DATA_IN_12);
    printf("Filter Threshold 0x%x0000\n", val);
}


void usage() {
   printf("user [-acflmpr]\n");
   printf("  -p: print current register settings\n");
   printf("  -a: dma_test\n");
   printf("  -r: disable stp\n");
   printf("  -c: reset global counter\n");
   printf("  -l: set xgmii loopback\n");
   printf("  -m: set mode [0 for nic(default), 1 for switch]\n");
   printf("  -f: set filter [0 for enable(default), 1 for disable]\n");
}

long hex2dec(char * a) {
    char c;
    long n = 0;

    while (*a) {
        c = *a++;
        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c >= 'a' && c <= 'f') {
            c = (c - 'a') + 10;
        } else if (c >= 'A' && c <= 'F') {
            c = (c - 'A') + 10;
        } else {
            goto INVALID;
        }
        n = (n << 4) + c;
    }
    return n;
INVALID:
    return -1;
}

int main (int argc, char **argv) {
    int f = open ("/dev/altera_dma", O_RDWR);
    int dma_flag = 0, reset_flag = 0;
    int reset_cnt_flag = 0;
    int xgmii_loop_flag = 0;
    int set_mode_flag = 0;
    int set_filter_flag = 0;
    int set_thres_flag = 0;
    int run_exp_flag = 0;
    int error_cnt_flag = 0;
    int is_loop = 0;
    int mode = 0;
    int filter = 0;
    int thres = 0;
    int exp_num = 0;
    int port_no = 0;
    int c;

    while ((c = getopt(argc, argv, "ae:rpcl:m:f:t:n:")) != -1) {
        switch (c) {
            case 'a':
                dma_flag = 1;
                break;
            case 'n':
                error_cnt_flag = 1;
                port_no = atoi(optarg);
                break;
            case 'e':
                run_exp_flag = 1;
                exp_num = atoi(optarg);
                break;
            case 'r':
                reset_flag = 1;
                break;
            case 'c':
                reset_cnt_flag = 1;
                break;
            case 'l':
                xgmii_loop_flag = 1;
                is_loop = atoi(optarg);
                break;
            case 'm':
                set_mode_flag = 1;
                mode = atoi(optarg);
                break;
            case 'f':
                set_filter_flag = 1;
                filter = atoi(optarg);
                break;
            case 'p':
                print_registers(f);
                break;
            case 't':
                set_thres_flag = 1;
                thres = hex2dec(optarg);
                break;
            default:
                usage();
        }
    }

    if (reset_flag) {
        reset_stp(f);
    }

    if (reset_cnt_flag) {
        reset_cnt(f);
    }

    if (xgmii_loop_flag) {
        xgmii_loop(f, is_loop);
    }

    if (set_mode_flag) {
        set_mode(f, mode);
    }

    if (set_filter_flag) {
        set_filter(f, filter);
    }

    if (set_thres_flag) {
        set_thres(f, thres);
    }

    if (error_cnt_flag) {
        printf("Port %d error cnt = 0x%x\n", port_no, get_errcnt(f, port_no));
    }

    if (dma_flag) {
        dma_test(f);
    }

    if (run_exp_flag) {
        run_exp(f, exp_num);
    }

    close(f);
    return 0;
}


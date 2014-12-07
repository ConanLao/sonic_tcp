#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "dma_cmd.h"
#include <stdint.h>

#define BUFFER_LENGTH 40

#include <termios.h>
#include <unistd.h>
#include "regs.h"

int main () {

    int f = open ("/dev/altera_dma", O_RDWR);
    FILE *out_f;
    if (f == -1) {
        printf ("Couldn't open the device.\n");
        return 0;
    } else {
        printf ("Opened the device: file handle #%lu!\n", (long unsigned int)f);
    }
	struct dma_cmd cmd_write;
	uint32_t disable_log = 0x0;
	// enable logging
	cmd_write.cmd = 14;
	cmd_write.usr_buf_size = 5;
	cmd_write.buf = (char*)malloc(5);
	cmd_write.buf[0] = DATA_OUT_1;
	cmd_write.buf[1] = disable_log & 0xFF;
	cmd_write.buf[2] = disable_log >> 8 & 0xFF;
	cmd_write.buf[3] = disable_log >> 16 & 0xFF;
	cmd_write.buf[4] = disable_log >> 24 & 0xFF;
	write(f, &cmd_write, 0);

    close(f);

    return 0;
}



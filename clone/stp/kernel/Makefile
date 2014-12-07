obj-m	:= stp.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

CPPFLAGS += -include $(KERNELDIR)/include/generated/autoconf.h

all: default user stop_log

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD)

user: user.c
	gcc -o $@ $^

stop_log: stop_log.c
	gcc -o $@ $^

install:
	./stp_load.sh

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.symvers *.order user


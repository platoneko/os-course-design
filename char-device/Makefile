obj-m += my_device.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) modules

clean:
	rm -f *.cmd *.o *.mod *.ko

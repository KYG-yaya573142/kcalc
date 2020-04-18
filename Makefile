KDIR=/lib/modules/$(shell uname -r)/build

obj-m += calc.o
obj-m += livepatch-calc.o
calc-objs += main.o expression.o
livepatch-calc-y = klp-calc.o expression.o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

GIT_HOOKS := .git/hooks/applied
TARGET_MODULE := calc

all: $(GIT_HOOKS)
	make -C $(KDIR) M=$(PWD) modules

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

load:
	sudo insmod $(TARGET_MODULE:%=%.ko)
	sudo chmod 0666 /dev/calc

unload:
	sudo rmmod $(TARGET_MODULE) || true >/dev/null

patch:
	sudo insmod livepatch-calc.ko

unpatch:
	sudo sh -c "echo 0 > /sys/kernel/livepatch/livepatch_calc/enabled"
	sleep 2
	sudo rmmod livepatch-calc

check: all
	scripts/test.sh

clean:
	make -C $(KDIR) M=$(PWD) clean

KDIR=/lib/modules/$(shell uname -r)/build

obj-m += calc.o
obj-m += livepatch-calc.o
calc-objs += main.o expression.o
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

check: all
	scripts/test.sh

clean:
	make -C $(KDIR) M=$(PWD) clean

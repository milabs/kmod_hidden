NAME		:= findme

obj-m		:= $(NAME).o

$(NAME)-y	:= module-init.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

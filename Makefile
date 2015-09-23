# Copyleft 2015 Joshua Besneatte (http://pirates.jairaja.org)
# Based on ShuttlePRO by Eric Messick (FixedImagePhoto.com/Contact)

CFLAGS=-O3 -W -Wall

INSTALL_DIR=/usr/local/bin
RULES_DIR=/etc/udev/rules.d
SERVICE_DIR=/lib/systemd/system

OBJ=\
	shuttled.o

all: shuttled

install: all
	install shuttled ${INSTALL_DIR}
	cp *.rules ${RULES_DIR}
	cp shuttled.service ${SERVICE_DIR}
	systemctl daemon-reload
	udevadm control --reload
	udevadm trigger

shuttled: ${OBJ}
	gcc ${CFLAGS} ${OBJ} -o shuttled

clean:
	rm -f shuttled $(OBJ)

shuttled.o: shuttled.h

### Projectos que iremos compilar, por ex: 
CONTIKI_PROJECT = IoT-AirNode

DEFINES+=PROJECT_CONF_H=\"project-conf.h\"

CONTIKI_TARGET_SOURCEFILES += dht22.c 
PROJECT_SOURCEFILES += slip-bridge.c

APPS += mqtt

CFLAGS += -DUIP_FALLBACK_INTERFACE=rpl_interface

all: $(CONTIKI_PROJECT) 

SMALL = 1
USE_TUNSLIP6=1
CONTIKI_WITH_IPV6 = 1

ifeq ($(PREFIX),)
 PREFIX = fd00::1/64
endif

### Localização da pasta root "contiki" em relação à pasta actual: 
TARGET = zoul
CONTIKI = ../../contiki
include $(CONTIKI)/Makefile.include

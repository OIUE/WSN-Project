CONTIKI_PROJECT = receiver sender sink

all: $(CONTIKI_PROJECT)

CONTIKI = /home/user/contiki

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
#CFLAGS += -DUIP_CONF_ND6_SEND_NS=1

#CONTIKI_SOURCEFILES += uip.c
#PROJECT_SOURCEFILES += uip.c

SMALL = 1
CONTIKI_WITH_IPV6 = 1

include $(CONTIKI)/Makefile.include

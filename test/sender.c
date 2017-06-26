#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/node-id.h"

#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-udp-packet.h"
#include "net/ipv6/uip-ds6-route.h"
#include "simple-udp.h"

#include <stdio.h>

#include "dev/leds.h"
/*---------------------------------------------------------------------------*/
#define UDP_SENDER_R_PORT 8765
#define UDP_RECEIVER_PORT 5678
#define UDP_SENDER_S_PORT 7777
#define UDP_SINK_PORT 7777
#define SEND_INTERVAL		0.5
#define RECEIVER_ID 8

typedef struct msg{
  uint16_t nodeId;
} msg_t;
/*---------------------------------------------------------------------------*/
static struct uip_udp_conn* receiver_conn;
static struct uip_udp_conn* sink_conn;
static uip_ipaddr_t receiver_ipaddr;
static struct etimer periodic;
static msg_t data;
/*---------------------------------------------------------------------------*/
PROCESS(sender_process, "sender process");
AUTOSTART_PROCESSES(&sender_process);
/*---------------------------------------------------------------------------*/
static void collectData(){
  data.nodeId = node_id;
}
/*---------------------------------------------------------------------------*/
static void sendData(){
  printf("sending data to node id %d\n", RECEIVER_ID);
  collectData();
  uip_udp_packet_sendto(receiver_conn, &data, sizeof(data),
                        &receiver_ipaddr, UIP_HTONS(UDP_RECEIVER_PORT));
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sender_process, ev, data){
  PROCESS_BEGIN();
  uip_ipaddr_t ipaddr;

  leds_on(LEDS_GREEN);
  etimer_set(&periodic, CLOCK_SECOND*5);
  PROCESS_WAIT_EVENT();

  NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER,31);
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,26);

  /* set global adress */
  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, node_id);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
  uip_ip6addr(&receiver_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, RECEIVER_ID);

  /* setup udp connection with receiver */
  receiver_conn = udp_new(NULL, UIP_HTONS(UDP_RECEIVER_PORT), NULL);
  if(receiver_conn == NULL) {
    printf("No UDP connection with receiver available, exiting the process!\n");
    PROCESS_EXIT();
  }
  udp_bind(receiver_conn, UIP_HTONS(UDP_SENDER_R_PORT));

  /* setup udp connection with sink */
  sink_conn = udp_new(NULL, UIP_HTONS(UDP_SINK_PORT), NULL);
  if(sink_conn == NULL) {
    printf("No UDP connection with sink available, exiting the process!\n");
    PROCESS_EXIT();
  }
  udp_bind(receiver_conn, UIP_HTONS(UDP_SENDER_S_PORT));


printf("tx power: %d\n",cc2420_get_txpower());

/* data periodically */
  etimer_set(&periodic, SEND_INTERVAL * CLOCK_SECOND);
  while(1) {
    leds_toggle(LEDS_RED);
    PROCESS_WAIT_EVENT();

    if(etimer_expired(&periodic)) {
      sendData();
      etimer_reset(&periodic);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

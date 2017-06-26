#include "contiki.h"
#include "sys/node-id.h"

#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-udp-packet.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ip/uip-debug.h"

#include <stdio.h>

#include "dev/leds.h"

/* handles connection and communication with the sink */
#include "sink_conn.h"
/*---------------------------------------------------------------------------*/
#define UDP_SENDER_PORT 8765      // UDP port for receiver connection
#define UDP_RECEIVER_PORT 5678    // UDP port of receiver
#define SEND_INTERVAL		0.5

//sender always hard coded to one receiver.
#define RECEIVER_ID 2
/*---------------------------------------------------------------------------*/
static struct uip_udp_conn* receiver_conn;
static uip_ipaddr_t receiver_ipaddr;
static struct etimer sendTimer;
/*---------------------------------------------------------------------------*/
PROCESS(sender_process, "sender process");
AUTOSTART_PROCESSES(&sender_process);
/*---------------------------------------------------------------------------*/
static void sendToReceiver(){
  printf("sending data to node id %d\n", RECEIVER_ID);
  uip_udp_packet_sendto(receiver_conn, &node_id, sizeof(node_id),
  &receiver_ipaddr, UIP_HTONS(UDP_RECEIVER_PORT));
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sender_process, ev, data){
  PROCESS_BEGIN();
  uip_ipaddr_t ipaddr;

  leds_on(LEDS_GREEN);

  /* set global adresses */
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
  udp_bind(receiver_conn, UIP_HTONS(UDP_SENDER_PORT));

  /* setup udp connection with sink */
  establishSinkConnection();

  /* send data periodically */
  etimer_set(&sendTimer, SEND_INTERVAL * CLOCK_SECOND);
  etimer_set(&batteryTimer, CLOCK_SECOND*3600);
  SENSORS_ACTIVATE(button_sensor);
  while(1) {
    leds_toggle(LEDS_RED);
    PROCESS_WAIT_EVENT();

    if(etimer_expired(&sendTimer)) {
      sendToReceiver();
      etimer_reset(&sendTimer);
    }

    if(etimer_expired(&batteryTimer)) {
      checkBattery(1);
      etimer_reset(&batteryTimer);
    }

    if(ev == sensors_event && data == &button_sensor) {
      checkBattery(0);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

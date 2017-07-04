#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "sys/node-id.h"

#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-udp-packet.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ip/uip-debug.h"
#include "uip-ds6-nbr.h"
#include "nbr-table.h"
#include "net/netstack.h"

#include <stdio.h>

#include "dev/leds.h"
/* handles connection and communication with the sink */
#include "sink_conn.h"
/*---------------------------------------------------------------------------*/
#define UDP_SENDER_PORT 8765      // UDP port for receiver connection
#define UDP_RECEIVER_PORT 5678    // UDP port of receiver
#define SEND_INTERVAL		0.5
#define UDP_IP_BUF    ((struct uip_udpip_hdr* ) &uip_buf[UIP_LLH_LEN])
/*---------------------------------------------------------------------------*/
static struct uip_udp_conn* receiver_conn;
static uip_ip6addr_t receiver_ipaddr;
static struct etimer sendTimer;
static uint16_t receiver_id = 0;
static int isPaired = 0;
/*---------------------------------------------------------------------------*/
/* pairing modes */
#ifndef PAIR_BY_ID                //pairing by specific receiver id
#define PAIR_BY_RSSI 1           //pairing by best connection
static radio_value_t bestRSSI = -100;
#endif
/*---------------------------------------------------------------------------*/
PROCESS(sender_process, "sender process");
AUTOSTART_PROCESSES(&sender_process);
/*---------------------------------------------------------------------------*/

/*
* send own node id to a receiver node
*/
static void sendToReceiver(){
  printf("sending data to node id %d, addr:", receiver_id);
  uip_debug_ipaddr_print(&receiver_ipaddr);
  printf("\n");
  uip_udp_packet_sendto(receiver_conn, &node_id, sizeof(node_id),
  &receiver_ipaddr,UIP_HTONS(UDP_RECEIVER_PORT));
}
/*---------------------------------------------------------------------------*/

/*
* pairs with ONE receiver based on ID or RSSI
* only called when a receiver wants to connect.
*/
static void tcpip_handler(){
  if(uip_newdata()){
    uint16_t* appdata;
    radio_value_t rssi;
    uip_ip6addr_t pairing_ipaddr;

    rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
    appdata = (uint16_t*)uip_appdata;
    printf("Got Data from node: %u ",*appdata);
    printf("RSSI = %ddBm\n",rssi);
    /* lookup local link address of potential receiver*/
    pairing_ipaddr = uip_ds6_nbr_lookup(&UDP_IP_BUF->srcipaddr)->ipaddr;
    printf("ll addr of receiver is: ");
    uip_debug_ipaddr_print(&receiver_ipaddr);
    printf("\n");
    /* ll address has to start with default prefix*/
    if(pairing_ipaddr.u8[0] == 0xfe && pairing_ipaddr.u8[1] == 0x80){
      #ifdef PAIR_BY_ID
      receiver_id = PAIR_BY_ID;
      if(*appdata == receiver_id){
        isPaired = 1;
        receiver_ipaddr = pairing_ipaddr;
        printf("paired by id with node %i\n",receiver_id);
      }
      #else
        #ifdef PAIR_BY_RSSI
        if(bestRSSI < rssi){
          isPaired = 1;
          receiver_ipaddr = pairing_ipaddr;
          bestRSSI = rssi;
          receiver_id = *appdata;
          printf("paired by rssi with node %i\n",receiver_id);
        }
        #endif
      #endif
    }else{
      printf("ll addr does not seem to be right\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sender_process, ev, data){
  PROCESS_BEGIN();
  uip_ip6addr_t ipaddr;
  leds_on(LEDS_GREEN);

  /* set global adresses */
  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, node_id);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);
  uip_ip6addr(&receiver_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, receiver_id);

  /* setup udp connection with receiver */
  receiver_conn = udp_new(NULL, UIP_HTONS(UDP_RECEIVER_PORT), NULL);
  if(receiver_conn == NULL) {
    printf("No UDP connection with receiver available, exiting the process!\n");
    PROCESS_EXIT();
  }
  udp_bind(receiver_conn, UIP_HTONS(UDP_SENDER_PORT));

  /* setup udp connection with sink */
  establishSinkConnection();

  /* wait for a receiver to initialize pairing*/
  while(!isPaired){
    PROCESS_WAIT_EVENT();
    if(ev == tcpip_event) {
      tcpip_handler();
    }
  }

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

    if(etimer_expired(&batteryTimer) ||
      (ev == sensors_event && data == &button_sensor) ) {
      checkBattery();
      etimer_set(&batteryTimer, CLOCK_SECOND*3600);
    }

    if(ev == tcpip_event){
      tcpip_handler();
    }

  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

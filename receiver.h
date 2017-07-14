#ifndef RECEIVER_H
#define RECEIVER_H


#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-udp-packet.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/netstack.h"
#include "net/ip/uip.h"
#include "net/rpl/rpl.h"
#include "sys/node-id.h"
#include "nbr-table.h"
#include "net/ip/uip-debug.h"

#include <stdio.h>
#include "dev/leds.h"

/* handles connection and communication with the sink */
#include "sink_conn.h"
/*----------------------------------------------------------------------------*/
#define UDP_SENDER_PORT	8765      // UDP port of sender
#define UDP_RECEIVER_PORT	5678    // UDP port for sender connection
#define ERROR_TOLERANCE 3
#define SAMPLE_LEN 10
#define UDP_IP_BUF    ((struct uip_udpip_hdr* ) &uip_buf[UIP_LLH_LEN])
/*----------------------------------------------------------------------------*/
static struct uip_udp_conn* sender_conn;
static int normalRSSI = 0;
static int count = 0;
static int last[SAMPLE_LEN];
static struct etimer pairTimer, timeoutTimer;
static int senderId;
static radio_value_t rssi;
/*----------------------------------------------------------------------------*/

/*
* Goes through the neighbor table and sends it's node id to every listed Ip-Addr
*/
static void pairWithSender(){
  uip_ds6_nbr_t *nbr = nbr_table_head(ds6_neighbors);
  while(nbr != NULL){
    printf("trying to pair with a sender\n");
    printf("sending to ");
    uip_debug_ipaddr_print(&nbr->ipaddr);
    printf("\n");
    uip_udp_packet_sendto(sender_conn, &node_id, sizeof(node_id),
    &nbr->ipaddr,UIP_HTONS(UDP_SENDER_PORT));
    nbr = nbr_table_next(ds6_neighbors,nbr);
  }
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static void resetNormalRSSI(){
  count = 0;
  memset(last,0,sizeof(last));
  normalRSSI = 0;
}
/*----------------------------------------------------------------------------*/
/*
*
* sampleLen cannot be greater than SAMPLE_LEN
*/
static void setNormalRSSI(int sampleLen){
  if(count < sampleLen){
    leds_on(LEDS_BLUE);
    last[count] = rssi;
    count++;
    /* calculate mean value */
  }else if(normalRSSI == 0){
    leds_off(LEDS_BLUE);
    int normal = 0;
    int i = 0;
    printf("Last %i RSSI:",SAMPLE_LEN);
    for(;i < SAMPLE_LEN; i++){
      normal = normal + last[i];
      printf(" %i ",last[i]);
    }
    printf("\n");
    normalRSSI = normal/SAMPLE_LEN;
    printf("Normal RSSI is %i\n",normalRSSI);
  }
}
/*----------------------------------------------------------------------------*/
/*
* Reads RSSI of a received packet. Calculates a mean RSSI value
* and checks whether the RSSI is below/above that.
*/
static void tcpip_handler();
/*----------------------------------------------------------------------------*/
PROCESS(receiver_process,"Receiver process");
AUTOSTART_PROCESSES(&receiver_process);
/*----------------------------------------------------------------------------*/
PROCESS_THREAD(receiver_process, ev, data){
  PROCESS_BEGIN();
  uip_ipaddr_t ipaddr;
  message.nodeId = node_id;
  leds_on(LEDS_GREEN);

  /* set global adress */
  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, node_id);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);


  /* setup new UDP connection to sender */
  sender_conn = udp_new(NULL, UIP_HTONS(UDP_SENDER_PORT), NULL);
  if(sender_conn == NULL){
    printf("UDP connection could not be established\n");
  }
  udp_bind(sender_conn, UIP_HTONS(UDP_RECEIVER_PORT));

  /* setup udp connection to sink */
  establishSinkConnection();

  etimer_set(&batteryTimer, CLOCK_SECOND*3600);
  etimer_set(&pairTimer,CLOCK_SECOND*3);
  SENSORS_ACTIVATE(button_sensor);

  /* try pairing every 3 seconds while there is no active connection wit a sender */
  while(ev != tcpip_event){
    PROCESS_WAIT_EVENT();
    if(etimer_expired(&pairTimer)){
      pairWithSender();
      etimer_reset(&pairTimer);
    }
  }

etimer_set(&timeoutTimer, CLOCK_SECOND*5);
  /* receive and process incoming packet */
  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == tcpip_event) {
      etimer_set(&timeoutTimer, CLOCK_SECOND*5);
      tcpip_handler();
    }

    if(etimer_expired(&batteryTimer)) {
      checkBattery();
      etimer_reset(&batteryTimer);
    }

    if(etimer_expired(&timeoutTimer)) {
      printf("connection timeout, trying to pair again.\n");
      etimer_set(&pairTimer,CLOCK_SECOND*3);
      while(ev != tcpip_event){
        PROCESS_WAIT_EVENT();
        if(etimer_expired(&pairTimer)){
          pairWithSender();
          etimer_reset(&pairTimer);
        }
      }
    }

    /* if button is pressed once: send battery status to sink, if twice: recalculate mean RSSI*/
    if(ev == sensors_event && data == &button_sensor) {
      etimer_set(&pairTimer,CLOCK_SECOND);
      PROCESS_WAIT_EVENT_UNTIL((ev == sensors_event && data == &button_sensor) || etimer_expired(&pairTimer));
      if(etimer_expired(&pairTimer)){
        checkBattery();
        etimer_set(&batteryTimer, CLOCK_SECOND*3600);
      }else if(ev == sensors_event && data == &button_sensor){
        printf("recalculating mean RSSI\n");
        count = 0;
        normalRSSI = 0;
        memset(last,0,sizeof(last));
      }

    }
  }

  PROCESS_END();
}

#endif

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

#include <stdio.h>
#include "dev/leds.h"

/* handles connection and communication with the sink */
#include "sink_conn.h"
/*----------------------------------------------------------------------------*/
#define UDP_SENDER_PORT	8765      // UDP port of sender
#define UDP_RECEIVER_PORT	5678    // UDP port for sender connection
#define ERROR_TOLERANCE 3
#define SAMPLE_LEN 20
#define UDP_IP_BUF    ((struct uip_udpip_hdr* ) &uip_buf[UIP_LLH_LEN])
/*----------------------------------------------------------------------------*/
static struct uip_udp_conn* sender_conn;
static int normalRSSI = 0;
static int count = 0;
static int last[SAMPLE_LEN];
static struct etimer pairTimer;
/*----------------------------------------------------------------------------*/
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
static void tcpip_handler(){
  leds_off(LEDS_GREEN);

  radio_value_t rssi;
  rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);

  uint16_t* appdata;
  if(uip_newdata()){
    appdata = (uint16_t*)uip_appdata;
    printf("Got Data from %u: ",*appdata);
    printf("RSSI = %ddBm\n",rssi);
  }

  if(count < SAMPLE_LEN){
    leds_on(LEDS_BLUE);
    last[count] = rssi;
    count++;
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
  }else{
    leds_toggle(LEDS_BLUE);
    if(rssi < normalRSSI-ERROR_TOLERANCE ){
      leds_on(LEDS_RED);
      printf("RSSI difference of %i\n",rssi - normalRSSI);
    }else{
      leds_off(LEDS_RED);
    }
  }
  leds_on(LEDS_GREEN);
}
/*----------------------------------------------------------------------------*/
PROCESS(receiver_process,"Receiver process");
AUTOSTART_PROCESSES(&receiver_process);
/*----------------------------------------------------------------------------*/
PROCESS_THREAD(receiver_process, ev, data){
  PROCESS_BEGIN();
  uip_ipaddr_t ipaddr;
  leds_on(LEDS_GREEN);

  /* set local link adress */
  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, node_id);
  //uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
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

  /* receive and process incoming packet */
  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == tcpip_event) {
      tcpip_handler();
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

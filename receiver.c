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

#include <stdio.h>

#include "dev/leds.h"

/* handles connection and communication with the sink */
#include "sink_conn.h"
/*----------------------------------------------------------------------------*/
#define UDP_SENDER_PORT	8765      // UDP port of sender
#define UDP_RECEIVER_PORT	5678    // UDP port for sender connection
#define ERROR_TOLERANCE 3
#define SAMPLE_LEN 20
/*----------------------------------------------------------------------------*/
static struct uip_udp_conn* sender_conn;
static uip_ip6addr_t ipaddr;
static int normalRSSI = 0;
static int count = 0;
static int last[SAMPLE_LEN];
/*----------------------------------------------------------------------------*/
static void tcpip_handler(){
  leds_off(LEDS_GREEN);

  radio_value_t chan;
  radio_value_t rssi;

  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &chan);
  rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);

  uint16_t* appdata;
  if(uip_newdata()){
    appdata = (uint16_t*)uip_appdata;
    printf("Got Data from %u: ",appdata);
    printf(" Channel = %u, RSSI = %ddBm\n",(unsigned int) chan ,rssi);
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
  struct uip_ds6_addr *root_if;
  leds_on(LEDS_GREEN);

  /* set global adress */
  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, node_id);
  printf("Def prefix: %d\n", UIP_DS6_DEFAULT_PREFIX);
  // uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  // uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  /* create RPL Directed Acyclic Graph */
  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);
  root_if = uip_ds6_addr_lookup(&ipaddr);
  if(root_if != NULL) {
    rpl_dag_t *dag;
    dag = rpl_set_root(RPL_DEFAULT_INSTANCE,(uip_ip6addr_t *)&ipaddr);
    uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, node_id);
    rpl_set_prefix(dag, &ipaddr, 64);
    printf("created a new RPL dag\n");
  } else {
    printf("failed to create a new RPL DAG\n");
  }

// sender :   fe80:0000:0000:0000:0212:7400:180c:4ab2
// receiver : fe80:0000:0000:0000:0212:7400:180c:4e26

  /* setup new UDP connection to sender */
  sender_conn = udp_new(NULL, UIP_HTONS(UDP_SENDER_PORT), NULL);
  if(sender_conn == NULL){
    printf("UDP connection could not be established\n");
  }
  udp_bind(sender_conn, UIP_HTONS(UDP_RECEIVER_PORT));

  /* setup udp connection to sink */
  establishSinkConnection();

  etimer_set(&batteryTimer, CLOCK_SECOND*3600);
  SENSORS_ACTIVATE(button_sensor);
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

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/uip.h"
#include "net/rpl/rpl.h"
#include "sys/node-id.h"

#include "net/netstack.h"
#include <stdio.h>

#include "dev/leds.h"
/*----------------------------------------------------------------------------*/
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UDP_SENDER_PORT	8765
#define UDP_RECEIVER_PORT	5678
#define ERROR_TOLERANCE 3
typedef struct msg{
  uint16_t nodeId;
} msg_t;
/*----------------------------------------------------------------------------*/
static struct uip_udp_conn *receiver_conn;
static uip_ip6addr_t ipaddr;
static int normalRSSI = 0;
static int count = 0;
static int lastTen[10];
/*----------------------------------------------------------------------------*/
static void tcpip_handler(){
  leds_off(LEDS_GREEN);

  radio_value_t chan;
  radio_value_t rssi;

  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &chan);
  rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);

  msg_t* appdata;
  if(uip_newdata()){
    appdata = (msg_t*)uip_appdata;
    printf("Got Data from %u: ",appdata->nodeId);
    printf("Channel = %u, RSSI = %ddBm\n",(unsigned int) chan ,rssi);
  }

  if(count < 10){
    leds_on(LEDS_BLUE);
    lastTen[count] = rssi;
    count++;
  }else if(normalRSSI == 0){
    leds_off(LEDS_BLUE);
    int normal = 0;
    int i = 0;
    printf("Last ten RSSI:");
    for(;i < 10; i++){
      normal = normal + lastTen[i];
      printf(" %i ",lastTen[i]);
    }
    printf("\n");
    normalRSSI = normal/10;
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
  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 1);

  /* create RPL Directed Acyclic Graph */
  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);
  root_if = uip_ds6_addr_lookup(&ipaddr);
  if(root_if != NULL) {
    rpl_dag_t *dag;
    dag = rpl_set_root(RPL_DEFAULT_INSTANCE,(uip_ip6addr_t *)&ipaddr);
    uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
    rpl_set_prefix(dag, &ipaddr, 64);
    printf("created a new RPL dag\n");
  } else {
    printf("failed to create a new RPL DAG\n");
  }

  /* RDC off to ensure high packet reception rates. */
  NETSTACK_RDC.off(1);

  /* setup new UDP connection */
  receiver_conn = udp_new(NULL, UIP_HTONS(UDP_SENDER_PORT), NULL);
  if(receiver_conn == NULL){
    printf("UDP connection could not be established\n");
  }
  udp_bind(receiver_conn, UIP_HTONS(UDP_RECEIVER_PORT));

  /* receive and process incoming packet */
  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();
}

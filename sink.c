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
#define UDP_SINK_PORT 7777
typedef struct msg{
  uint16_t nodeId;
  uint16_t battery;
} msg_t;
/*----------------------------------------------------------------------------*/
static struct uip_udp_conn* conn;
static uip_ip6addr_t ipaddr;
/*----------------------------------------------------------------------------*/
static void tcpip_handler(){
//  msg_t* appdata;
  if(uip_newdata()){
    // USES TOO MUCH SPACE
    // appdata = (msg_t*)uip_appdata;
    // uint16_t battery = appdata->battery;
    // uint16_t nid = appdata->nodeId;
    // float mv = (battery * 5.0) / 4096;
    //
    // printf("Battery Level of node %u is %i.%03dV ",appdata->nodeId,(int)mv,
    // (int)((mv-(int)mv)*1000));
    printf("Got message\n");
  }
  //TODO warning for user when battery is low


}
/*----------------------------------------------------------------------------*/
PROCESS(sink_process,"Sink process");
AUTOSTART_PROCESSES(&sink_process);
/*----------------------------------------------------------------------------*/
PROCESS_THREAD(sink_process, ev, data){
  PROCESS_BEGIN();
  struct uip_ds6_addr *root_if;
  leds_on(LEDS_GREEN);

  /* set global adress */
  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 77);

  /* create RPL Directed Acyclic Graph */
  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);
  root_if = uip_ds6_addr_lookup(&ipaddr);
  if(root_if != NULL) {
    rpl_dag_t *dag;
    dag = rpl_set_root(RPL_DEFAULT_INSTANCE,(uip_ip6addr_t *)&ipaddr);
    uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 77);
    rpl_set_prefix(dag, &ipaddr, 64);
    printf("created a new RPL dag\n");
  } else {
    printf("failed to create a new RPL DAG\n");
  }

  /* RDC off to ensure high packet reception rates. */
  NETSTACK_RDC.off(1);

  /* setup new UDP connection */
  conn = udp_new(NULL, UIP_HTONS(UDP_SINK_PORT), NULL);
  if(conn == NULL){
    printf("UDP connection could not be established\n");
  }
  udp_bind(conn, UIP_HTONS(UDP_SINK_PORT));

  /* receive and process incoming packet */
  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();
}

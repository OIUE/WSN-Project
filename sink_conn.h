#ifndef SINK_CONNECTION
#define SINK_CONNECTION
/*---------------------------------------------------------------------------*/
#include "dev/button-sensor.h"
#include "dev/battery-sensor.h"
/*---------------------------------------------------------------------------*/
#define UDP_SINK_PORT 7777
/*---------------------------------------------------------------------------*/
typedef struct msg{
  uint16_t nodeId;
  uint16_t battery;
} msg_t;

static struct uip_udp_conn* sink_conn;
static uip_ipaddr_t sink_ipaddr;
static struct etimer batteryTimer;
static msg_t message;
/*---------------------------------------------------------------------------*/
static void establishSinkConnection(){
  uip_ip6addr(&sink_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 77);

  /* setup udp connection with sink */
  sink_conn = udp_new(NULL, UIP_HTONS(UDP_SINK_PORT), NULL);
  if(sink_conn == NULL) {
    printf("No UDP connection with sink available!\n");
  }
  udp_bind(sink_conn, UIP_HTONS(UDP_SINK_PORT));
}
/*---------------------------------------------------------------------------*/
static void getBatteryLevel(){
  SENSORS_ACTIVATE(battery_sensor);
  uint16_t battery_val = battery_sensor.value(0);
  SENSORS_DEACTIVATE(battery_sensor);
  message.nodeId = node_id;
  message.battery = battery_val;
}
/*---------------------------------------------------------------------------*/
static void sendToSink(){
  printf("sending data to sink\n");
  uip_udp_packet_sendto(sink_conn, &message, sizeof(message),
  &sink_ipaddr, UIP_HTONS(UDP_SINK_PORT));
}
/*---------------------------------------------------------------------------*/
/*
* 2.1V is the minimum power to operate the radio, so send warning before that
* battery value of 1883.7 is equal to 2.3V
* if the user presses the button on a device, send battery data regardless
* of current value
* @param mode 0 to always send battery measurement, 1 to only send if low
*/
static void checkBattery(int mode){
  getBatteryLevel();
  printf("trying to send battery\n");
  printf("battery is %d\n", message.battery);
  if((message.battery <= 1883) || mode == 0){
    sendToSink();
  }
}
/*---------------------------------------------------------------------------*/
#endif

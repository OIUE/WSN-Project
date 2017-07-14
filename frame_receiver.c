#include "receiver.h"

/*
* Reads RSSI of a received packet. Calculates a mean RSSI value
* and checks whether the RSSI is below that.
*/
static void tcpip_handler(){

  uint16_t* appdata;
  if(uip_newdata()){
    rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
    appdata = (uint16_t*)uip_appdata;
    printf("Got Data from %u: ",*appdata);
    printf("RSSI = %ddBm\n",rssi);

    /* save first received senderId */
    if(senderId == 0){
      senderId = *appdata;
    }
    /* in case senders change: reset normal rssi*/
    if(senderId != *appdata){
      senderId = *appdata;
      resetNormalRSSI();
    }
    /* sample RSSI */
    if(normalRSSI == 0){
      setNormalRSSI(SAMPLE_LEN);
    }else{
      /* check if RSSI is lower/higher than normal*/
      leds_toggle(LEDS_BLUE);

      /* if RSSI is lower than normal */
      if(rssi < normalRSSI-ERROR_TOLERANCE ){
        leds_on(LEDS_RED);
        printf("recognized person\n");
        message.value = 0;
        sendToSink();
      }
    }
  }
}

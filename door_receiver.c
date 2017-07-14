#include "receiver.h"

static int pos_count = 0, neg_count = 0;

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
        pos_count = 0;
        if(neg_count > 9){
          printf("recognized opening of door.\n");
          message.value = 1;
          sendToSink();
          neg_count = 0;
          resetNormalRSSI();
        }else{
          neg_count++;
        }
        /* if RSSI is higher than normal */
      }else{
        leds_off(LEDS_RED);
        if(rssi > normalRSSI + ERROR_TOLERANCE){
          neg_count = 0;
          if(pos_count > 9){
            printf("recognized closing of door.\n");
            message.value = 2;
            sendToSink();
            pos_count = 0;
            resetNormalRSSI();
          }else{
            pos_count++;
          }
        /* if RSSI in normal range*/
      }else{
        if(neg_count > 0){
          printf("recognized person\n");
          message.value = 0;
          sendToSink();
          neg_count = 0;
        }
      }
    }
  }
}
}

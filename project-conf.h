#undef UIP_CONF_ROUTER
#define UIP_CONF_ROUTER                     1

#undef UIP_CONF_IPV6_RPL
#define UIP_CONF_IPV6_RPL                   1

#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC                   nullrdc_driver

#undef NETSTACK_RDC_CHANNEL_CHECKRATE
#define NETSTACK_RDC_CHANNEL_CHECKRATE      64

#undef RF_CHANNEL
#define RF_CHANNEL                          26

#undef CC2420_CONF_CHANNEL
#define CC2420_CONF_CHANNEL                 26

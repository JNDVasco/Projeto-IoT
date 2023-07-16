/*---------------------------------------------------------------------------*/
#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_
/*---------------------------------------------------------------------------*/
#define NODE_NAME "Water Test Node"
#define NODE_TYPE "water"
/*---------------------------------------------------------------------------*/
/* User configuration */
#define MQTT_CLIEND_ID            "IoTWater1"
#define MQTT_USER                 "mqttUser"
#define MQTT_PASS                 "mqttPass"
#define MQTT_BROKER_IP_ADDR       "fd00::2" // 192.168.110.1

#define MQTT_DATA_TOPIC           "iot/aquatic"
#define MQTT_NODE_STATUS_TOPIC    "iot/status"
/*---------------------------------------------------------------------------*/
/* Default configuration values */
#define DEFAULT_BROKER_PORT          1883
#define DEFAULT_PUBLISH_INTERVAL     (5 * CLOCK_SECOND)
#define DEFAULT_KEEP_ALIVE_TIMER     60

#undef IEEE802154_CONF_PANID
#define IEEE802154_CONF_PANID        0xAAAA

#undef CC2538_RF_CONF_CHANNEL
#define CC2538_RF_CONF_CHANNEL       26

#define BUFFER_SIZE                  64
#define PAYLOAD_SIZE                 512
/*---------------------------------------------------------------------------*/
#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC          nullrdc_driver

#define SICSLOWPAN_CONF_FRAG 0


/* Maximum TCP segment size for outgoing segments of our socket */
#define MAX_TCP_SEGMENT_SIZE       32

/*---------------------------------------------------------------------------*/
// Change the DHT22 Pins
#define DHT22_CONF_PORT GPIO_A_NUM
#define DHT22_CONF_PIN 5

#endif /* PROJECT_CONF_H_ */
/*---------------------------------------------------------------------------*/
/** @} */


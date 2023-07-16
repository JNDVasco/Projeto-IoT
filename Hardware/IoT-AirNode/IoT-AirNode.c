#include <stdio.h>

#include <string.h>

#include "project-conf.h"

// Contiki
#include "contiki.h"
#include "contiki-conf.h"
#include "contiki-lib.h"
#include "contiki-net.h"

// MQTT
#include "mqtt.h"
#include "dev/slip.h"
#include "rpl/rpl-private.h"
#include "net/rpl/rpl.h"
#include "net/netstack.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/ip/uip-debug.h"

// Timer's
#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "sys/timer.h"

// Zolertia imports
#include "dev/leds.h"

// ADC's
#include "dev/adc-zoul.h"
#include "dev/zoul-sensors.h"

#include "dev/dht22.h"

/*---------------------------------------------------------------------------*/
// Node info
char nodeName[BUFFER_SIZE] = NODE_NAME;
char nodeType[BUFFER_SIZE] = NODE_TYPE;
/*---------------------------------------------------------------------------*/
/*MQTT CONFIG STUFF*/
/*---------------------------------------------------------------------------*/
// Setup the MQTT configuration
char brokerIP[BUFFER_SIZE] = MQTT_BROKER_IP_ADDR;
uint16_t brokerPort = DEFAULT_BROKER_PORT;

char mqttUser[BUFFER_SIZE] = MQTT_USER;
char mqttPass[BUFFER_SIZE] = MQTT_PASS;
char clientID[BUFFER_SIZE] = MQTT_CLIEND_ID;
clock_time_t pubInterval = DEFAULT_PUBLISH_INTERVAL;

char dataTopic[BUFFER_SIZE] = MQTT_DATA_TOPIC;
char statusTopic[BUFFER_SIZE] = MQTT_NODE_STATUS_TOPIC;
/*---------------------------------------------------------------------------*/
static uip_ipaddr_t prefix;
static uint8_t prefix_set;

#define CONNECTION_STABLE_TIME (CLOCK_SECOND * 5)
/*---------------------------------------------------------------------------*/
static struct timer connection_life;
/*---------------------------------------------------------------------------*/
/* Various states */
static uint8_t state;

#define STATE_CONNECTED 0
#define STATE_DISCONNECTED 1
/*---------------------------------------------------------------------------*/
/*
 * The main MQTT buffers.
 * We will need to increase if we start publishing more data.
 */
static struct mqtt_connection conn;
/*---------------------------------------------------------------------------*/
#define movementThreshold 6000
/*-------------------------------------------------*/
PROCESS(iotAirNode, "Main - IoT AirNode");
AUTOSTART_PROCESSES(&iotAirNode);
/*-------------------------------------------------*/
static struct etimer et;
static struct ctimer ct;
/*-------------------------------------------------*/
// This timer is just a simple heartbeat so we know
// the node hasn't crashed like I wished I did into
// a wall or something during this project
static void ctimer_callback(void *ptr)
{
    leds_toggle(LEDS_RED); // Blink the LED
    printf("[INFO] - Heartbeat | State = %d\n", state);

    ctimer_restart(&ct); // Restart the timer
}
/*---------------------------------------------------------------------------*/
// Tunslip housekeeping
/*---------------------------------------------------------------------------*/
static void print_local_addresses(void)
{
    int i;
    uint8_t state;

    PRINTA("Server IPv6 addresses:\n");
    for (i = 0; i < UIP_DS6_ADDR_NB; i++)
    {
        state = uip_ds6_if.addr_list[i].state;
        if (uip_ds6_if.addr_list[i].isused &&
            (state == ADDR_TENTATIVE || state == ADDR_PREFERRED))
        {
            PRINTA(" ");
            uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
            PRINTA("\n");
        }
    }
}
/*---------------------------------------------------------------------------*/
void request_prefix(void)
{
    /* mess up uip_buf with a dirty request... */
    uip_buf[0] = '?';
    uip_buf[1] = 'P';
    uip_len = 2;
    slip_send();
    uip_clear_buf();
}
/*---------------------------------------------------------------------------*/
void set_prefix_64(uip_ipaddr_t *prefix_64)
{
    rpl_dag_t *dag;
    uip_ipaddr_t ipaddr;
    memcpy(&prefix, prefix_64, 16);
    memcpy(&ipaddr, prefix_64, 16);
    prefix_set = 1;
    uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
    uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

    dag = rpl_set_root(RPL_DEFAULT_INSTANCE, &ipaddr);
    if (dag != NULL)
    {
        rpl_set_prefix(dag, &prefix, 64);
        PRINTF("created a new RPL dag\n");
    }
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static void publishData(void)
{
    static char payload[PAYLOAD_SIZE];

    int CO;

    int temp;
    int hum;

    int movement = 0;

    if (adc_zoul.value(ZOUL_SENSORS_ADC3) > movementThreshold)
    {
        movement = (random_rand() % 4) + 1;
        leds_on(LEDS_GREEN);
    }
    else
    {
        leds_off(LEDS_GREEN);
    }

    CO = adc_zoul.value(ZOUL_SENSORS_ADC1);
    CO /= 66;
    

    if (dht22_read_all(&temp, &hum) != DHT22_ERROR)
    {
        printf("[PUB DATA] - CO = %d \n", CO);
        printf("[PUB DATA] - Movement = %d \n", movement);
        printf("[PUB DATA] - TEMP = %d.%u C\n", temp / 10, temp % 10);
        printf("[PUB DATA] - HUM = %d.%u C\n", hum / 10, hum % 10);

        sprintf(payload,
                "{\"node_type\":\"%s\",\"node_name\":\"%s\", \"data\": {\"temp\": %d.%u, \"humidity\": %d.%u, \"CO\": %d, \"movement\": %d}}",
                nodeType, nodeName, temp / 10, temp % 10, hum / 10, hum % 10, CO, movement);

        mqtt_publish(&conn, NULL, dataTopic, (uint8_t *)payload, strlen(payload), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
    }
    else
    {
        printf("[PUB DATA] - Failed to read DHT22\n");
    }
}

/*---------------------------------------------------------------------------*/
static void publishStatus(void)
{
    static char payload[PAYLOAD_SIZE];

    static uint16_t temp;
    static uint32_t batt;

    batt = vdd3_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED);
    temp = cc2538_temp_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED);

    printf("[PUB STATUS] - VDD = %u mV\n", (uint16_t)batt);
    printf("[PUB STATUS] - Temp = %d.%u C\n", temp / 1000, temp % 1000);

    sprintf(payload,
            "{\"node_type\":\"%s\",\"node_name\":\"%s\", \"data\": {\"batt\":%u, \"temp\": %d.%u}}",
            nodeType, nodeName, (uint16_t)batt, temp / 1000, temp % 1000);

    mqtt_publish(&conn, NULL, statusTopic, (uint8_t *)payload, strlen(payload), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
}
/*---------------------------------------------------------------------------*/
static void mqttEventCallback(struct mqtt_connection *m, mqtt_event_t event, void *data)
{
    switch (event)
    {
    case MQTT_EVENT_CONNECTED:
    {
        printf("[MQTT] - Application has a MQTT connection\n");
        timer_set(&connection_life, CONNECTION_STABLE_TIME);
        state = STATE_CONNECTED;
        break;
    }
    case MQTT_EVENT_DISCONNECTED:
    {
        printf("[MQTT] - MQTT Disconnect. Reason %u\n", *((mqtt_event_t *)data));

        state = STATE_DISCONNECTED;
        process_poll(&iotAirNode);
        break;
    }
    case MQTT_EVENT_PUBACK:
    {
        printf("[MQTT] - Publishing complete.\n");
        break;
    }
    default:
        printf("[MQTT] - Application got a unhandled MQTT event: %i\n", event);
        break;
    }
}
/*---------------------------------------------------------------------------*/
// Main Thread.
// Spwans the heartbeat timer
// Setup MQTT and connects to the broker
// Spawns the main loop
PROCESS_THREAD(iotAirNode, ev, data)
{
    PROCESS_BEGIN();

    prefix_set = 0;
    NETSTACK_MAC.off(0);

    PROCESS_PAUSE();

    printf("[INFO] - Starting IoT AirNode\n");

    // ------------------------------------------------------------------------ //
    state = STATE_DISCONNECTED;
    adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC_ALL);
    SENSORS_ACTIVATE(dht22);

    etimer_set(&et, CLOCK_SECOND); // Wait 1 seconds
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));


    /* Request prefix until it has been received */
    while (!prefix_set)
    {
        etimer_set(&et, CLOCK_SECOND);
        printf("[INFO] - Waiting for prefix\n");
        request_prefix();
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    }

    printf("[INFO] - Prefix set\n");

    NETSTACK_MAC.off(1);

    print_local_addresses();

    // Turn the led on and wait for the user input
    leds_on(LEDS_RED);
    printf("[INFO] - Node Ready!\n");

    ctimer_set(&ct, CLOCK_SECOND * 2, ctimer_callback, NULL);

    while (1)
    {
        // if connected to mqtt send data else reconnect
        if (state == STATE_CONNECTED)
        {
            // Publish data
            publishData();

            etimer_reset(&et);
            etimer_set(&et, 2 * CLOCK_SECOND); // Wait 1 seconds
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

            // Publish status
            publishStatus();

            etimer_reset(&et);
            etimer_set(&et, 2 * CLOCK_SECOND); // Wait 1 seconds
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        }
        else if (state == STATE_DISCONNECTED)
        {
            static uint8_t connAttempt = 0;

            printf("[INFO] - Connecting to MQTT broker\n");
            printf("[INFO] - Broker address: %s\n", brokerIP);
            mqtt_set_username_password(&conn, mqttUser, mqttPass);
            mqtt_register(&conn, &iotAirNode, clientID, mqttEventCallback, MAX_TCP_SEGMENT_SIZE);

            // Get a IP connection
            do
            {
                connAttempt++;
                printf("[INFO] - Waiting for IP address | Attempt: %d\n", connAttempt);

                etimer_set(&et, 2 * CLOCK_SECOND); // Wait 1 seconds
                PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

            } while (uip_ds6_get_global(ADDR_PREFERRED) == NULL);

            mqtt_status_t rslt = mqtt_connect(&conn, brokerIP, brokerPort, pubInterval * 3);

            printf("[INFO] - MQTT Broker connected! Result: %d \n", rslt);
            state = STATE_CONNECTED;
        }
        else
        {
            printf("[INFO] - Unknown state = %d\n", state);
            // Wait 1 seconds before turning the leds on
            etimer_set(&et, CLOCK_SECOND); // Wait 1 seconds
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        }
    }

    PROCESS_END();
}
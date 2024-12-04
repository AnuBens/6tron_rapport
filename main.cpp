#include "mbed.h"
#include <nsapi_dns.h>
#include <MQTTClientMbedOs.h>

namespace {


#define MQTT_TOPIC_TEMP         "AnuBens/feeds/temperature"
#define MQTT_TOPIC_HUM          "AnuBens/feeds/humidite"
#define MQTT_TOPIC_PRESSURE     "AnuBens/feeds/pression"
#define MQTT_TOPIC_SUBSCRIBE    "AnuBens/feeds/LED"
#define MQTT_CLIENT_ID          "6LoWPAN_Node_" "AnuBens"
#define SYNC_INTERVAL           1
}


static DigitalOut led(LED1);
static InterruptIn button(BUTTON1);


NetworkInterface *network;
MQTTClient *client;

// Configuration MQTT
const char *hostname = "io.adafruit.com";
int port = 1883;


nsapi_size_or_error_t rc = 0;

// File d'événements
static int id_yield;
static EventQueue main_queue(32 * EVENTS_EVENT_SIZE);

/*!
 *  \brief Appelé lorsqu'un message est reçu
 */
void messageArrived(MQTT::MessageData &md) {
    MQTT::Message &message = md.message;
    printf("Message received: %.*s\n", message.payloadlen, (char *)message.payload);

    // Traiter le message pour contrôler la LED
    if (strncmp((char *)message.payload, "ON", message.payloadlen) == 0) {
        led = 1;
    } else if (strncmp((char *)message.payload, "OFF", message.payloadlen) == 0) {
        led = 0;
    }
}

/*!
 *  \brief Publier des données sur Adafruit IO
 */
static int8_t publish() {
    char payload[128];
    MQTT::Message message;
    message.qos = MQTT::QOS1;
    message.retained = false;
    message.dup = false;

    // Simuler des données (remplacez par des valeurs réelles si capteurs connectés)
    float temperature = 25.5; // Exemple : température fictive
    float humidity = 60.0;    // Exemple : humidité fictive
    float pressure = 1013.0;  // Exemple : pression fictive

    // Publier la température
    snprintf(payload, sizeof(payload), "%.2f", temperature);
    message.payload = (void *)payload;
    message.payloadlen = strlen(payload);
    printf("Publishing temperature: %.2f°C to %s\n", temperature, MQTT_TOPIC_TEMP);
    rc = client->publish(MQTT_TOPIC_TEMP, message);
    if (rc != 0) {
        printf("Failed to publish temperature: %d\n", rc);
        return rc;
    }

    // Publier l'humidité
    snprintf(payload, sizeof(payload), "%.2f", humidity);
    message.payload = (void *)payload;
    message.payloadlen = strlen(payload);
    printf("Publishing humidity: %.2f%% to %s\n", humidity, MQTT_TOPIC_HUM);
    rc = client->publish(MQTT_TOPIC_HUM, message);
    if (rc != 0) {
        printf("Failed to publish humidity: %d\n", rc);
        return rc;
    }

    // Publier la pression
    snprintf(payload, sizeof(payload), "%.2f", pressure);
    message.payload = (void *)payload;
    message.payloadlen = strlen(payload);
    printf("Publishing pressure: %.2f hPa to %s\n", pressure, MQTT_TOPIC_PRESSURE);
    rc = client->publish(MQTT_TOPIC_PRESSURE, message);
    if (rc != 0) {
        printf("Failed to publish pressure: %d\n", rc);
        return rc;
    }

    return 0;
}

/*!
 *  \brief Inverser l'état de la LED
 */
void toggle_led() {
    led = !led;
    printf("LED state: %d\n", led.read());
}

int main() {
    printf("Connecting to border router...\n");

    // Configuration réseau
    network = NetworkInterface::get_default_instance();
    if (!network) {
        printf("Error! No network interface found.\n");
        return -1;
    }

    rc = network->connect();
    if (rc != 0) {
        printf("Error! net->connect() returned: %d\n", rc);
        return rc;
    }

    // Afficher l'adresse IP
    SocketAddress a;
    network->get_ip_address(&a);
    printf("IP address: %s\n", a.get_ip_address() ? a.get_ip_address() : "None");

    // Configuration MQTT
    printf("Connecting to MQTT broker...\n");
    SocketAddress address;
    network->gethostbyname(hostname, &address);
    address.set_port(port);

    TCPSocket socket;
    client = new MQTTClient(&socket);
    socket.open(network);

    rc = socket.connect(address);
    if (rc != 0) {
        printf("Connection to MQTT broker failed.\n");
        return rc;
    }

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 4;
    data.keepAliveInterval = 25;
    data.username.cstring ="AnuBens";
    data.password.cstring ="aio_bCuh71jGaf811LvRjPUahXUvFqEV";

    if (client->connect(data) != 0) {
        printf("MQTT connection failed.\n");
        return -1;
    }

    printf("Connected to MQTT broker.\n");

    // S'abonner au topic pour contrôler la LED
    if ((rc = client->subscribe(MQTT_TOPIC_SUBSCRIBE, MQTT::QOS1, messageArrived)) != 0) {
        printf("Failed to subscribe: %d\n", rc);
        return rc;
    }
    printf("Subscribed to Topic: %s\n", MQTT_TOPIC_SUBSCRIBE);

    // Tâches périodiques
    id_yield = main_queue.call_every(SYNC_INTERVAL * 1000, []() { client->yield(100); });
    button.fall(main_queue.event(publish));
    button.fall(main_queue.event(toggle_led));

    // Boucle principale
    main_queue.dispatch_forever();
}

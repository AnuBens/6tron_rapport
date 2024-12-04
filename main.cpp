
#include "mbed.h"
#include <nsapi_dns.h>
#include <MQTTClientMbedOs.h>
#include <bme280/bme280.h>


namespace {
#define MQTT_TOPIC_TEMP         "AnuBens/feeds/temperature"
#define MQTT_TOPIC_HUM          "AnuBens/feeds/humidite"
#define MQTT_TOPIC_PRESSURE     "AnuBens/feeds/pression"
#define MQTT_TOPIC_SUBSCRIBE    "AnuBens/feeds/LED"
#define MQTT_CLIENT_ID          "6LoWPAN_Node_" "AnuBens"
#define SYNC_INTERVAL           1
}

// Peripherals
static DigitalOut led(LED1);
static InterruptIn button(BUTTON1);

using namespace sixtron;
 
 
namespace
{
    constexpr auto SDA_PIN = I2C1_SDA;
    constexpr auto SCL_PIN = I2C1_SCL;
    constexpr auto I2C_ADDRESS = BME280::I2CAddress::Address1;
}
I2C i2c(SDA_PIN, SCL_PIN);
BME280 sensor(&i2c, I2C_ADDRESS);
 
 
volatile float temperature = 0.0f;

volatile float humidite = 0.0f;

volatile float pression = 0.0f;
 

// Network
NetworkInterface *network;
MQTTClient *client;

// MQTT
const char* hostname = "io.adafruit.com";
int port = 1883;

// Ticker
Ticker publish_ticker;

// Constant
constexpr float TEMP_THRESHOLD = 26.7; // Limite de température élevée

// Ticker pour led alerte
Ticker alert_ticker;

// Flag pour alerte
volatile bool is_alert_active = false;


// Error code
nsapi_size_or_error_t rc = 0;

//fonction clignoter led
void blink_led() {
    led = !led; // Inverse l'état de la LED
}

// Event queue
static int id_yield;
static EventQueue main_queue(32 * EVENTS_EVENT_SIZE);

void update_sensor_data() {
   

    // Lire les données depuis le capteur
    temperature = sensor.temperature();
    humidite = sensor.humidity();
    pression = sensor.pressure() / 100.0; // Conversion en hPa

    printf("Température : %.2f °C, Humidité : %.2f %%, Pression : %.2f hPa\n",
           temperature, humidite, pression);
}

/*!
 *  \brief Called when a message is received
 *
 *  Print messages received on mqtt topic
 */
void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    printf("Payload %.*s\r\n", message.payloadlen, (char*)message.payload);

    // Get the payload string
    char* char_payload = (char*)malloc((message.payloadlen+1)*sizeof(char)); // allocate the necessary size for our buffer
    char_payload = (char *) message.payload; // get the arrived payload in our buffer
    char_payload[message.payloadlen] = '\0'; // String must be null terminated

    // Compare our payload with known command strings
    if (strcmp(char_payload, "ON") == 0) {
        led = 1;
    }
    else if (strcmp(char_payload, "OFF") == 0) {
        led = 0;
    }
    else if (strcmp(char_payload, "RESET") == 0) {
        printf("RESETTING ...\n");
        system_reset();
    }
}

/*!
 *  \brief Yield to the MQTT client
 *
 *  On error, stop publishing and yielding
 */
static void yield(){
    // printf("Yield\n");
    
    rc = client->yield(100);

    if (rc != 0){
        printf("Yield error: %d\n", rc);
        main_queue.cancel(id_yield);
        main_queue.break_dispatch();
        system_reset();
    }
}

/*!
 *  \brief Publish data over the corresponding adafruit MQTT topic
 *
 */
static int8_t publish() {

   char payload[128]; // Déclaration du buffer
    MQTT::Message message;
    message.qos = MQTT::QOS1;
    message.retained = false;
    message.dup = false;

    update_sensor_data();
     // Vérification de l'alerte
    if (temperature > TEMP_THRESHOLD) {
        if (!is_alert_active) {
            printf("Alerte : Température élevée détectée (%.2f°C)\n", temperature);
            is_alert_active = true;

            // Activer le clignotement de la LED
            alert_ticker.attach(&blink_led, std::chrono::milliseconds(500));

            // Publier une alerte sur Adafruit IO
            char alert_payload[128];
            snprintf(alert_payload, sizeof(alert_payload), "ALERTE : Température élevée (%.2f°C)", temperature);
            MQTT::Message alert_message;
            alert_message.qos = MQTT::QOS1;
            alert_message.retained = false;
            alert_message.dup = false;
            alert_message.payload = (void *)alert_payload;
            alert_message.payloadlen = strlen(alert_payload);
            client->publish("AnuBens/feeds/alerte", alert_message);
        }
    } else {
        if (is_alert_active) {
            printf("Température revenue à la normale (%.2f°C)\n", temperature);
            is_alert_active = false;

            // Désactiver le clignotement de la LED
            alert_ticker.detach();
            led = 0; // Éteindre la LED
        }
    }


   // Publier la température
    snprintf(payload, sizeof(payload), "%.2f", temperature);
    message.payload = (void *)payload;
    message.payloadlen = strlen(payload);
    printf("Publishing temperature: %.2f°C to %s\n", temperature, MQTT_TOPIC_TEMP);
    rc = client->publish(MQTT_TOPIC_TEMP, message);
    if (rc != 0) {
        printf("Failed to publish temperature: %d\n", rc);
    } else {
        printf("Temperature published successfully.\n");
    }


    // Publier l'humidité
    snprintf(payload, sizeof(payload), "%.2f", humidite);
    message.payload = (void *)payload;
    message.payloadlen = strlen(payload);
    printf("Publishing humidity: %.2f%% to %s\n", humidite, MQTT_TOPIC_HUM);
    rc = client->publish(MQTT_TOPIC_HUM, message);
    if (rc != 0) {
        printf("Failed to publish humidity: %d\n", rc);
        return rc;
    }

    // Publier la pression
    snprintf(payload, sizeof(payload), "%.2f", pression);
    message.payload = (void *)payload;
    message.payloadlen = strlen(payload);
    printf("Publishing pressure: %.2f hPa to %s\n", pression, MQTT_TOPIC_PRESSURE);
    rc = client->publish(MQTT_TOPIC_PRESSURE, message);
    if (rc != 0) {
        printf("Failed to publish pressure: %d\n", rc);
        return rc;
    }

    return 0;

}
   void toggle_led() {
    led = !led; 
    printf("LED état : %d \n", led.read());
  
}
// main() runs in its own thread in the OS
// (note the calls to ThisThread::sleep_for below for delays)

int main()
{

    if (sensor.initialize() != true) {
        printf("Erreur d'initialisation du capteur BME280\n");
        return 1;
    }
    printf("Capteur BME280 initialisé avec succès\n");
 
    // config
    sensor.set_sampling(
    BME280::SensorMode::NORMAL,
    BME280::SensorSampling::OVERSAMPLING_X16, // température
    BME280::SensorSampling::OVERSAMPLING_X16, // pression
    BME280::SensorSampling::OVERSAMPLING_X16, // humidité
    BME280::SensorFilter::OFF,                // Pas de filtrage
    BME280::StandbyDuration::MS_0_5           // Durée de veille
);

    printf("Connecting to border router...\n");

    /* Get Network configuration */
    network = NetworkInterface::get_default_instance();

    if (!network) {
        printf("Error! No network interface found.\n");
        return 0;
    }

    /* Add DNS */
    nsapi_addr_t new_dns = {
        NSAPI_IPv6,
        { 0xfd, 0x9f, 0x59, 0x0a, 0xb1, 0x58, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0x01 }
    };
    nsapi_dns_add_server(new_dns, "LOWPAN");

    /* Border Router connection */
    rc = network->connect();
    if (rc != 0) {
        printf("Error! net->connect() returned: %d\n", rc);
        return rc;
    }

    /* Print IP address */
    SocketAddress a;
    network->get_ip_address(&a);
    printf("IP address: %s\n", a.get_ip_address() ? a.get_ip_address() : "None");

    /* Open TCP Socket */
    TCPSocket socket;
    SocketAddress address;
    network->gethostbyname(hostname, &address);
    address.set_port(port);

    /* MQTT Connection */
    client = new MQTTClient(&socket);
    socket.open(network);
    rc = socket.connect(address);
    if(rc != 0){
        printf("Connection to MQTT broker Failed\n");
        return rc;
    }

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 4;
    data.keepAliveInterval = 25;
    data.username.cstring = (char *)"AnuBens";
    data.password.cstring = (char *)"aio_bCuh71jGaf811LvRjPUahXUvFqEV";
    if (client->connect(data) != 0){
        printf("Connection to MQTT Broker Failed\n");
    }

    printf("Connected to MQTT broker\n");

    /* MQTT Subscribe */
    if ((rc = client->subscribe(MQTT_TOPIC_SUBSCRIBE, MQTT::QOS0, messageArrived)) != 0){
        printf("rc from MQTT subscribe is %d\r\n", rc);
    }
    printf("Subscribed to Topic: %s\n", MQTT_TOPIC_SUBSCRIBE);

    yield();

    // Yield every 1 second
    id_yield = main_queue.call_every(SYNC_INTERVAL * 1000, yield);

    // Publish ticker
    publish_ticker.attach(main_queue.event(publish), 10); // Appeler `publish` toutes les 10 secondes

    


    main_queue.dispatch_forever();
}
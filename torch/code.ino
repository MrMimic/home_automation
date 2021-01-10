

/*
The idea is to automate a kind of torch.
I need to get luminosity (to send an "on" command only if needed).

It is captured on A0 PIN as a float value.
Then sent to a MQTT broker through WiFi.

The broker subscribe to another chanel.
When this chan receive a message "on", the ESP module turn on the relay.

Everything will be controlled via Jeedom (Jeedom's MQTT broker will subscribe to the "luminosity"
chanel. A 6AM and every 15minutes, it will check the luminosity, and turn on the torch if needed.
Once the luminosity is high enough, it will turn it off.
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Constants
#define SLEEP_DELAY 3  // Delay between two measures in seconds
#define RELAY_GPIO 5   // GPIO where the IN relay is attached
#define SENSOR_GPIO A0 // GPIO where the sensor's analog is plugged

// WiFi
const char *WIFI_SSID = "";
const char *WIFI_PASS = "";
WiFiClient espClient;

// MQTT
const char *MQTT_SERVER = "";
const char *MQTT_USER = "";
const char *MQTT_PASS = "";
const char *MQTT_PULICATION = "test/publication";
const char *MQTT_SUBSCRIPTION = "test/subscription";
const int MQTT_PORT = 1883;
PubSubClient client(espClient);

void setup()
{
    Serial.begin(115200);
    pinMode(RELAY_GPIO, OUTPUT);
    digitalWrite(RELAY_GPIO, HIGH);
    setupWifi();
    setupMqtt();
}

void setupMqtt()
{
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);
}

void setupWifi()
{
    delay(10);
    Serial.println();
    Serial.print("Connecting to WiFi network: ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.print("WiFi connected. IP address: ");
    Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // Control the relay regarding the character received.
    if ((char)payload[0] == '1')
    {
        digitalWrite(RELAY_GPIO, LOW);
    }
    else if ((char)payload[0] == '0')
    {
        digitalWrite(RELAY_GPIO, HIGH);
    }
    else
    {
        Serial.print("Unrecognized command: ");
        Serial.println(payload[0]);
    }
}

void reconnect()
{
    while (!client.connected())
    {
        Serial.println("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect("ESP8266Client", MQTT_USER, MQTT_PASS))
        {
            Serial.println("connected");
            // Chanel subscription
            client.subscribe(MQTT_SUBSCRIPTION);
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

float getLuminosity()
{
    int sensorValue = analogRead(SENSOR_GPIO);
    return sensorValue;
}

void loop()
{
    // Reconnect the broker and subscribe to chanel
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    // Get the luminosity value
    int luminosity = getLuminosity();
    Serial.print("Sending luminosity: ");
    Serial.println(luminosity);
    char luminosityString[6];
    dtostrf(luminosity, 0, 0, luminosityString);

    // Publish the value
    client.publish(MQTT_PULICATION, luminosityString);
    delay(SLEEP_DELAY * 1000);
}
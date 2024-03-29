#include "Arduino.h"

// WATCHDOG
#include <Esp.h>

#define TIMER_INTERVAL_MS 500
// WIFI
#include <ESP8266WiFi.h>
// OTA
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

// MQTT
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define MAYOR 1
#define MINOR 0
#define PATCH 2
#define WIFI_SSID "JAVI"
#define WIFI_PASS "xavier1234"
// MQTT
#define MSG_BUFFER_SIZE (50)

volatile uint32_t lastMillis = 0;

String version = String(MAYOR) + "." + String(MINOR) + "." + String(PATCH);
bool state = 1;
// OTA
AsyncWebServer server(8080);

// MQTT
const char *mqtt_server = "192.168.1.251";
const char *TOPIC = "Tanque1/canal/temperature/sensor1";
WiFiClient espClient;
PubSubClient client(espClient);
OneWire ourWire(D3); // Se establece el pin 2  como bus OneWire

DallasTemperature sensors(&ourWire); // Se declara una variable u objeto para nuestro sensor
// Update these with values suitable for your network.
unsigned long lastMsg = 0;
char msg[MSG_BUFFER_SIZE];
int value = 0;
float array_temp;
int cnt = 0;
int sensorValue = 0; // variable to store the value coming from the sensor
float temp = 0;

void callback_mqtt(char *topic, byte *payload, unsigned int length);
void reconnect_mqtt();

void setup()
{
  // initialize LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  Serial.printf("\n");
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS); // change it to your ussid and password
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
  }
  Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        uint32_t seconds = (uint32_t)(millis() / 1000);
                    char reply[100];
                    Serial.println(seconds);
                    sprintf(reply, "%d %s  ds18b20 temperature sensor 1 %.2f °C", seconds,version.c_str(),temp);

    request->send(200, "text/plain", reply); });
  AsyncElegantOTA.begin(&server); // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");

  // SETUP APP
  sensors.begin(); // Se inicia el sensor
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback_mqtt);

  Serial.println("Delta ms = " + String(millis() - lastMillis) + " " + version);
}
void loop()
{
  if (!client.connected())
  {
    reconnect_mqtt();
  }
  else
  {
    client.loop();

    /*
      Serial.print("Temperatura= ");
      Serial.print(temp);
      Serial.println(" C");
    */
    if (millis() - lastMillis > 50)
    {
      lastMillis = millis();
      sensors.requestTemperatures();            // Se envía el comando para leer la temperatura
      array_temp += sensors.getTempCByIndex(0); // Se obtiene la temperatura en ºC
      if (cnt >= 20)
      {
        digitalWrite(LED_BUILTIN, state);
        state = !state;

        Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
        Serial.println("");
        temp = array_temp / (cnt);
        snprintf(msg, MSG_BUFFER_SIZE, "%.2f", temp);
        // Serial.print("Publish message: ");
        // Serial.println(msg);
        client.publish(TOPIC, msg);
        cnt = 0;
        array_temp = 0;
      }
      else
        cnt++;
    }
  }
}

void callback_mqtt(char *topic, byte *payload, unsigned int length)
{
  if (strcmp(TOPIC, topic) == 0)
  {
    digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED)); // Turn the LED on (Note that LOW is the volTOPICe level
  }
}

void reconnect_mqtt()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(TOPIC, "Teperature sensor");
      // ... and resubscribe
      client.subscribe(TOPIC);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
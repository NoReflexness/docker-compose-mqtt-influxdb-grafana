/**

    XXX

    Required libraries:
      - MKRNB
      - PubSubClient
**/

// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME280.h>

#include <MKRNB.h>
#include <PubSubClient.h>

// If you are not using the default PIN, consider using "arduino_secrets.h"
// See https://www.hackster.io/Arduino_Genuino/store-your-sensitive-data-safely-when-sharing-a-sketch-e7d0f0
const char PIN_NUMBER[] = "1234";
const char APN[] = "internet";

#define MQTT_TOPIC_TEMPERATURE "home/iothon/temperature"
#define MQTT_TOPIC_STATE       "home/iothon/status"
#define MQTT_PUBLISH_DELAY     60000 // 60 seconds
#define MQTT_CLIENT_ID         "mkrnb1500iothon"

const char *MQTT_SERVER   = "195.148.126.xx"; // XXX change to your server IP
const char *MQTT_USER     = "mqttuser";     // NULL for no authentication
const char *MQTT_PASSWORD = "mqttpassword"; // NULL for no authentication

NB           nbAccess(true); // NB access: include a 'true' parameter for debug enabled
GPRS         gprsAccess;     // GPRS access
NBClient     tcpSocket;
PubSubClient mqttClient(tcpSocket);

void setup() {
  Serial.begin(115200);
  while (! Serial)
      ;

  Serial.println("MKR NB 1500 MQTT client starting.");

  Serial.print("Connecting to the NB-IoT / LTE Cat M1 network...");
  while (nbAccess.begin(PIN_NUMBER, APN) != NB_READY) {
    Serial.println("failed.  Retrying in 10 seconds.");
    delay(10000);
  }
  Serial.println("connected.");

  Serial.print("Acquiring a PDP context...");
  while (gprsAccess.attachGPRS() != GPRS_READY) {
    Serial.println("failed.  Retrying in 10 seconds.");
    delay(10000);
  }
  Serial.println("acquired.");

  mqttClient.setServer(MQTT_SERVER, 1883);
}

void loop() {
  mqttConnectIfNeeded();
  mqttClient.loop();

  long lastMsgTime = 0;

  long now = millis();
  if (now - lastMsgTime > MQTT_PUBLISH_DELAY) {
    lastMsgTime = now;

    float temperature = readTemperature();
    mqttPublish(MQTT_TOPIC_TEMPERATURE, temperature);
  }
}

float readTemperature() {
  String temp;

  // Read the NB-IoT modem's internal temperature

  MODEM.send("AT+UTEMP?");
  MODEM.waitForResponse(100, &temp);

  if (temp.startsWith("+UTEMP: ")) {
    temp.remove(0, 8);
    return temp.toFloat();
  }
  return NAN;
}

void mqttConnectIfNeeded() {
  while (!mqttClient.connected()) {
    Serial.print("Connceting to the MQTT...");

    // Attempt to connect
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD, MQTT_TOPIC_STATE, 1, true, "disconnected", false)) {
      Serial.println("connected");

      // Once connected, publish an announcement...
      mqttClient.publish(MQTT_TOPIC_STATE, "connected", true);
    } else {
      Serial.print("failed, state=");
      Serial.print(mqttClient.state());
      Serial.println(". Trying again in 5 seconds.");
      delay(5000);
    }
  }
}

void mqttPublish(String topic, float payload) {
  Serial.print("Publishing at ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(payload);

  mqttClient.publish(topic.c_str(), String(payload).c_str(), true);
}

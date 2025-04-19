#ifndef _T3_MQTT_H
#define _T3_MQTT_H

//#include "variables.h"
#include "Client.h"
#include "WiFiClientSecure.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

class T3_MQTT {
  public:
  T3_MQTT(const char *server, uint16_t port, const char *cid, const char *user, const char *pass, const char* cacert);
  void setupSubscribe(uint8_t qos, const char *feedName, void (*callback)(char *data, uint16_t len));
  void setupPublish(const char *feedName);
  void loop();
  void connect();
  void publish(const char *msg);
  
  private:
  //Adafruit_MQTT_Client mqtt;
  Adafruit_MQTT_Client *mqtt;
  // Setup a feed called 'feed' for subscribing to current time
  Adafruit_MQTT_Subscribe *feed;
  // Setup a feed called 'pub' for publishing.
  Adafruit_MQTT_Publish *pub;
  // Create an ESP8266 WiFiClient class to connect to the MQTT server.
  WiFiClientSecure client;

  uint16_t pingInterval = 60000;
  uint16_t lastPing;

};

#endif
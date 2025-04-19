#include "_T3_MQTT.h"

T3_MQTT::T3_MQTT(const char *server, uint16_t port, const char *cid, const char *user, const char *pass, const char* cacert) {
  mqtt = new Adafruit_MQTT_Client(&client, server, port, cid, user, pass);
  client.setCACert(cacert);
}

void T3_MQTT::setupSubscribe(uint8_t qos, const char *feedName, void (*callback)(char *data, uint16_t len)) {
  feed = new Adafruit_MQTT_Subscribe(mqtt, feedName, qos);
  feed->setCallback(callback);
  mqtt->subscribe(feed);
}

void T3_MQTT::setupPublish(const char *feedName){
  pub = new Adafruit_MQTT_Publish(mqtt, feedName);
}

void T3_MQTT::loop() {
  connect();
  mqtt->processPackets(1);
  if(millis() - lastPing < pingInterval) return;
  lastPing = millis();

  mqtt->ping();
} 

void T3_MQTT::connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt->connected()) {
    return;
  }

  //Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt->connect()) != 0) { // connect will return 0 for connected
       //Serial.println(mqtt->connectErrorString(ret));
       //Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt->disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }

  //Serial.println("MQTT Connected!");
}

void T3_MQTT::publish(const char *msg) {
  pub->publish(msg);
}

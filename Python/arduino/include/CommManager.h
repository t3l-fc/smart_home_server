#include "_T3_MQTT.h"
#include "params.h"
#include <ArduinoJson.h>

class CommManager {
    public:
    T3_MQTT* mqtt = nullptr;

    bool setup() {
        mqtt = new T3_MQTT(AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY, adafruitio_root_ca);
        
        if (!mqtt) return false;

        mqtt->connect();
        delay(1000);
        mqtt->setupPublish(feed);

        return true;
    }

    void setupSubscribe(void (*callback)(char *data, uint16_t len)) {
        mqtt->setupSubscribe(MQTT_QOS_1, feed, callback);
    }

    void publish(String action, String device, bool state) {
        String msg = "{\"action\":\"" + action + "\",\"device\":\"" + device + "\",\"state\":" + state + "}";
        mqtt->publish(msg.c_str());
    }

};


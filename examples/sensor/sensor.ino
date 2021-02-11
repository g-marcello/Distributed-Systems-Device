#include <MoodyEsp8266.h>

const char caCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
your certificate here
-----END CERTIFICATE-----
)EOF";

const uint8_t brokerFingerprint[] = {};

MoodySensor sensor;
uint8_t c = 0;

String countService() {
    return String(++c);
}

void setup() {
    sensor.setCert(caCert, brokerFingerprint);
    sensor.registerService("count", countService);
    sensor.begin();
}

void loop() {
    sensor.loop();
}


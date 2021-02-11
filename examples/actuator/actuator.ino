#include <MoodyEsp8266.h>

const char caCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
your certificate here
-----END CERTIFICATE-----
)EOF";

const uint8_t brokerFingerprint[] = {};

MoodyActuator actuator;

void actuate(uint8_t action) {
    switch(action) {
        case 0:
            Serial.println("Exec action 0");
            break;
        case 1:
            Serial.println("Exec action 1");
            break;
        default:
            Serial.println("Not implemented");    
    }
}

void setup() {
    actuator.setCert(caCert, brokerFingerprint);
    MoodyActuator::setActuate(actuate);
    actuator.begin();
}

void loop() {
    actuator.loop();
}
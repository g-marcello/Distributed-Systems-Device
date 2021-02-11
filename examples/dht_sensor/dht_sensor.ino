/*
* This is a working example of a two-service Moody sensor node, that uses a 
* DHT11 sensor through the DHT.h APIs, to implement a temperature service and a 
* humidity service. The DHT11 sends the obtained data through the PIN0 of the ESP8266.
*/
#include <MoodyEsp8266.h>
#include <DHT.h>
#include "certs.h"

#define DATA_PIN 0
#define DHT_TYPE DHT11

MoodySensor sensor;
DHT dht(DATA_PIN, DHT_TYPE);

float temp, hum;

String temperatureService() {
    temp = dht.readTemperature();
    return String(temp);
}

String humidityService() {
    hum = dht.readHumidity();
    return String(hum);
}


void setup() {
    dht.begin();
    sensor.setCert(caCert, brokerFingerprint);
    sensor.registerService("temperature", temperatureService);
    sensor.registerService("humidity", humidityService);
    sensor.begin();
}

void loop() {
    sensor.loop();
}

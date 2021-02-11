/*
* One way to have a tidy environment is to set the certificates in a separate 
* header file like this one.
*/

#ifndef CERTS_H
#define CERTS_H


static const char caCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
... ca cert here ...
-----END CERTIFICATE-----
)EOF";


static const uint8_t brokerFingerprint[] = {};

#endif
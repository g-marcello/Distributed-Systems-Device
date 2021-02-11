#include "MoodyEsp8266.h"

connection_info MoodyNode::conninfo;

const char login_html[] PROGMEM = R"===(
<html>
    <head>
        <title>
            Moody Node - Connect to WiFi
        </title>
        <style>
            .login {
                margin: auto;
                width: 330px;
                height: auto;
                font-family: Courier New;
                border-radius: 25px;
                background-color: #ebeeff;
                text-align: left;
                padding: 1%;
                text-decoration: none;
                font-size: 15px;
                border: 1px solid #af7bc7;
            }
        </style>
        <script>
            function save_credentials() {
                let xhttp = new XMLHttpRequest();
                xhttp.onreadystatechange = function() {
                    if(this.readyState === 4 && this.status === 200){
                        alert("The esp will now reset and try to connect to the specified network.");
                    } else if(this.status === 422) {
                        alert("Insert both an ssid and a key");
                    }
                };
                xhttp.open("POST", "/connect", true);
                xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
                let ssid = document.getElementById("ssid").value;
                let key = document.getElementById("key").value;
                let broker = document.getElementById("broker").value;
                let params = "ssid="+ssid+"&key="+key+"&broker="+broker;
                xhttp.send(params);
            }
        </script>
    </head>
    <body>
        <div class="login">
            <h3>Connect to your home WiFi</h3>
            <table>
                <tr>
                    <td>SSID:</td>
                    <td><input type="text" name="ssid" id="ssid" /></td>
                </tr>
                <tr>
                    <td>KEY:</td>
                    <td><input type="password" name="key" id="key"/></td>
                </tr>
                <tr>
                    <td>Broker Address:</td>
                    <td><input type="text" name="broker" id="broker"/></td>
                </tr>
                <tr>
                    <td><input type="submit" value="Connect" onclick="save_credentials()" /></td>
                </tr>
            </table>
        </div>
    </body>
</html>
)===";

const long APRand = random(100, 500);

// Validates a post request coming from the /connect endpoint, putting the received ssid and key
// into values and returning true if the content of the request was valid.
// If false is returned, nothing can be said about the contents of values;
bool validPostConnect(AsyncWebServerRequest *request)
{
    bool p1 = request->hasParam("ssid", true);
    bool p2 = request->hasParam("key", true);
    bool p3 = request->hasParam("broker", true);
    if (!p1 || !p2 || !p3)
    {
        return false;
    }
    String ssid = request->getParam("ssid", true)->value();
    String key = request->getParam("key", true)->value();
    String broker = request->getParam("broker", true)->value();
    unsigned int l1 = ssid.length();
    unsigned int l2 = key.length();
    unsigned int l3 = broker.length();

    return l1 > 0 && l1 <= SSID_LENGTH && l2 > 0 && l2 <= KEY_LENGTH && l3 > 0 && l3 <= BROKER_ADDR_LENGTH;
}

AsyncWebServer createAPServer(int port)
{
    IPAddress IP = WiFi.softAPIP();

    AsyncWebServer server(port);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", login_html);
    });

    server.on("/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!validPostConnect(request))
        {
            request->send(422, "text/plain", "wrong syntax");
            return;
        }
        request->send(200, "text/plain", "ok");
        delay(1000);

        MoodyNode::conninfo.ok = CONN_OK;
        String ssid = request->getParam("ssid", true)->value();
        String key = request->getParam("key", true)->value();
        String broker = request->getParam("broker", true)->value();
        strcpy(MoodyNode::conninfo.SSID, ssid.c_str());
        strcpy(MoodyNode::conninfo.KEY, key.c_str());
        strcpy(MoodyNode::conninfo.BROKER_ADDR, broker.c_str());

        EEPROM.put(CONNINFO_ADDR, MoodyNode::conninfo);
        EEPROM.commit();
#if defined(ESP8266)
        ESP.reset();
#else
        ESP.restart();
#endif
    });
    return server;
}

WiFiClientSecure MoodyNode::wifiClient = WiFiClientSecure();
PubSubClient MoodyNode::client = PubSubClient(wifiClient);
AsyncWebServer MoodyNode::apServer = createAPServer(WEB_SERVER_PORT);

void MoodyNode::setCert(const char *caCert, const uint8_t *brokerFingerprint)
{
#if defined(ESP8266)
    caCertX509 = X509List(caCert);
    wifiClient.setTrustAnchors(&caCertX509);
    wifiClient.allowSelfSignedCerts();
    wifiClient.setFingerprint(brokerFingerprint);
#else
    wifiClient.setCACert(caCert);
#endif
}

void MoodyNode::activateAPMode()
{
    char randSSID[13];
    sprintf(randSSID, "%s%d\0", AP_SSID, APRand);
    // Init ESP WiFi as AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(randSSID);
    apMode = true;
    apServer.begin();
}

bool MoodyNode::connectToWifi()
{
    WiFi.mode(WIFI_STA);
    uint8_t attempt = 0;

    while (attempt < MAX_ATTEMPTS)
    {
        attempt++;
        DEBUG_MSG("Connecting to the WiFi - Attempt n.%d\n", attempt);
        WiFi.begin(conninfo.SSID, conninfo.KEY);
        if (WiFi.waitForConnectResult() == WL_CONNECTED)
        {
            return true;
        }
        delay(1000);
    }
    return false;
}

bool MoodyNode::connectToBroker()
{
    int attempt = 0;
    while (!client.connected() && attempt < MAX_ATTEMPTS)
    {
        DEBUG_MSG("Trying to connect to the broker @%s - Attempt n.%d\n", conninfo.BROKER_ADDR, ++attempt);
        String clientId = "MoodyNode-" + String(random(100, 1000));
        if (client.connect(clientId.c_str()))
        {
            return true;
        }
        DEBUG_MSG("Connection failed, rc=%d trying again in 5 seconds\n", client.state());
        delay(2000);
    }

    return false;
}

void MoodyNode::begin()
{
    EEPROM.begin(EEPROM_SIZE_ACTUATOR);
    EEPROM.get(CONNINFO_ADDR, conninfo);

#if defined(DEBUG_ESP_PORT)
    Serial.begin(115200);
#endif
    DEBUG_MSG("SSID:\t%s\nKEY:\t%s\nBroker:\t%s\n", conninfo.SSID, conninfo.KEY, conninfo.BROKER_ADDR);

    bool okWifi = connectToWifi();
    if (!okWifi)
    {
        activateAPMode();
        return;
    }
    client.setServer(conninfo.BROKER_ADDR, MQTT_PORT);
    bool okMqtt = connectToBroker();
    if (!okMqtt)
    {
        activateAPMode();
        return;
    }
    lastSetup();
}

void MoodyNode::loop()
{
    if (!apMode)
    {
        bool wifiConn = WiFi.isConnected();
        if (!wifiConn)
        {
            bool okWifi = connectToWifi();
            if (!okWifi)
            {
                activateAPMode();
                return;
            }
        }

        bool mqttConn = client.connected();
        if (!mqttConn)
        {
            client.setServer(conninfo.BROKER_ADDR, MQTT_PORT);
            bool okMqtt = connectToBroker();
            if (!okMqtt)
            {
                DEBUG_MSG("The broker address may be wrong");
                activateAPMode();
                return;
            }
        }

        lastLoop();
        client.loop();
    }
}

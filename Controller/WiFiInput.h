#include <SPI.h>
//#include <WiFiNINA.h>
#include <WiFiWebServer.h>

#include "arduino_secrets.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int keyIndex = 0; // your network key index number (needed only for WEP)

int status = WL_IDLE_STATUS;
WiFiWebServer server(80);

class WiFiInput {
private:
  Throttle &throttle;
  Steering &steering;

  unsigned long lastCommandTime = 0;

public:
  WiFiInput(Throttle &_throttle, Steering &_steering) : throttle(_throttle), steering(_steering) {}

  void setup() {
    // check for the WiFi module:
    if (WiFi.status() == WL_NO_MODULE) {
      Serial.println("Communication with WiFi module failed!");
      // don't continue
      while (true);
    }

    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
      Serial.println("Please upgrade the firmware");
    }

    // attempt to connect to WiFi network:
    while (status != WL_CONNECTED) {
      Serial.print("Attempting to connect to Network named: ");
      Serial.println(ssid);

      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
      status = WiFi.begin(ssid, pass);
      // wait 10 seconds for connection:
      delay(10000);
    }

    server.on("/", [&]() {
      if (server.hasArg("speed") && server.hasArg("steering")) {
        throttle.setSpeed(server.arg("speed").toInt());
        steering.setTargetPos(server.arg("steering").toInt());
        lastCommandTime = millis();
      }

      int buffer_size = 512;
      char temp[buffer_size];
      snprintf(
        temp, buffer_size - 1,
        "{ \"speed\": %d, \"steering\": { \"curPos\": %d, \"targetPos\": %d } }",
        throttle.getSpeed(), steering.getCurrentPos(), steering.getTargetPos()
      );

      server.send(200, F("text/plain"), temp);
    });

    server.begin();
    printWifiStatus();
  }

  unsigned long handleInput() {
    server.handleClient();

    return lastCommandTime;
  }

  void printWifiStatus() {
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");

    Serial.print("To see this page in action, open a browser to http://");
    Serial.println(ip);
  }
};

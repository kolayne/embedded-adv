// ESP Telnet (https://github.com/LennartHennigs/ESPTelnet)
#include <ESPTelnet.h>
#include <ESPTelnetStream.h>

// Definitions of configurable values
#include "config.hpp"


// ===========
// | GLOBALS |
// ===========
ESPTelnetStream telnet;


void die(String error) {
  Serial.println(error);
  Serial.println("Rebooting now...");
  delay(2000);
  ESP.restart();
  delay(2000);
}

bool isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void onTelnetConnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" connected");

  telnet.println("\nWelcome " + telnet.getIP());
  telnet.println("(Use ^] + q  to disconnect.)");
}

void onTelnetDisconnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" disconnected");
}

void onTelnetReconnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" reconnected");
}

void onTelnetConnectionAttempt(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" tried to connected");
}

void setupTelnet() {
  // passing on functions for various telnet events
  telnet.onConnect(onTelnetConnect);
  telnet.onConnectionAttempt(onTelnetConnectionAttempt);
  telnet.onReconnect(onTelnetReconnect);
  telnet.onDisconnect(onTelnetDisconnect);

  Serial.print("- Telnet: ");
  if (telnet.begin(TELNET_PORT)) {
    Serial.println("running");
  } else {
    die("Failed to begin telnet");
  }
}

bool connectToWiFi(const char* ssid, const char* password, int max_tries = 20, int pause = 500) {
  int i = 0;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

#ifdef ARDUINO_ARCH_ESP8266
  WiFi.forceSleepWake();
  delay(200);
#endif
  WiFi.begin(ssid, password);
  do {
    delay(pause);
    Serial.print(".");
    i++;
  } while (!isConnected() && i < max_tries);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  Serial.print("My ip is: ");
  Serial.println(WiFi.localIP());

  return isConnected();
}

/* ------------------------------------------------- */

void setup() {
  Serial.begin(115200);
  Serial.println("ESP Telnet Test");

  connectToWiFi(WIFI_SSID, WIFI_PASSWORD);

  if (isConnected()) {
    auto ip = WiFi.localIP();
    Serial.println();
    Serial.print("Let's go: ");
    Serial.print(ip);
    Serial.print(":");
    Serial.println(TELNET_PORT);
    setupTelnet();
  } else {
    die("Error connecting to WiFi");
  }
}

void loop() {
  telnet.loop();
  if(Serial.available() > 0) {
    telnet.write(Serial.read());
  }
  if (telnet.available() > 0) {
    Serial.print((char)telnet.read());
  }
}

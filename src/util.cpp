#include "WiFiS3.h"
#include "util.h"
#include <Arduino.h>

void print_mac_addr(uint8_t mac[]) {
  for (int i = 0; i < 6; i++) {
    if (i > 0) {
      Serial.print(":");
    }
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
  }
  Serial.println();
}

void print_network_info() {
    // print the SSID of the network you're attached to:
    Serial.println("------------ Network info ------------");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
       // print the MAC address of the router you're attached to:
    uint8_t bssid[6];
    WiFi.BSSID(bssid);
    Serial.print("BSSID: ");
    print_mac_addr(bssid);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("Signal strength (RSSI): ");
    Serial.println(rssi);

    // print ip address
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP().toString());

    // print the encryption type:
    uint8_t encryption = WiFi.encryptionType();
    Serial.print("Encryption Type: ");
    Serial.println(encryption, HEX);
    Serial.println("--------------------------------------");
}

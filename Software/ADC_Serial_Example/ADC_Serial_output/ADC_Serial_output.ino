#include <WiFi.h>

void setup() {
  Serial.begin(115200);

  Serial.println();
  Serial.println("Starting WiFi Scan...");

  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();

  delay(1000);

  int networks = WiFi.scanNetworks();

  Serial.printf("Found %d networks\n\n", networks);

  for (int i = 0; i < networks; i++) {
    Serial.printf("%2d: ", i + 1);
    Serial.print(WiFi.SSID(i));

    Serial.print(" | RSSI: ");
    Serial.print(WiFi.RSSI(i));

    Serial.print(" dBm | CH: ");
    Serial.print(WiFi.channel(i));

    Serial.print(" | ");

    switch (WiFi.encryptionType(i)) {
      case WIFI_AUTH_OPEN:
        Serial.println("OPEN");
        break;
      case WIFI_AUTH_WPA_PSK:
        Serial.println("WPA");
        break;
      case WIFI_AUTH_WPA2_PSK:
        Serial.println("WPA2");
        break;
      case WIFI_AUTH_WPA_WPA2_PSK:
        Serial.println("WPA/WPA2");
        break;
      case WIFI_AUTH_WPA3_PSK:
        Serial.println("WPA3");
        break;
      default:
        Serial.println("OTHER");
    }
  }
}

void loop() {
}
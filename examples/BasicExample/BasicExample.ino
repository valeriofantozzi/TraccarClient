#include <WiFi.h>
#include <TraccarClient.h>

// Replace with your credentials
const char* ssid = "YOUR_SSID";
const char* pass = "YOUR_PASS";

// Configure your server and device id
TraccarClient client("http://your.traccar.server", 5055, "device001");

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println("\nWiFi connected");

  // Optional configuration
  client.setDebug(true);
  client.setBasePath("/");
  client.setTimeoutMs(4000);

  // Prepare a minimal position
  TraccarPosition p;
  p.latitude = 41.9028;   // Rome
  p.longitude = 12.4964;
  p.speedKmh = 50.0;
  p.validFlag = 1;        // 1 valid fix, 0 invalid, -1 omit

  int httpCode = 0;
  bool ok = client.sendOsmAnd(p, &httpCode);
  Serial.printf("OsmAnd sent: %s (HTTP %d)\n", ok ? "OK" : "FAIL", httpCode);
}

void loop() {
  // No-op
}

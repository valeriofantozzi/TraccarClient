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

  // Prepare a JSON body with optional fields
  TraccarPosition p;
  p.latitude = 41.9028;    // Rome
  p.longitude = 12.4964;
  p.altitudeMeters = 21.5;
  p.speedKmh = 36.0;
  p.headingDeg = 180.0;
  p.hdop = 0.9;
  p.accuracyMeters = 5.0;
  p.timestampMs = 0;       // 0 lets server set timestamp
  p.validFlag = 1;         // 1 valid, 0 invalid, -1 omit
  p.batteryPercent = 85;
  p.charging = true;
  p.eventName = "startup";
  p.activityType = "in_vehicle";
  p.odometer = 1234.5;     // meters

  int httpCode = 0;
  bool ok = client.sendJson(p, &httpCode);
  Serial.printf("JSON sent: %s (HTTP %d)\n", ok ? "OK" : "FAIL", httpCode);
}

void loop() {
  // No-op
}

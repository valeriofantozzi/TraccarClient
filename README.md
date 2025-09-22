## TraccarClient for Arduino/ESP32 🚀

Send GPS data to your Traccar server the easy way! 💨

- Supports OsmAnd (GET) and JSON (POST) protocols 🌐
- Works on Arduino/ESP32 and similar platforms ⚙️
- Simple API with smart optional fields ✅

---

### Installation 📦

- Arduino Library Manager: search for "TraccarClient" and install (after publication)
- Manual: copy this folder to `Documents/Arduino/libraries/TraccarClient`

---

### How it works ⚡️

The library provides a `TraccarClient` class and a `TraccarPosition` struct.

- `TraccarClient` manages host, port, deviceId, data sending and debug
- `TraccarPosition` describes the GPS fix and extra data (all optional)

Main methods send data in three formats:

- `sendOsmAnd(...)` sends OsmAnd-formatted query via GET
- `sendJson(...)` sends JSON via POST
- `sendOsmAndForm(...)` sends `application/x-www-form-urlencoded` via POST

All methods return `true` when the HTTP code is 200. You can read the actual HTTP code by passing an optional `int*` pointer.

---

### Quick Start ✨

1) Connect to WiFi (ESP32/ESP8266 or Arduino with networking)
2) Configure the client and deviceId
3) Fill a `TraccarPosition` and send

OsmAnd GET example:

```cpp
#include <WiFi.h>
#include <TraccarClient.h>

const char* ssid = "YOUR_SSID";
const char* pass = "YOUR_PASS";

TraccarClient client("http://your.traccar.server", 5055, "device001");

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print('.'); }

  client.setDebug(true);            // log to Serial
  client.setBasePath("/");          // optional, default "/"
  client.setTimeoutMs(4000);        // optional

  TraccarPosition p;
  p.latitude = 41.9028;             // Rome 🇮🇹
  p.longitude = 12.4964;
  p.speedKmh = 50.0;                // km/h (converted to knots by protocol)
  p.validFlag = 1;                  // 1 valid fix, 0 invalid, -1 omit

  int httpCode = 0;
  bool ok = client.sendOsmAnd(p, &httpCode);
  Serial.printf("OsmAnd sent: %s (HTTP %d)\n", ok ? "OK" : "FAIL", httpCode);
}

void loop() {}
```

JSON POST example:

```cpp
TraccarPosition p;
p.latitude = 41.9028;
p.longitude = 12.4964;
p.timestampMs = 0;       // 0 = let the server decide, or set epoch ms

int httpCode = 0;
bool ok = client.sendJson(p, &httpCode);
```

Form POST (x-www-form-urlencoded) example:

```cpp
TraccarPosition p;
p.latitude = 41.9028;
p.longitude = 12.4964;

bool ok = client.sendOsmAndForm(p);
```

You can also build the OsmAnd URL without sending (useful for debug):

```cpp
String url = client.buildOsmAndUrl(p);
Serial.println(url);
```

---

### Quick API reference 🔍

- `TraccarClient(const String& hostUrl, uint16_t port, const String& deviceId)`
- `void setHost(const String& hostUrl)`
- `void setPort(uint16_t port)`
- `void setDeviceId(const String& deviceId)`
- `void setBasePath(const String& basePath)`  // default "/"
- `void setDebug(bool enabled)`                // log to Serial
- `void setTimeoutMs(uint16_t ms)`            // HTTP connect timeout
- `bool sendOsmAnd(const TraccarPosition& pos, int* outHttpCode = nullptr) const`
- `bool sendJson(const TraccarPosition& pos, int* outHttpCode = nullptr) const`
- `bool sendOsmAndForm(const TraccarPosition& pos, int* outHttpCode = nullptr) const`
- `String buildOsmAndUrl(const TraccarPosition& pos) const`

---

### TraccarPosition fields 🧭

- `latitude` (double, degrees) – use `NAN` to omit
- `longitude` (double, degrees) – use `NAN` to omit
- `altitudeMeters` (double, meters) – `NAN` to omit
- `speedKmh` (double, km/h) – `NAN` to omit
- `headingDeg` (double, 0..360) – `NAN` to omit
- `hdop` (double) – `NAN` to omit
- `accuracyMeters` (double, meters) – `NAN` to omit
- `timestampMs` (uint64_t, epoch ms) – `0` lets the server set it
- `batteryPercent` (int, 0..100) – `-1` to omit
- `validFlag` (int, -1/0/1) – `-1` to omit
- `charging` (bool) – used only if `batteryPercent >= 0`
- `driverUniqueId` (String) – optional
- `cell` (String) – format "mcc,mnc,lac,cellId[,signalStrength]"
- `wifi` (String) – one or more entries like "mac,-70" separated by `;`
- `eventName` (String) – e.g. "motionchange"
- `activityType` (String) – e.g. "still","walking","in_vehicle"
- `odometer` (double, meters) – `NAN` to omit

Important notes:

- Speed is converted to knots for OsmAnd; by default standard rounding is used. Define `TRACCAR_SPEED_ROUND_DOWN=1` to always round down.
- `deviceId` is required for all formats.
- `basePath` is usually `/` (use it if your server expects a path).

---

### Results and debug 🧪

- Send methods return `true` when the HTTP code is `200`
- Pass `int* outHttpCode` to read the exact HTTP response code
- With `setDebug(true)` the library prints URLs/bodies and codes to `Serial`

---

### Compatibility 🧰

- `architectures=*` (Arduino/ESP32/ESP8266, etc.)
- Requires network connectivity and `HTTPClient` on supported platforms

---

### Author 👨‍💻

Valerio Fantozzi — iamvaleriofantozzi@gmail.com

If you like this library, please leave a ⭐️ on the repository!
## TraccarClient for Arduino/ESP32 🚀

Invia i dati GPS al tuo server Traccar in modo semplice e affidabile! 💨

- Supporta protocollo OsmAnd (GET) e JSON (POST) 🌐
- Funziona su Arduino/ESP32 e affini ⚙️
- API facile da usare, con campi opzionali intelligenti ✅

---

### Installazione 📦

- Arduino Library Manager: cerca "TraccarClient" e installa (dopo pubblicazione)
- Manuale: copia questa cartella in `Documents/Arduino/libraries/TraccarClient`

---

### Come funziona ⚡️

La libreria fornisce una classe `TraccarClient` e una struct `TraccarPosition`.

- `TraccarClient` gestisce host, porta, deviceId, invio dati e debug
- `TraccarPosition` descrive la posizione GPS e dati extra (tutti facoltativi)

I metodi principali inviano i dati in tre formati:

- `sendOsmAnd(...)` invia query in formato OsmAnd via GET
- `sendJson(...)` invia JSON via POST
- `sendOsmAndForm(...)` invia `application/x-www-form-urlencoded` via POST

Tutti i metodi ritornano `true` se l'HTTP code è 200. Puoi ottenere l'HTTP code reale passando un puntatore `int*` opzionale.

---

### Quick Start ✨

1) Connetti il WiFi (ESP32/ESP8266 o Arduino con networking)
2) Configura il client e il deviceId
3) Popola `TraccarPosition` e invia

Esempio (OsmAnd GET):

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

  client.setDebug(true);            // log su Serial
  client.setBasePath("/");          // opzionale, default "/"
  client.setTimeoutMs(4000);        // opzionale

  TraccarPosition p;
  p.latitude = 41.9028;             // Roma 🇮🇹
  p.longitude = 12.4964;
  p.speedKmh = 50.0;                // km/h (convertito in nodi lato protocollo)
  p.validFlag = 1;                  // 1 = fix valido, 0 = non valido, -1 = ometti

  int httpCode = 0;
  bool ok = client.sendOsmAnd(p, &httpCode);
  Serial.printf("OsmAnd sent: %s (HTTP %d)\n", ok ? "OK" : "FAIL", httpCode);
}

void loop() {}
```

Esempio (JSON POST):

```cpp
TraccarPosition p;
p.latitude = 41.9028;
p.longitude = 12.4964;
p.timestampMs = 0;       // 0 = lascia al server, oppure epoch ms

int httpCode = 0;
bool ok = client.sendJson(p, &httpCode);
```

Esempio (Form POST x-www-form-urlencoded):

```cpp
TraccarPosition p;
p.latitude = 41.9028;
p.longitude = 12.4964;

bool ok = client.sendOsmAndForm(p);
```

Puoi anche costruire l’URL OsmAnd senza inviare (utile per debug):

```cpp
String url = client.buildOsmAndUrl(p);
Serial.println(url);
```

---

### Riferimento rapido API 🔍

- `TraccarClient(const String& hostUrl, uint16_t port, const String& deviceId)`
- `void setHost(const String& hostUrl)`
- `void setPort(uint16_t port)`
- `void setDeviceId(const String& deviceId)`
- `void setBasePath(const String& basePath)`  // default "/"
- `void setDebug(bool enabled)`                // log su Serial
- `void setTimeoutMs(uint16_t ms)`            // timeout connessione HTTP
- `bool sendOsmAnd(const TraccarPosition& pos, int* outHttpCode = nullptr) const`
- `bool sendJson(const TraccarPosition& pos, int* outHttpCode = nullptr) const`
- `bool sendOsmAndForm(const TraccarPosition& pos, int* outHttpCode = nullptr) const`
- `String buildOsmAndUrl(const TraccarPosition& pos) const`

---

### Campi di TraccarPosition 🧭

- `latitude` (double, gradi) – usa `NAN` per omettere
- `longitude` (double, gradi) – usa `NAN` per omettere
- `altitudeMeters` (double, metri) – `NAN` per omettere
- `speedKmh` (double, km/h) – `NAN` per omettere
- `headingDeg` (double, 0..360) – `NAN` per omettere
- `hdop` (double) – `NAN` per omettere
- `accuracyMeters` (double, metri) – `NAN` per omettere
- `timestampMs` (uint64_t, epoch ms) – `0` per far decidere al server
- `batteryPercent` (int, 0..100) – `-1` per omettere
- `validFlag` (int, -1/0/1) – `-1` per omettere
- `charging` (bool) – usato solo se `batteryPercent >= 0`
- `driverUniqueId` (String) – opzionale
- `cell` (String) – formato "mcc,mnc,lac,cellId[,signalStrength]"
- `wifi` (String) – uno o più "mac,-70" separati da `;`
- `eventName` (String) – es. "motionchange"
- `activityType` (String) – es. "still","walking","in_vehicle"
- `odometer` (double, metri) – `NAN` per omettere

Note importanti:

- La velocità è convertita in nodi per OsmAnd; per default arrotondamento standard. Definisci `TRACCAR_SPEED_ROUND_DOWN=1` per arrotondare per difetto.
- `deviceId` è obbligatorio per tutti i formati.
- `basePath` di solito è `/` (path del device/protocollo sul server se necessario).

---

### Risultati e debug 🧪

- I metodi di invio ritornano `true` se l’HTTP code è `200`
- Passa `int* outHttpCode` per leggere il codice di risposta preciso
- Con `setDebug(true)` vengono stampati URL/corpi e codici su `Serial`

---

### Compatibilità 🧰

- `architectures=*` (Arduino/ESP32/ESP8266, ecc.)
- Richiede una connessione di rete disponibile e `HTTPClient` nelle piattaforme supportate

---

### Autore 👨‍💻

Valerio Fantozzi — iamvaleriofantozzi@gmail.com

Se questa libreria ti piace, lascia una ⭐️ nel repository!



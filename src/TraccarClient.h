#ifndef TRACCAR_CLIENT_H
#define TRACCAR_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Speed rounding configuration for Traccar
#ifndef TRACCAR_SPEED_ROUND_DOWN
#define TRACCAR_SPEED_ROUND_DOWN 0  // 1 = floor (round down), 0 = normal rounding
#endif

#ifdef __cplusplus
extern "C" {
#endif

// C-friendly position structure
typedef struct traccar_position_s {
  double latitude;       // degrees; NAN to omit
  double longitude;      // degrees; NAN to omit
  double altitudeMeters; // meters; NAN to omit
  double speedKmh;       // km/h; NAN to omit
  double headingDeg;     // 0..360; NAN to omit
  double hdop;           // NAN to omit
  double accuracyMeters; // meters; NAN to omit
  uint64_t timestampMs;  // 0 to omit (server time)
  int32_t batteryPercent; // 0..100, -1 to omit
  int32_t validFlag;     // -1 omit, 0 false, 1 true
  bool charging;         // used only if batteryPercent>=0
  // Optional OsmAnd/Traccar extras
  const char* driverUniqueId; // optional; nullptr to omit
  const char* cell;      // optional; "mcc,mnc,lac,cellId[,signalStrength]"
  const char* wifi;      // optional; "mac,-70" or multiple separated by ';'
  const char* eventName; // optional; e.g. "motionchange"
  const char* activityType; // optional; e.g. "still","walking","in_vehicle"
  double odometer;       // meters; NAN to omit
} traccar_position_t;

// Opaque client handle
typedef struct traccar_client_s traccar_client_t;

// Lifecycle
traccar_client_t* traccar_create(const char* host_url, uint16_t port, const char* device_id);
void traccar_destroy(traccar_client_t* client);

// Configuration
void traccar_set_base_path(traccar_client_t* client, const char* base_path);
void traccar_set_debug(traccar_client_t* client, bool enabled);
void traccar_set_timeout_ms(traccar_client_t* client, uint16_t timeout_ms);

// Operations
bool traccar_send_osmand(traccar_client_t* client, const traccar_position_t* pos, int* out_http_code);
bool traccar_send_json(traccar_client_t* client, const traccar_position_t* pos, int* out_http_code);
bool traccar_send_osmand_form(traccar_client_t* client, const traccar_position_t* pos, int* out_http_code);

// Utility: build OsmAnd URL into provided buffer (returns length written, not including NUL)
size_t traccar_build_osmand_url(traccar_client_t* client, const traccar_position_t* pos, char* out, size_t out_size);
size_t traccar_build_osmand_form_body(traccar_client_t* client, const traccar_position_t* pos, char* out, size_t out_size);

#ifdef __cplusplus
} // extern "C"
#endif

// Optional C++ interface for Arduino sketches
#ifdef __cplusplus
#ifdef ARDUINO
#include <Arduino.h>
#include <time.h>

 

struct TraccarPosition {
  double latitude;
  double longitude;
  double altitudeMeters;
  double speedKmh;
  double headingDeg;
  double hdop;
  double accuracyMeters;
  uint64_t timestampMs;
  int batteryPercent;
  int validFlag;
  bool charging;
  String driverUniqueId;
  String cell;
  String wifi;
  String eventName;
  String activityType;
  double odometer;

  TraccarPosition()
    : latitude(NAN), longitude(NAN), altitudeMeters(NAN),
      speedKmh(NAN), headingDeg(NAN), hdop(NAN), accuracyMeters(NAN),
      timestampMs(0), batteryPercent(-1), validFlag(-1), charging(false),
      driverUniqueId(""), cell(""), wifi(""), eventName(""), activityType(""), odometer(NAN) {}
};

class TraccarClient {
public:
  TraccarClient();
  TraccarClient(const String& hostUrl, uint16_t port, const String& deviceId);

  void setHost(const String& hostUrl);
  void setPort(uint16_t port);
  void setDeviceId(const String& deviceId);
  void setBasePath(const String& basePath); // default "/"
  void setDebug(bool enabled);
  void setTimeoutMs(uint16_t connectTimeoutMs);

  bool sendOsmAnd(const TraccarPosition& pos, int* outHttpCode = nullptr) const;
  bool sendJson(const TraccarPosition& pos, int* outHttpCode = nullptr) const;
  bool sendOsmAndForm(const TraccarPosition& pos, int* outHttpCode = nullptr) const;

  String buildOsmAndUrl(const TraccarPosition& pos) const;

private:
  String makeBaseUrl() const; // host[:port][basePath]
  static String formatIso8601(uint64_t epochMs);

  String _host;      // with schema, e.g. "http://example.com"
  uint16_t _port;    // e.g. 5055
  String _deviceId;  // device id string
  String _basePath;  // usually "/"
  bool _debug;
  uint16_t _connectTimeoutMs;
};

 
#endif // ARDUINO
#endif // __cplusplus

#endif // TRACCAR_CLIENT_H

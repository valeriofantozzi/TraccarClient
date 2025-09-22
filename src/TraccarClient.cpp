#include "TraccarClient.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

#ifdef ARDUINO
#include <Arduino.h>
#include <HTTPClient.h>
#endif

struct traccar_client_s {
  char* host;      // includes scheme, e.g. http://example
  uint16_t port;   // 5055
  char* device_id; // id
  char* base_path; // "/"
  bool debug;
  uint16_t timeout_ms;
};

static inline bool tr_is_provided(double v) {
  return !isnan(v);
}

static char* tr_strdup(const char* s) {
  if (!s) return nullptr;
  size_t n = strlen(s) + 1;
  char* p = (char*)malloc(n);
  if (p) memcpy(p, s, n);
  return p;
}

static size_t tr_append(char* out, size_t out_size, size_t* idx, const char* s) {
  size_t n = strlen(s);
  if (*idx >= out_size) return 0;
  size_t avail = out_size - *idx;
  size_t tocpy = (n < avail ? n : avail - 1);
  if (tocpy > 0) memcpy(out + *idx, s, tocpy);
  *idx += tocpy;
  out[(*idx < out_size) ? *idx : out_size-1] = '\0';
  return tocpy;
}

static inline bool tr_is_unreserved(char c) {
  if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) return true;
  switch (c) { case '-': case '.': case '_': case '~': return true; default: return false; }
}

static void tr_append_urlenc(char* out, size_t out_size, size_t* idx, const char* s) {
  static const char hex[] = "0123456789ABCDEF";
  for (const unsigned char* p = (const unsigned char*)s; p && *p; ++p) {
    unsigned char c = *p;
    if (tr_is_unreserved((char)c)) {
      char ch[2] = {(char)c, '\0'};
      tr_append(out, out_size, idx, ch);
    } else if (c == ' ') {
      tr_append(out, out_size, idx, "%20");
    } else {
      char enc[4]; enc[0] = '%'; enc[1] = hex[(c >> 4) & 0xF]; enc[2] = hex[c & 0xF]; enc[3] = '\0';
      tr_append(out, out_size, idx, enc);
    }
  }
}

static void tr_build_base_url(const traccar_client_t* c, char* out, size_t out_size) {
  size_t idx = 0; out[0] = '\0';
  if (c->host && *c->host) tr_append(out, out_size, &idx, c->host);
  if (c->port) {
    char buf[12]; snprintf(buf, sizeof(buf), ":%u", (unsigned)c->port);
    tr_append(out, out_size, &idx, buf);
  }
  if (c->base_path && *c->base_path) {
    if (c->base_path[0] != '/') tr_append(out, out_size, &idx, "/");
    tr_append(out, out_size, &idx, c->base_path);
  }
  if (idx == 0 || out[idx-1] != '/') tr_append(out, out_size, &idx, "/");
}

static uint64_t tr_now_ms_or_0() {
  time_t now = time(nullptr);
  if (now > 100000) return (uint64_t)now * 1000ULL;
  return 0;
}

traccar_client_t* traccar_create(const char* host_url, uint16_t port, const char* device_id) {
  traccar_client_t* c = (traccar_client_t*)calloc(1, sizeof(*c));
  if (!c) return nullptr;
  c->host = tr_strdup(host_url);
  c->port = port;
  c->device_id = tr_strdup(device_id);
  c->base_path = tr_strdup("/");
  c->debug = false;
  c->timeout_ms = 4000;
  return c;
}

void traccar_destroy(traccar_client_t* c) {
  if (!c) return;
  free(c->host);
  free(c->device_id);
  free(c->base_path);
  free(c);
}

void traccar_set_base_path(traccar_client_t* c, const char* base_path) {
  if (!c) return;
  free(c->base_path);
  c->base_path = tr_strdup((base_path && *base_path) ? base_path : "/");
}

void traccar_set_debug(traccar_client_t* c, bool enabled) {
  if (!c) return;
  c->debug = enabled;
}

void traccar_set_timeout_ms(traccar_client_t* c, uint16_t timeout_ms) {
  if (!c) return;
  c->timeout_ms = timeout_ms;
}

size_t traccar_build_osmand_url(traccar_client_t* c, const traccar_position_t* pos, char* out, size_t out_size) {
  if (!c || !pos || !out || out_size == 0) return 0;
  size_t idx = 0; out[0] = '\0';
  char base[192]; tr_build_base_url(c, base, sizeof(base));
  tr_append(out, out_size, &idx, base);
  tr_append(out, out_size, &idx, "?");
  tr_append(out, out_size, &idx, "id=");
  if (c->device_id) tr_append_urlenc(out, out_size, &idx, c->device_id); else tr_append(out, out_size, &idx, "");
  char buf[48];
  if (tr_is_provided(pos->latitude))  { snprintf(buf, sizeof(buf), "&lat=%.7f", pos->latitude); tr_append(out, out_size, &idx, buf); }
  if (tr_is_provided(pos->longitude)) { snprintf(buf, sizeof(buf), "&lon=%.7f", pos->longitude); tr_append(out, out_size, &idx, buf); }
  if (tr_is_provided(pos->altitudeMeters)) { snprintf(buf, sizeof(buf), "&altitude=%.1f", pos->altitudeMeters); tr_append(out, out_size, &idx, buf); }
  if (tr_is_provided(pos->hdop)) { snprintf(buf, sizeof(buf), "&hdop=%.2f", pos->hdop); tr_append(out, out_size, &idx, buf); }
  if (tr_is_provided(pos->speedKmh)) { 
#if TRACCAR_SPEED_ROUND_DOWN
    snprintf(buf, sizeof(buf), "&speed=%d", (int)floor(pos->speedKmh / 1.852));
#else
    snprintf(buf, sizeof(buf), "&speed=%d", (int)round(pos->speedKmh / 1.852));
#endif
    tr_append(out, out_size, &idx, buf); 
  }
  if (pos->validFlag >= 0) tr_append(out, out_size, &idx, pos->validFlag ? "&valid=true" : "&valid=false");
  uint64_t ts = pos->timestampMs ? pos->timestampMs : tr_now_ms_or_0();
  if (ts) { snprintf(buf, sizeof(buf), "&timestamp=%llu", (unsigned long long)ts); tr_append(out, out_size, &idx, buf); }
  if (tr_is_provided(pos->accuracyMeters)) { snprintf(buf, sizeof(buf), "&accuracy=%.1f", pos->accuracyMeters); tr_append(out, out_size, &idx, buf); }
  if (tr_is_provided(pos->headingDeg)) { snprintf(buf, sizeof(buf), "&heading=%.1f", pos->headingDeg); tr_append(out, out_size, &idx, buf); }
  if (pos->batteryPercent >= 0) {
    snprintf(buf, sizeof(buf), "&batt=%d", (int)pos->batteryPercent); tr_append(out, out_size, &idx, buf);
    tr_append(out, out_size, &idx, pos->charging ? "&charge=true" : "&charge=false");
  }
  if (pos->driverUniqueId && *pos->driverUniqueId) { tr_append(out, out_size, &idx, "&driverUniqueId="); tr_append_urlenc(out, out_size, &idx, pos->driverUniqueId); }
  if (pos->cell && *pos->cell) { tr_append(out, out_size, &idx, "&cell="); tr_append_urlenc(out, out_size, &idx, pos->cell); }
  if (pos->wifi && *pos->wifi) { tr_append(out, out_size, &idx, "&wifi="); tr_append_urlenc(out, out_size, &idx, pos->wifi); }
  if (pos->eventName && *pos->eventName) { tr_append(out, out_size, &idx, "&event="); tr_append_urlenc(out, out_size, &idx, pos->eventName); }
  if (pos->activityType && *pos->activityType) { tr_append(out, out_size, &idx, "&activity="); tr_append_urlenc(out, out_size, &idx, pos->activityType); }
  if (tr_is_provided(pos->odometer)) { snprintf(buf, sizeof(buf), "&odometer=%.1f", pos->odometer); tr_append(out, out_size, &idx, buf); }
  return idx;
}

size_t traccar_build_osmand_form_body(traccar_client_t* c, const traccar_position_t* pos, char* out, size_t out_size) {
  if (!c || !pos || !out || out_size == 0) return 0;
  size_t idx = 0; out[0] = '\0';
  char buf[48];
  tr_append(out, out_size, &idx, "id="); if (c->device_id) tr_append_urlenc(out, out_size, &idx, c->device_id);
  if (tr_is_provided(pos->latitude))  { snprintf(buf, sizeof(buf), "&lat=%.7f", pos->latitude); tr_append(out, out_size, &idx, buf); }
  if (tr_is_provided(pos->longitude)) { snprintf(buf, sizeof(buf), "&lon=%.7f", pos->longitude); tr_append(out, out_size, &idx, buf); }
  if (tr_is_provided(pos->altitudeMeters)) { snprintf(buf, sizeof(buf), "&altitude=%.1f", pos->altitudeMeters); tr_append(out, out_size, &idx, buf); }
  if (tr_is_provided(pos->hdop)) { snprintf(buf, sizeof(buf), "&hdop=%.2f", pos->hdop); tr_append(out, out_size, &idx, buf); }
  if (tr_is_provided(pos->speedKmh)) { 
#if TRACCAR_SPEED_ROUND_DOWN
    snprintf(buf, sizeof(buf), "&speed=%d", (int)floor(pos->speedKmh / 1.852));
#else
    snprintf(buf, sizeof(buf), "&speed=%d", (int)round(pos->speedKmh / 1.852));
#endif
    tr_append(out, out_size, &idx, buf); 
  }
  if (pos->validFlag >= 0) tr_append(out, out_size, &idx, pos->validFlag ? "&valid=true" : "&valid=false");
  uint64_t ts = pos->timestampMs ? pos->timestampMs : tr_now_ms_or_0();
  if (ts) { snprintf(buf, sizeof(buf), "&timestamp=%llu", (unsigned long long)ts); tr_append(out, out_size, &idx, buf); }
  if (tr_is_provided(pos->accuracyMeters)) { snprintf(buf, sizeof(buf), "&accuracy=%.1f", pos->accuracyMeters); tr_append(out, out_size, &idx, buf); }
  if (tr_is_provided(pos->headingDeg)) { snprintf(buf, sizeof(buf), "&heading=%.1f", pos->headingDeg); tr_append(out, out_size, &idx, buf); }
  if (pos->batteryPercent >= 0) {
    snprintf(buf, sizeof(buf), "&batt=%d", (int)pos->batteryPercent); tr_append(out, out_size, &idx, buf);
    tr_append(out, out_size, &idx, pos->charging ? "&charge=true" : "&charge=false");
  }
  if (pos->driverUniqueId && *pos->driverUniqueId) { tr_append(out, out_size, &idx, "&driverUniqueId="); tr_append_urlenc(out, out_size, &idx, pos->driverUniqueId); }
  if (pos->cell && *pos->cell) { tr_append(out, out_size, &idx, "&cell="); tr_append_urlenc(out, out_size, &idx, pos->cell); }
  if (pos->wifi && *pos->wifi) { tr_append(out, out_size, &idx, "&wifi="); tr_append_urlenc(out, out_size, &idx, pos->wifi); }
  if (pos->eventName && *pos->eventName) { tr_append(out, out_size, &idx, "&event="); tr_append_urlenc(out, out_size, &idx, pos->eventName); }
  if (pos->activityType && *pos->activityType) { tr_append(out, out_size, &idx, "&activity="); tr_append_urlenc(out, out_size, &idx, pos->activityType); }
  if (tr_is_provided(pos->odometer)) { snprintf(buf, sizeof(buf), "&odometer=%.1f", pos->odometer); tr_append(out, out_size, &idx, buf); }
  return idx;
}

#ifdef ARDUINO

static String tr_make_base_url_arduino(const traccar_client_t* c) {
  String base; base.reserve(128);
  if (c->host && *c->host) base += c->host;
  if (c->port) { base += ":"; base += String(c->port); }
  if (c->base_path && *c->base_path) {
    if (c->base_path[0] != '/') base += "/";
    base += c->base_path;
  }
  if (base.length() == 0 || base[base.length()-1] != '/') base += "/";
  return base;
}

bool traccar_send_osmand(traccar_client_t* c, const traccar_position_t* pos, int* out_http_code) {
  if (!c || !pos || !c->device_id || !*c->device_id) return false;
  char url[384]; traccar_build_osmand_url(c, pos, url, sizeof(url));
  HTTPClient http;
  http.setConnectTimeout(c->timeout_ms);
  if (!http.begin(String(url))) {
    if (c->debug) Serial.println("[Traccar] http.begin failed");
    return false;
  }
  int code = http.GET();
  http.end();
  if (c->debug) Serial.printf("[Traccar] GET %d\n", code);
  if (out_http_code) *out_http_code = code;
  return code == 200;
}

bool traccar_send_osmand_form(traccar_client_t* c, const traccar_position_t* pos, int* out_http_code) {
  if (!c || !pos || !c->device_id || !*c->device_id) return false;
  String base = tr_make_base_url_arduino(c);
  HTTPClient http;
  http.setConnectTimeout(c->timeout_ms);
  if (!http.begin(base)) {
    if (c->debug) Serial.println("[Traccar] http.begin failed");
    return false;
  }
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  char bodyBuf[384]; traccar_build_osmand_form_body(c, pos, bodyBuf, sizeof(bodyBuf));
  int code = http.POST((uint8_t*)bodyBuf, strlen(bodyBuf));
  http.end();
  if (c->debug) Serial.printf("[Traccar] POST form %d\n", code);
  if (out_http_code) *out_http_code = code;
  return code == 200;
}

static String tr_format_iso8601(uint64_t epochMs) {
  if (epochMs == 0) return String("");
  time_t sec = (time_t)(epochMs / 1000ULL);
  struct tm tmv; gmtime_r(&sec, &tmv);
  char buf[32]; int ms = (int)(epochMs % 1000ULL);
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
           tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
           tmv.tm_hour, tmv.tm_min, tmv.tm_sec, ms);
  return String(buf);
}

bool traccar_send_json(traccar_client_t* c, const traccar_position_t* pos, int* out_http_code) {
  if (!c || !pos || !c->device_id || !*c->device_id) return false;
  String base = tr_make_base_url_arduino(c);
  HTTPClient http;
  http.setConnectTimeout(c->timeout_ms);
  if (!http.begin(base)) {
    if (c->debug) Serial.println("[Traccar] http.begin failed");
    return false;
  }
  http.addHeader("Content-Type", "application/json");

  uint64_t ts = pos->timestampMs ? pos->timestampMs : tr_now_ms_or_0();
  String tsIso = tr_format_iso8601(ts);

  String body; body.reserve(384);
  body += "{";
  body += "\"id\":\""; body += c->device_id; body += "\"";
  
  if (tr_is_provided(pos->latitude)) { body += ",\"lat\":"; body += String(pos->latitude, 7); }
  if (tr_is_provided(pos->longitude)) { body += ",\"lon\":"; body += String(pos->longitude, 7); }
  if (tr_is_provided(pos->altitudeMeters)) { body += ",\"altitude\":"; body += String(pos->altitudeMeters, 1); }
  if (tr_is_provided(pos->speedKmh)) { 
#if TRACCAR_SPEED_ROUND_DOWN
    body += ",\"speed\":"; body += String((int)floor(pos->speedKmh / 1.852)); // convert km/h to knots, rounded down
#else
    body += ",\"speed\":"; body += String((int)round(pos->speedKmh / 1.852)); // convert km/h to knots, rounded
#endif
  }
  if (tr_is_provided(pos->headingDeg)) { body += ",\"heading\":"; body += String(pos->headingDeg, 1); }
  if (tr_is_provided(pos->hdop)) { body += ",\"hdop\":"; body += String(pos->hdop, 2); }
  if (tr_is_provided(pos->accuracyMeters)) { body += ",\"accuracy\":"; body += String(pos->accuracyMeters, 1); }
  if (pos->validFlag >= 0) { body += ",\"valid\":"; body += (pos->validFlag ? "true" : "false"); }
  if (tsIso.length()) { body += ",\"timestamp\":\""; body += tsIso; body += "\""; }
  if (tr_is_provided(pos->odometer)) { body += ",\"odometer\":"; body += String(pos->odometer, 1); }
  if (pos->batteryPercent >= 0) { body += ",\"batt\":"; body += String(pos->batteryPercent); }
  if (pos->charging) { body += ",\"charge\":true"; }
  if (pos->eventName && *pos->eventName) { body += ",\"event\":\""; body += pos->eventName; body += "\""; }
  if (pos->activityType && *pos->activityType) { body += ",\"activity\":\""; body += pos->activityType; body += "\""; }
  if (pos->driverUniqueId && *pos->driverUniqueId) { body += ",\"driverUniqueId\":\""; body += pos->driverUniqueId; body += "\""; }
  if (pos->cell && *pos->cell) { body += ",\"cell\":\""; body += pos->cell; body += "\""; }
  if (pos->wifi && *pos->wifi) { body += ",\"wifi\":\""; body += pos->wifi; body += "\""; }
  
  body += "}";

  if (c->debug) {
    Serial.printf("[Traccar] POST to: %s\n", base.c_str());
    Serial.printf("[Traccar] JSON body: %s\n", body.c_str());
  }

  int code = http.POST(body);
  http.end();
  if (c->debug) Serial.printf("[Traccar] POST %d\n", code);
  if (out_http_code) *out_http_code = code;
  return code == 200;
}

#else
// Non-Arduino host build stubs (no HTTP)
bool traccar_send_osmand(traccar_client_t* c, const traccar_position_t* pos, int* out_http_code) {
  (void)c; (void)pos; if (out_http_code) *out_http_code = 0; return false;
}
bool traccar_send_json(traccar_client_t* c, const traccar_position_t* pos, int* out_http_code) {
  (void)c; (void)pos; if (out_http_code) *out_http_code = 0; return false;
}
bool traccar_send_osmand_form(traccar_client_t* c, const traccar_position_t* pos, int* out_http_code) {
  (void)c; (void)pos; if (out_http_code) *out_http_code = 0; return false;
}
#endif

#ifdef ARDUINO
// ----------------- C++ Arduino wrapper -----------------

static inline bool isProvided(double v) { return !isnan(v); }

TraccarClient::TraccarClient()
  : _host(""), _port(5055), _deviceId(""), _basePath("/"), _debug(false), _connectTimeoutMs(4000) {}

TraccarClient::TraccarClient(const String& hostUrl, uint16_t port, const String& deviceId)
  : _host(hostUrl), _port(port), _deviceId(deviceId), _basePath("/"), _debug(false), _connectTimeoutMs(4000) {}

void TraccarClient::setHost(const String& hostUrl) { _host = hostUrl; }
void TraccarClient::setPort(uint16_t port) { _port = port; }
void TraccarClient::setDeviceId(const String& deviceId) { _deviceId = deviceId; }
void TraccarClient::setBasePath(const String& basePath) { _basePath = basePath.length() ? basePath : "/"; }
void TraccarClient::setDebug(bool enabled) { _debug = enabled; }
void TraccarClient::setTimeoutMs(uint16_t connectTimeoutMs) { _connectTimeoutMs = connectTimeoutMs; }

String TraccarClient::makeBaseUrl() const {
  String base; base.reserve(_host.length() + 16 + _basePath.length());
  base += _host;
  if (_port) { base += ":"; base += String(_port); }
  if (_basePath.length()) {
    if (_basePath[0] != '/') base += "/";
    base += _basePath;
  }
  if (base.length() == 0 || base[base.length()-1] != '/') base += "/";
  return base;
}

String TraccarClient::formatIso8601(uint64_t epochMs) {
  if (epochMs == 0) return String("");
  time_t sec = (time_t)(epochMs / 1000ULL);
  struct tm tmv;
  gmtime_r(&sec, &tmv);
  char buf[32];
  int ms = (int)(epochMs % 1000ULL);
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
           tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
           tmv.tm_hour, tmv.tm_min, tmv.tm_sec, ms);
  return String(buf);
}

String TraccarClient::buildOsmAndUrl(const TraccarPosition& pos) const {
  traccar_position_t p{};
  p.latitude = pos.latitude;
  p.longitude = pos.longitude;
  p.altitudeMeters = pos.altitudeMeters;
  p.speedKmh = pos.speedKmh;
  p.headingDeg = pos.headingDeg;
  p.hdop = pos.hdop;
  p.accuracyMeters = pos.accuracyMeters;
  p.timestampMs = pos.timestampMs;
  p.batteryPercent = pos.batteryPercent;
  p.validFlag = pos.validFlag;
  p.charging = pos.charging;
  p.driverUniqueId = pos.driverUniqueId.length() ? pos.driverUniqueId.c_str() : nullptr;
  p.cell = pos.cell.length() ? pos.cell.c_str() : nullptr;
  p.wifi = pos.wifi.length() ? pos.wifi.c_str() : nullptr;
  p.eventName = pos.eventName.length() ? pos.eventName.c_str() : nullptr;
  p.activityType = pos.activityType.length() ? pos.activityType.c_str() : nullptr;
  p.odometer = pos.odometer;

  traccar_client_t tmp{0};
  tmp.host = (char*)_host.c_str();
  tmp.port = _port;
  tmp.device_id = (char*)_deviceId.c_str();
  tmp.base_path = (char*)_basePath.c_str();
  tmp.debug = _debug;
  tmp.timeout_ms = _connectTimeoutMs;

  char url[384]; traccar_build_osmand_url(&tmp, &p, url, sizeof(url));
  return String(url);
}

bool TraccarClient::sendOsmAnd(const TraccarPosition& pos, int* outHttpCode) const {
  if (_host.length() == 0 || _deviceId.length() == 0) return false;
  String url = buildOsmAndUrl(pos);
  HTTPClient http;
  http.setConnectTimeout(_connectTimeoutMs);
  if (!http.begin(url)) {
    if (_debug) Serial.println("[Traccar] http.begin failed");
    return false;
  }
  int code = http.GET();
  http.end();
  if (_debug) Serial.printf("[Traccar] GET %d\n", code);
  if (outHttpCode) *outHttpCode = code;
  return (code == 200);
}

bool TraccarClient::sendJson(const TraccarPosition& pos, int* outHttpCode) const {
  if (_host.length() == 0 || _deviceId.length() == 0) return false;
  String base = makeBaseUrl();
  HTTPClient http;
  http.setConnectTimeout(_connectTimeoutMs);
  if (!http.begin(base)) {
    if (_debug) Serial.println("[Traccar] http.begin failed");
    return false;
  }
  http.addHeader("Content-Type", "application/json");

  uint64_t ts = pos.timestampMs ? pos.timestampMs : tr_now_ms_or_0();
  String tsIso = formatIso8601(ts);

  String body; body.reserve(384);
  body += "{";
  body += "\"id\":\""; body += _deviceId; body += "\"";
  
  if (isProvided(pos.latitude)) { body += ",\"lat\":"; body += String(pos.latitude, 7); }
  if (isProvided(pos.longitude)) { body += ",\"lon\":"; body += String(pos.longitude, 7); }
  if (isProvided(pos.altitudeMeters)) { body += ",\"altitude\":"; body += String(pos.altitudeMeters, 1); }
  if (isProvided(pos.speedKmh)) { 
#if TRACCAR_SPEED_ROUND_DOWN
    body += ",\"speed\":"; body += String((int)floor(pos.speedKmh / 1.852)); // convert km/h to knots, rounded down
#else
    body += ",\"speed\":"; body += String((int)round(pos.speedKmh / 1.852)); // convert km/h to knots, rounded
#endif
  }
  if (isProvided(pos.headingDeg)) { body += ",\"heading\":"; body += String(pos.headingDeg, 1); }
  if (isProvided(pos.hdop)) { body += ",\"hdop\":"; body += String(pos.hdop, 2); }
  if (isProvided(pos.accuracyMeters)) { body += ",\"accuracy\":"; body += String(pos.accuracyMeters, 1); }
  if (pos.validFlag >= 0) { body += ",\"valid\":"; body += (pos.validFlag ? "true" : "false"); }
  if (tsIso.length()) { body += ",\"timestamp\":\""; body += tsIso; body += "\""; }
  if (isProvided(pos.odometer)) { body += ",\"odometer\":"; body += String(pos.odometer, 1); }
  if (pos.batteryPercent >= 0) { body += ",\"batt\":"; body += String(pos.batteryPercent); }
  if (pos.charging) { body += ",\"charge\":true"; }
  if (pos.eventName.length()) { body += ",\"event\":\""; body += pos.eventName; body += "\""; }
  if (pos.activityType.length()) { body += ",\"activity\":\""; body += pos.activityType; body += "\""; }
  if (pos.driverUniqueId.length()) { body += ",\"driverUniqueId\":\""; body += pos.driverUniqueId; body += "\""; }
  if (pos.cell.length()) { body += ",\"cell\":\""; body += pos.cell; body += "\""; }
  if (pos.wifi.length()) { body += ",\"wifi\":\""; body += pos.wifi; body += "\""; }
  
  body += "}";

  if (_debug) {
    Serial.printf("[Traccar] POST to: %s\n", base.c_str());
    Serial.printf("[Traccar] JSON body: %s\n", body.c_str());
  }

  int code = http.POST(body);
  http.end();
  if (_debug) Serial.printf("[Traccar] POST %d\n", code);
  if (outHttpCode) *outHttpCode = code;
  return (code == 200);
}

bool TraccarClient::sendOsmAndForm(const TraccarPosition& pos, int* outHttpCode) const {
  if (_host.length() == 0 || _deviceId.length() == 0) return false;
  traccar_position_t p{};
  p.latitude = pos.latitude;
  p.longitude = pos.longitude;
  p.altitudeMeters = pos.altitudeMeters;
  p.speedKmh = pos.speedKmh;
  p.headingDeg = pos.headingDeg;
  p.hdop = pos.hdop;
  p.accuracyMeters = pos.accuracyMeters;
  p.timestampMs = pos.timestampMs;
  p.batteryPercent = pos.batteryPercent;
  p.validFlag = pos.validFlag;
  p.charging = pos.charging;
  p.driverUniqueId = pos.driverUniqueId.length() ? pos.driverUniqueId.c_str() : nullptr;
  p.cell = pos.cell.length() ? pos.cell.c_str() : nullptr;
  p.wifi = pos.wifi.length() ? pos.wifi.c_str() : nullptr;
  p.eventName = pos.eventName.length() ? pos.eventName.c_str() : nullptr;
  p.activityType = pos.activityType.length() ? pos.activityType.c_str() : nullptr;
  p.odometer = pos.odometer;

  traccar_client_t tmp{0};
  tmp.host = (char*)_host.c_str();
  tmp.port = _port;
  tmp.device_id = (char*)_deviceId.c_str();
  tmp.base_path = (char*)_basePath.c_str();
  tmp.debug = _debug;
  tmp.timeout_ms = _connectTimeoutMs;

  String base = makeBaseUrl();
  HTTPClient http;
  http.setConnectTimeout(_connectTimeoutMs);
  if (!http.begin(base)) {
    if (_debug) Serial.println("[Traccar] http.begin failed");
    return false;
  }
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  char bodyBuf[384]; traccar_build_osmand_form_body(&tmp, &p, bodyBuf, sizeof(bodyBuf));
  int code = http.POST((uint8_t*)bodyBuf, strlen(bodyBuf));
  http.end();
  if (_debug) Serial.printf("[Traccar] POST form %d\n", code);
  if (outHttpCode) *outHttpCode = code;
  return (code == 200);
}

 
#endif // ARDUINO


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ---------- HOME WI-FI ----------
const char* WIFI_SSID = "Your_Wifi_Name";
const char* WIFI_PASSWORD = "Password";

// ---------- SENSOR AND OUTPUT PINS ----------
const int GAS_PIN = A0;
const int FLAME_PIN = D5;
const int BUZZER_PIN = D6;
const int RELAY_PIN = D1;

// ---------- CONFIGURATION ----------
const int GAS_THRESHOLD = 100;  // Set your calibrated value

const bool FLAME_ACTIVE_LOW = true;

// Most relay modules are active LOW.
// Verify this with your relay module before connecting the pump.
const bool RELAY_ACTIVE_LOW = false;

// ---------- WEB SERVER ----------
ESP8266WebServer server(80);

int gasValue = 0;
bool flameDetected = false;
bool gasDetected = false;
bool alarmActive = false;
bool pumpActive = false;
bool serverStarted = false;

unsigned long lastWiFiAttempt = 0;

// ---------- MOBILE DASHBOARD ----------
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="theme-color" content="#eef2f7">
<title>Fire & Gas Monitor</title>
<style>
body {
  margin: 0;
  padding: 18px;
  background: #eef2f7;
  color: #172033;
  font-family: Arial, sans-serif;
}
.container { max-width: 500px; margin: auto; }
h1 { text-align: center; font-size: 25px; }
.card {
  background: white;
  border-radius: 14px;
  padding: 20px;
  margin: 14px 0;
  box-shadow: 0 3px 12px #0001;
}
.small { color: #64748b; font-size: 13px; }
.value { font-size: 30px; font-weight: bold; margin-top: 8px; }
.status { font-size: 21px; font-weight: bold; margin-top: 8px; }
.normal { color: #13803c; }
.danger { color: #d82020; }
</style>
</head>
<body>
<div class="container">
  <h1>Fire & Gas Detector</h1>

  <div class="card">
    <div class="small">SYSTEM STATUS</div>
    <div id="alarm" class="status">Connecting...</div>
  </div>

  <div class="card">
    <div class="small">MQ-2 GAS READING</div>
    <div class="value"><span id="gas">--</span> / 1023</div>
    <div class="small">Threshold: <span id="threshold">--</span></div>
    <div id="gasStatus" class="status">--</div>
  </div>

  <div class="card">
    <div class="small">FLAME SENSOR</div>
    <div id="flame" class="status">--</div>
  </div>

  <div class="card">
    <div class="small">BUZZER</div>
    <div id="buzzer" class="status">--</div>
  </div>

  <div class="card">
    <div class="small">WATER PUMP</div>
    <div id="pump" class="status">--</div>
    <div class="small">Automatically controlled by flame detection</div>
  </div>

  <p id="connection" class="small" style="text-align:center">
    Connecting to detector...
  </p>
  <p class="small" style="text-align:center">
    Updates every 2 seconds
  </p>
</div>

<script>
async function updateStatus() {
  try {
    const response = await fetch('/data', {cache: 'no-store'});
    if (!response.ok) throw new Error('Request failed');

    const d = await response.json();

    document.getElementById('gas').textContent = d.gas;
    document.getElementById('threshold').textContent = d.threshold;

    const alarm = document.getElementById('alarm');
    alarm.textContent = d.alarm ? 'DANGER DETECTED' : 'NORMAL';
    alarm.className = 'status ' + (d.alarm ? 'danger' : 'normal');

    const gasStatus = document.getElementById('gasStatus');
    gasStatus.textContent = d.gasDetected ? 'Gas threshold exceeded' : 'Gas normal';
    gasStatus.className = 'status ' +
      (d.gasDetected ? 'danger' : 'normal');

    const flame = document.getElementById('flame');
    flame.textContent = d.flame ? 'FLAME DETECTED' : 'No flame detected';
    flame.className = 'status ' + (d.flame ? 'danger' : 'normal');

    const buzzer = document.getElementById('buzzer');
    buzzer.textContent = d.alarm ? 'ON' : 'OFF';
    buzzer.className = 'status ' + (d.alarm ? 'danger' : 'normal');

    const pump = document.getElementById('pump');
    pump.textContent = d.pump ? 'ON' : 'OFF';
    pump.className = 'status ' + (d.pump ? 'danger' : 'normal');

    document.getElementById('connection').textContent =
      'Connected to detector';
  } catch (e) {
    document.getElementById('connection').textContent =
      'Connection lost. Retrying...';
  }
}

updateStatus();
setInterval(updateStatus, 2000);
</script>
</body>
</html>
)rawliteral";

// ---------- WEB HANDLERS ----------
void handleHome() {
  server.send_P(200, "text/html", webpage);
}

void handleData() {
  String json = "{";
  json += "\"gas\":" + String(gasValue) + ",";
  json += "\"threshold\":" + String(GAS_THRESHOLD) + ",";
  json += "\"gasDetected\":";
  json += (gasDetected ? "true" : "false");
  json += ",\"flame\":";
  json += (flameDetected ? "true" : "false");
  json += ",\"alarm\":";
  json += (alarmActive ? "true" : "false");
  json += ",\"pump\":";
  json += (pumpActive ? "true" : "false");
  json += "}";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void startWebServer() {
  server.on("/", HTTP_GET, handleHome);
  server.on("/data", HTTP_GET, handleData);
  server.begin();

  serverStarted = true;

  Serial.println();
  Serial.println("Web server started.");
  Serial.print("Open on your phone: http://");
  Serial.println(WiFi.localIP());
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);

  pinMode(FLAME_PIN, INPUT_PULLUP);

  // Set pump OFF before enabling the relay output.
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
  pinMode(RELAY_PIN, OUTPUT);

  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println();
  Serial.println("Fire and Gas Detector starting...");
  Serial.println("Connecting to Wi-Fi...");
}

// ---------- MAIN LOOP ----------
void loop() {
  // Read sensors continuously.
  gasValue = analogRead(GAS_PIN);
  gasDetected = gasValue >= GAS_THRESHOLD;

  int flameReading = digitalRead(FLAME_PIN);

  flameDetected = FLAME_ACTIVE_LOW
                  ? (flameReading == LOW)
                  : (flameReading == HIGH);

  alarmActive = gasDetected || flameDetected;

  // Local buzzer responds to either gas or flame.
  if (alarmActive) {
    tone(BUZZER_PIN, 2000);
  } else {
    noTone(BUZZER_PIN);
  }

  // Pump activates only for flame detection, not gas alone.
  pumpActive = flameDetected;

  digitalWrite(
    RELAY_PIN,
    pumpActive
      ? (RELAY_ACTIVE_LOW ? LOW : HIGH)
      : (RELAY_ACTIVE_LOW ? HIGH : LOW)
  );

  // Start server after Wi-Fi connects.
  if (WiFi.status() == WL_CONNECTED) {
    if (!serverStarted) {
      startWebServer();
    }

    server.handleClient();
  } else {
    if (millis() - lastWiFiAttempt >= 10000) {
      lastWiFiAttempt = millis();
      Serial.println("Wi-Fi disconnected; reconnecting...");
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
  }

  delay(10);
}

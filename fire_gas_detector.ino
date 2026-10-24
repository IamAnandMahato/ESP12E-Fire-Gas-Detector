/*
  ESP-12E / ESP8266 Fire and Gas Detector
  Features:
    - MQ-2 gas/smoke analog reading on A0
    - Flame sensor digital output on D5
    - Buzzer signal on D6
    - Relay input on D1 (water pump activates on flame only)
    - Local Wi-Fi web dashboard

  IMPORTANT:
    - Set your Wi-Fi credentials below.
    - Confirm your relay's active level before connecting the pump.
    - This is an educational prototype, not a certified fire safety system.
    - Never use water on electrical, oil, or chemical fires.
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// -------- Wi-Fi settings --------
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// -------- Pin settings (NodeMCU labels) --------
const uint8_t GAS_PIN = A0;
const uint8_t FLAME_PIN = D5;
const uint8_t BUZZER_PIN = D6;
const uint8_t RELAY_PIN = D1;

// -------- Calibration / module settings --------
// Tune this after observing MQ-2 readings in your environment.
const int GAS_THRESHOLD = 450;

// Many flame sensor modules pull DO LOW when flame is detected.
const bool FLAME_ACTIVE_LOW = true;

// Many relay modules are active LOW. Change to false if your relay is active HIGH.
const bool RELAY_ACTIVE_LOW = true;

// Many active buzzer modules sound when SIG is HIGH.
// If your buzzer module is active LOW, change this to false.
const bool BUZZER_ACTIVE_HIGH = true;

ESP8266WebServer server(80);

int gasValue = 0;
bool flameDetected = false;
bool gasDetected = false;
bool alarmActive = false;
bool pumpActive = false;

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL_MS = 250;

void setRelay(bool on) {
  pumpActive = on;
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(RELAY_PIN, on ? LOW : HIGH);
  } else {
    digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  }
}

void setBuzzer(bool on) {
  if (BUZZER_ACTIVE_HIGH) {
    digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
  } else {
    digitalWrite(BUZZER_PIN, on ? LOW : HIGH);
  }
}

void readSensors() {
  gasValue = analogRead(GAS_PIN);
  int flameState = digitalRead(FLAME_PIN);

  flameDetected = FLAME_ACTIVE_LOW ? (flameState == LOW) : (flameState == HIGH);
  gasDetected = (gasValue >= GAS_THRESHOLD);

  // Alarm for either gas or flame; pump only for flame.
  alarmActive = gasDetected || flameDetected;
  setBuzzer(alarmActive);
  setRelay(flameDetected);
}

String jsonBool(bool value) {
  return value ? "true" : "false";
}

void handleRoot() {
  const char page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP-12E Fire & Gas Monitor</title>
<style>
  body{font-family:Arial,sans-serif;margin:0;background:#f3f6fb;color:#172033}
  header{background:#172b4d;color:#fff;padding:22px 16px;text-align:center}
  main{max-width:760px;margin:20px auto;padding:0 14px}
  .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:14px}
  .card{background:#fff;border-radius:14px;padding:18px;box-shadow:0 3px 12px #15284b14}
  .label{color:#637083;font-size:14px}
  .value{font-size:25px;font-weight:bold;margin-top:8px}
  .status{font-weight:bold}
  .safe{color:#16803c}.danger{color:#c62828}
  footer{text-align:center;color:#64748b;font-size:12px;padding:18px}
</style>
</head>
<body>
<header><h1>🔥 Fire & Gas Monitor</h1><p>ESP8266 NodeMCU ESP-12E</p></header>
<main>
  <div class="grid">
    <section class="card"><div class="label">MQ-2 Gas Sensor</div><div class="value" id="gasValue">--</div><p id="gasStatus" class="status">Reading...</p></section>
    <section class="card"><div class="label">Flame Sensor</div><div class="value" id="flameStatus">--</div><p>Digital sensor status</p></section>
    <section class="card"><div class="label">Buzzer Alarm</div><div class="value" id="alarmStatus">--</div><p>Activates for gas or flame</p></section>
    <section class="card"><div class="label">Water Pump Relay</div><div class="value" id="pumpStatus">--</div><p>Activates for flame detection only</p></section>
  </div>
  <section class="card" style="margin-top:14px"><div class="label">System status</div><div class="value" id="systemStatus">Connecting...</div><p>Updates automatically every 2 seconds.</p></section>
</main>
<footer>Educational prototype only — not a certified fire safety system.</footer>
<script>
async function updateData(){
  try{
    const response=await fetch('/data',{cache:'no-store'});
    const d=await response.json();
    document.getElementById('gasValue').textContent=d.gasValue;
    document.getElementById('gasStatus').textContent=d.gasDetected?'GAS/SMOKE ALERT':'Below configured threshold';
    document.getElementById('gasStatus').className='status '+(d.gasDetected?'danger':'safe');
    document.getElementById('flameStatus').textContent=d.flameDetected?'DETECTED':'Not detected';
    document.getElementById('flameStatus').className='value '+(d.flameDetected?'danger':'safe');
    document.getElementById('alarmStatus').textContent=d.alarmActive?'ON':'OFF';
    document.getElementById('alarmStatus').className='value '+(d.alarmActive?'danger':'safe');
    document.getElementById('pumpStatus').textContent=d.pumpActive?'ON':'OFF';
    document.getElementById('pumpStatus').className='value '+(d.pumpActive?'danger':'safe');
    document.getElementById('systemStatus').textContent=d.flameDetected?'FLAME ALERT':(d.gasDetected?'GAS/SMOKE ALERT':'Monitoring');
    document.getElementById('systemStatus').className='value '+((d.flameDetected||d.gasDetected)?'danger':'safe');
  }catch(e){
    document.getElementById('systemStatus').textContent='Connection lost — retrying';
    document.getElementById('systemStatus').className='value danger';
  }
}
updateData(); setInterval(updateData,2000);
</script>
</body>
</html>
)rawliteral";
  server.send_P(200, "text/html", page);
}

void handleData() {
  String json = "{";
  json += "\"gasValue\":" + String(gasValue) + ",";
  json += "\"gasDetected\":" + jsonBool(gasDetected) + ",";
  json += "\"flameDetected\":" + jsonBool(flameDetected) + ",";
  json += "\"alarmActive\":" + jsonBool(alarmActive) + ",";
  json += "\"pumpActive\":" + jsonBool(pumpActive);
  json += "}";
  server.send(200, "application/json", json);
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  unsigned long startAttempt = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected. Dashboard URL: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi connection failed. Will retry in loop.");
  }
}

unsigned long lastWiFiAttempt = 0;

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(FLAME_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  // Put outputs into OFF state before starting the sensors/server.
  setBuzzer(false);
  setRelay(false);

  Serial.println();
  Serial.println(F("ESP-12E Fire and Gas Detector starting..."));
  Serial.print(F("Gas threshold: "));
  Serial.println(GAS_THRESHOLD);

  connectWiFi();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.begin();
  Serial.println(F("Web server started on port 80."));
}

void loop() {
  server.handleClient();

  if (millis() - lastSensorRead >= SENSOR_INTERVAL_MS) {
    lastSensorRead = millis();
    readSensors();

    Serial.print(F("Gas: "));
    Serial.print(gasValue);
    Serial.print(F(" | Gas alert: "));
    Serial.print(gasDetected ? F("YES") : F("NO"));
    Serial.print(F(" | Flame: "));
    Serial.print(flameDetected ? F("YES") : F("NO"));
    Serial.print(F(" | Pump: "));
    Serial.println(pumpActive ? F("ON") : F("OFF"));
  }

  if (WiFi.status() != WL_CONNECTED && millis() - lastWiFiAttempt >= 10000) {
    lastWiFiAttempt = millis();
    Serial.println(F("Wi-Fi disconnected. Retrying..."));
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

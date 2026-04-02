#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ── WIFI ──
const char* ssid     = "vivo Y300 5G";
const char* password = "76797679";

// ── FIREBASE ──
const char* FIREBASE_URL     = "https://aqua-82fea-default-rtdb.firebaseio.com/sensors.json";
const char* FIREBASE_HISTORY = "https://aqua-82fea-default-rtdb.firebaseio.com/history.json";

// ── PINS ──
#define TURBIDITY_PIN  35
#define TDS_PIN        34
#define THERMISTOR_PIN 32
#define SALINITY_PIN   33
#define GREEN_LED      27
#define RED_LED        26
#define BUZZER         25

// ── TURBIDITY CALIBRATION ──
#define CLEAR_VOLTAGE 1.56
#define DIRTY_VOLTAGE 1.19
#define MAX_NTU       3000.0

// ── TEMP CALIBRATION ──
#define RAW_30C  880.0
#define RAW_0C   1800.0

// ── SALINITY — 4.7k resistor ──
// Voltage divider: 3.3V → 4.7kΩ → copper strips → GND
// Jab paani mein salt zyada = resistance kam = voltage zyada
#define SAL_RESISTOR  4700.0   // 4.7k ohm
#define SAL_SUPPLY    3.3      // ESP32 3.3V

// ── THRESHOLDS ──
#define TURBIDITY_LIMIT 4.0
#define TDS_LIMIT       190.0
#define TEMP_MIN        5.0
#define TEMP_MAX        35.0
#define SALINITY_LIMIT  0.5

// ── READ FUNCTIONS ──
float readTurbidity() {
  int sum = 0;
  for (int i = 0; i < 10; i++) { sum += analogRead(TURBIDITY_PIN); delay(10); }
  float voltage = (sum / 10.0) * 3.3 / 4095.0;
  if (voltage >= CLEAR_VOLTAGE) return 0;
  if (voltage <= DIRTY_VOLTAGE) return MAX_NTU;
  return (CLEAR_VOLTAGE - voltage) / (CLEAR_VOLTAGE - DIRTY_VOLTAGE) * MAX_NTU;
}

float readTDS() {
  int sum = 0;
  for (int i = 0; i < 10; i++) { sum += analogRead(TDS_PIN); delay(10); }
  float voltage = (sum / 10.0) * 3.3 / 4095.0;
  float tds = (133.42 * pow(voltage, 3) - 255.86 * pow(voltage, 2) + 857.39 * voltage) * 0.5;
  return max(tds, 0.0f);
}

float readTemperature() {
  int sum = 0;
  for (int i = 0; i < 10; i++) { sum += analogRead(THERMISTOR_PIN); delay(10); }
  float raw = sum / 10.0;
  return (raw - RAW_30C) * (30.0 - 0.0) / (RAW_0C - RAW_30C) + 30.0;
}

float readSalinity() {
  int sum = 0;
  for (int i = 0; i < 10; i++) { sum += analogRead(SALINITY_PIN); delay(10); }
  float voltage = (sum / 10.0) * SAL_SUPPLY / 4095.0;

  // Voltage divider formula se water resistance nikalo
  // V_out = V_supply * R_water / (R_water + R_fixed)
  // R_water = R_fixed * V_out / (V_supply - V_out)
  if (voltage <= 0.01) return 0.0;  // Open circuit — dry / no water
  if (voltage >= SAL_SUPPLY - 0.05) return 5.0;  // Short circuit — max salt

  float r_water = SAL_RESISTOR * voltage / (SAL_SUPPLY - voltage);

  // Conductivity (mS/cm) — lower resistance = higher conductivity
  // Approximate: EC (mS/cm) = 1000 / R_water (ohms) * cell_constant
  // Cell constant for copper strips approx = 1.0
  float ec = 1000.0 / r_water;

  // EC to salinity (ppt): approx 1 mS/cm = 0.5 ppt for freshwater
  float salinity = ec * 0.5;

  return max(0.0f, min(salinity, 10.0f));  // 0 to 10 ppt range
}

// ── FIREBASE UPLOAD (latest data) ──
void sendToFirebase(float turb, float tds, float temp, float sal, bool drinkable) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(FIREBASE_URL);
  http.addHeader("Content-Type", "application/json");

  String json = "{";
  json += "\"turbidity\":"   + String(turb, 2) + ",";
  json += "\"tds\":"         + String(tds, 1)  + ",";
  json += "\"temperature\":" + String(temp, 1) + ",";
  json += "\"salinity\":"    + String(sal, 2)  + ",";
  json += "\"drinkable\":"   + String(drinkable ? "true" : "false") + ",";
  json += "\"timestamp\":"   + String(millis());
  json += "}";

  int code = http.PUT(json);
  Serial.print("Firebase latest: "); Serial.println(code);
  http.end();
}

// ── HISTORY PUSH ──
void pushToHistory(float turb, float tds, float temp, float sal, bool drinkable) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(FIREBASE_HISTORY);
  http.addHeader("Content-Type", "application/json");

  String json = "{";
  json += "\"turbidity\":"   + String(turb, 2) + ",";
  json += "\"tds\":"         + String(tds, 1)  + ",";
  json += "\"temperature\":" + String(temp, 1) + ",";
  json += "\"salinity\":"    + String(sal, 2)  + ",";
  json += "\"drinkable\":"   + String(drinkable ? "true" : "false") + ",";
  json += "\"timestamp\":"   + String(millis());
  json += "}";

  int code = http.POST(json);
  Serial.print("Firebase history: "); Serial.println(code);
  http.end();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED,   OUTPUT);
  pinMode(BUZZER,    OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED,   LOW);
  digitalWrite(BUZZER,    LOW);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Welcome To");
  lcd.setCursor(0, 1); lcd.print("AquaGuard...");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500); Serial.print("."); tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1); lcd.print(WiFi.localIP());
    delay(2000);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("WiFi Failed!");
    lcd.setCursor(0, 1); lcd.print("Offline mode");
    delay(2000);
  }
}

void loop() {
  float turb = readTurbidity();
  float tds  = readTDS();
  float temp = readTemperature();
  float sal  = readSalinity();

  bool turbOK    = turb <= TURBIDITY_LIMIT;
  bool tdsOK     = tds  <= TDS_LIMIT;
  bool tempOK    = temp >= TEMP_MIN && temp <= TEMP_MAX;
  bool salOK     = sal  <= SALINITY_LIMIT;
  bool drinkable = turbOK && tdsOK && tempOK && salOK;

  // ── PAGE 1: TURBIDITY ──
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Turbidity:"); lcd.print(turb, 1);
  lcd.setCursor(0, 1); lcd.print(turbOK ? "Status: OK      " : "Status: HIGH    ");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("WHO: <4 NTU=OK");
  lcd.setCursor(0, 1); lcd.print(">4 NTU = HIGH");
  delay(2000);

  // ── PAGE 2: TDS ──
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("TDS:"); lcd.print(tds, 0); lcd.print(" ppm");
  lcd.setCursor(0, 1); lcd.print(tdsOK ? "Status: OK      " : "Status: HIGH    ");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("WHO:<190ppm=OK");
  lcd.setCursor(0, 1); lcd.print(">190ppm = HIGH");
  delay(2000);

  // ── PAGE 3: TEMPERATURE ──
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Temp:"); lcd.print(temp, 1); lcd.print(" C");
  lcd.setCursor(0, 1); lcd.print(tempOK ? "Status: OK      " : "Status: HIGH    ");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("WHO: 5-35C=OK");
  lcd.setCursor(0, 1); lcd.print("Out range=HIGH");
  delay(2000);

  // ── PAGE 4: SALINITY ──
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Salinity:"); lcd.print(sal, 2); lcd.print(" ppt");
  lcd.setCursor(0, 1); lcd.print(salOK ? "Status: OK      " : "Status: HIGH    ");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("WHO:<0.5ppt=OK");
  lcd.setCursor(0, 1); lcd.print(">0.5ppt = HIGH");
  delay(2000);

  // ── PAGE 5: HIGH PARAMETERS ──
  if (!turbOK || !tdsOK || !tempOK || !salOK) {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("High Params:");
    String high = "";
    if (!turbOK) high += "Turb ";
    if (!tdsOK)  high += "TDS ";
    if (!tempOK) high += "Temp ";
    if (!salOK)  high += "Sal";
    lcd.setCursor(0, 1); lcd.print(high);
    delay(3000);
  }

  // ── PAGE 6: FINAL RESULT ──
  lcd.clear();
  if (drinkable) {
    lcd.setCursor(0, 0); lcd.print("Water is:");
    lcd.setCursor(0, 1); lcd.print("DRINKABLE  :)");
  } else {
    lcd.setCursor(0, 0); lcd.print("Water is: NOT");
    lcd.setCursor(0, 1); lcd.print("DRINKABLE  :(");
  }
  delay(3000);

  // ── LED + BUZZER ──
  if (drinkable) {
    for (int i = 0; i < 5; i++) {
      digitalWrite(GREEN_LED, HIGH); delay(300);
      digitalWrite(GREEN_LED, LOW);  delay(300);
    }
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER,   LOW);
  } else {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED,   HIGH);
    digitalWrite(BUZZER,    HIGH);
    delay(2000);
    digitalWrite(BUZZER,  LOW);
    digitalWrite(RED_LED, LOW);
  }

  delay(1000);

  // ── FRESH READINGS — Firebase ke liye ──
  float f_turb = readTurbidity();
  float f_tds  = readTDS();
  float f_temp = readTemperature();
  float f_sal  = readSalinity();

  bool f_turbOK    = f_turb <= TURBIDITY_LIMIT;
  bool f_tdsOK     = f_tds  <= TDS_LIMIT;
  bool f_tempOK    = f_temp >= TEMP_MIN && f_temp <= TEMP_MAX;
  bool f_salOK     = f_sal  <= SALINITY_LIMIT;
  bool f_drinkable = f_turbOK && f_tdsOK && f_tempOK && f_salOK;

  sendToFirebase(f_turb, f_tds, f_temp, f_sal, f_drinkable);
  pushToHistory(f_turb, f_tds, f_temp, f_sal, f_drinkable);

  Serial.printf("Sal voltage test → Raw sal: %.2f ppt\n", f_sal);
  Serial.printf("Uploaded → Turb:%.1f TDS:%.0f Temp:%.1f Sal:%.2f Drink:%s\n",
    f_turb, f_tds, f_temp, f_sal, f_drinkable ? "YES" : "NO");
}

/*
 ╔══════════════════════════════════════════════════════════════╗
 ║         AquaGuard ESP32 — Firebase Sensor Upload             ║
 ║                                                              ║
 ║  Yeh code ESP32 se sensor data directly Firebase par         ║
 ║  bhejta hai — website automatically update ho jaati hai      ║
 ║                                                              ║
 ║  Required Libraries (Arduino IDE → Manage Libraries):        ║
 ║  1. Firebase ESP32 Client by mobizt                         ║
 ║     (search: "Firebase ESP32 Client")                        ║
 ║  2. ArduinoJson by Benoit Blanchon                           ║
 ╚══════════════════════════════════════════════════════════════╝
*/

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>

// ══════════════════════════════════════
// ⚙️  APNI SETTINGS YAHAN DAALO
// ══════════════════════════════════════

// WiFi
#define WIFI_SSID     "APNA_WIFI_NAAM"       // Apna WiFi naam
#define WIFI_PASSWORD "APNA_WIFI_PASSWORD"   // Apna WiFi password

// Firebase (console.firebase.google.com se copy karo)
#define FIREBASE_HOST "APNA_PROJECT-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "APNA_DATABASE_SECRET"  
// Database Secret: Firebase Console → Project Settings → 
//                  Service Accounts → Database Secrets → Show

// Sensor Pins (apne hardware ke hisaab se change karo)
#define TDS_PIN      34   // TDS sensor analog pin
#define TURB_PIN     35   // Turbidity sensor analog pin
#define TEMP_PIN     32   // NTC Thermistor analog pin
#define SAL_PIN      33   // Salinity copper strip analog pin
#define PH_PIN       36   // pH sensor analog pin

// Upload interval
#define UPLOAD_EVERY_MS  5000   // Har 5 seconds mein upload

// ══════════════════════════════════════
// Firebase objects
// ══════════════════════════════════════
FirebaseData   fbData;
FirebaseConfig fbConfig;
FirebaseAuth   fbAuth;

// ══════════════════════════════════════
// SENSOR READING FUNCTIONS
// ══════════════════════════════════════

// TDS (Total Dissolved Solids)
float readTDS() {
  int raw = analogRead(TDS_PIN);
  float voltage = raw * 3.3 / 4095.0;
  // EC ka approximate formula
  float ec = (133.42 * pow(voltage, 3) - 255.86 * pow(voltage, 2) + 857.39 * voltage) * 0.5;
  float tds = ec * 0.5;   // EC to TDS conversion factor
  return max(0.0f, tds);
}

// Turbidity (NTU)
float readTurbidity() {
  int raw = analogRead(TURB_PIN);
  float voltage = raw * 3.3 / 4095.0;
  // Approximate NTU conversion (sensor ke datasheet ke hisaab se adjust karo)
  float ntu = -1120.4 * pow(voltage, 2) + 5742.3 * voltage - 4352.9;
  return max(0.0f, ntu);
}

// Temperature (NTC Thermistor — Steinhart-Hart equation)
float readTemperature() {
  int raw = analogRead(TEMP_PIN);
  float resistance = (4095.0 / raw - 1.0) * 10000.0;  // 10k pull-up resistor
  float steinhart;
  steinhart = resistance / 10000.0;          // R/Rnominal
  steinhart = log(steinhart);                // ln(R/Rn)
  steinhart /= 3950.0;                       // B coefficient
  steinhart += 1.0 / (25.0 + 273.15);       // + 1/T0
  steinhart = 1.0 / steinhart;              // invert
  float tempC = steinhart - 273.15;         // Kelvin to Celsius
  return tempC;
}

// Salinity (EC-based copper strip method)
float readSalinity() {
  int raw = analogRead(SAL_PIN);
  float voltage = raw * 3.3 / 4095.0;
  float salinity = voltage * 2.0;  // Basic linear approximation — calibrate as needed
  return max(0.0f, salinity);
}

// pH sensor
float readPH() {
  int raw = analogRead(PH_PIN);
  float voltage = raw * 3.3 / 4095.0;
  // pH 4 = ~2.03V, pH 7 = ~1.5V, pH 10 = ~0.97V (typical pH sensor)
  float ph = 7.0 + ((1.5 - voltage) / 0.18);
  ph = constrain(ph, 0.0, 14.0);
  return ph;
}

// ══════════════════════════════════════
// SETUP
// ══════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n╔══════════════════════════╗");
  Serial.println("║  AquaGuard ESP32 Start   ║");
  Serial.println("╚══════════════════════════╝");

  // WiFi connect
  Serial.printf("\n📶 WiFi se connect ho raha hai: %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;
    if (attempts > 30) {
      Serial.println("\n❌ WiFi connect nahi hua! Restart karo.");
      return;
    }
  }
  
  Serial.printf("\n✅ WiFi Connected! IP: %s\n", WiFi.localIP().toString().c_str());

  // Firebase setup
  fbConfig.host = FIREBASE_HOST;
  fbConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
  
  Firebase.begin(&fbConfig, &fbAuth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("🔥 Firebase se connect ho gaya!");
  Serial.println("📡 Sensor data upload shuru ho raha hai...\n");
}

// ══════════════════════════════════════
// MAIN LOOP
// ══════════════════════════════════════
unsigned long lastUpload = 0;

void loop() {
  unsigned long now = millis();
  
  if (now - lastUpload >= UPLOAD_EVERY_MS) {
    lastUpload = now;
    
    // Sensor readings
    float tds  = readTDS();
    float turb = readTurbidity();
    float temp = readTemperature();
    float sal  = readSalinity();
    float ph   = readPH();
    
    // Serial monitor mein print karo (LCD ke liye bhi yahi use karo)
    Serial.println("──────────────────────────");
    Serial.printf("TDS      : %.0f mg/L\n",  tds);
    Serial.printf("Turbidity: %.1f NTU\n",   turb);
    Serial.printf("Temp     : %.1f °C\n",    temp);
    Serial.printf("Salinity : %.2f ppt\n",   sal);
    Serial.printf("pH       : %.1f\n",       ph);
    
    // Safety check
    bool safe = (tds < 500) && (turb < 4) && (ph > 6.5) && (ph < 8.5) && (temp < 35);
    Serial.printf("Status   : %s\n", safe ? "✅ SAFE" : "⚠️ UNSAFE");
    
    // Firebase pe push karo
    if (Firebase.ready()) {
      FirebaseJson json;
      json.set("tds",       tds);
      json.set("turbidity", turb);
      json.set("temp",      temp);
      json.set("salinity",  sal);
      json.set("ph",        ph);
      json.set("safe",      safe);
      json.set("timestamp", (int)(now / 1000));
      
      // Path: /aquaguard_sensors/latest
      if (Firebase.setJSON(fbData, "/aquaguard_sensors/latest", json)) {
        Serial.println("📡 Firebase: Upload successful ✓");
      } else {
        Serial.printf("❌ Firebase Error: %s\n", fbData.errorReason().c_str());
      }
      
      // History bhi save karo (last 100 readings)
      FirebaseJson histJson;
      histJson.set("tds",       tds);
      histJson.set("turbidity", turb);
      histJson.set("temp",      temp);
      histJson.set("salinity",  sal);
      histJson.set("ph",        ph);
      histJson.set("safe",      safe);
      histJson.set("ts",        (int)(now / 1000));
      Firebase.pushJSON(fbData, "/aquaguard_sensors/history", histJson);
      
    } else {
      Serial.println("⚠️ Firebase ready nahi hai...");
    }
    
    Serial.println();
  }
}

/*
 ══════════════════════════════════════════════════════
  LCD USE KARNA HO TO YEH ADD KARO:
  
  #include <LiquidCrystal_I2C.h>
  LiquidCrystal_I2C lcd(0x27, 16, 2); // ya 20x4 ke liye (0x27, 20, 4)
  
  setup() mein:
    lcd.init();
    lcd.backlight();
  
  loop() mein readings ke baad:
    lcd.clear();
    lcd.setCursor(0,0); lcd.printf("TDS:%.0f pH:%.1f", tds, ph);
    lcd.setCursor(0,1); lcd.printf("T:%.1f Turb:%.1f", temp, turb);
 ══════════════════════════════════════════════════════
*/

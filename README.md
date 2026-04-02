# 💧 AquaGuard — Smart Water Quality Monitoring System

<div align="center">

![AquaGuard Banner](https://img.shields.io/badge/AquaGuard-Smart%20Water%20Monitoring-00e87a?style=for-the-badge&logo=dropbox&logoColor=white)
![ESP32](https://img.shields.io/badge/ESP32-IoT%20Hardware-blue?style=for-the-badge&logo=espressif)
![Firebase](https://img.shields.io/badge/Firebase-Realtime%20DB-orange?style=for-the-badge&logo=firebase)
![Groq AI](https://img.shields.io/badge/Groq-AI%20Prediction-purple?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)

**🏆 1st Prize — Technova 2026 Hackathon | 11th / 200+ Teams — SGU Hackathon 2.0**

*Real-time IoT water quality monitoring with AI-powered disease risk prediction for rural India*

</div>

---

## 📌 Table of Contents

- [About the Project](#about)
- [Features](#features)
- [Tech Stack](#tech-stack)
- [Hardware Setup](#hardware)
- [Project Structure](#structure)
- [Installation & Setup](#installation)
- [How It Works](#how-it-works)
- [Team](#team)

---

## 🌊 About the Project <a name="about"></a>

AquaGuard is a real-time smart water quality monitoring system built for rural India. It uses an **ESP32 IoT device** with 4 sensors to continuously monitor water quality parameters, upload data to **Firebase**, and uses **Groq AI (LLaMA 3.3)** to predict disease outbreak risks like Cholera, Typhoid, Diarrhea, and Dysentery.

Health officials and ASHA workers get instant alerts when water quality deteriorates — helping prevent waterborne disease outbreaks before they happen.

> *"If a family in Bhilwara can know their water is safe before they drink it — that's what technology should do."*
> — Alpha Coders Team, GEC Ajmer

---

## ✨ Features <a name="features"></a>

| Feature | Description |
|--------|-------------|
| 🔬 **5-Parameter Sensing** | TDS, Turbidity, Temperature, Salinity, pH — live readings every 5 seconds |
| 🤖 **AI Disease Prediction** | Groq LLaMA 3.3 predicts Cholera, Typhoid, Diarrhea & Dysentery risk % |
| 📊 **Live Dashboard** | Real-time web dashboard for health officials |
| 🗺️ **Village Alert Map** | Interactive Rajasthan map with red/yellow/green village status |
| 📱 **ASHA Worker Portal** | Field workers submit disease reports from villages |
| 🔔 **SMS Alerts** | Automated SMS to officials when danger threshold crossed |
| 🎙️ **Voice Assistant** | Hindi/Marwadi/English voice chatbot for field workers |
| 💧 **Drinkability Status** | Instant YES/NO verdict based on WHO standards |

---

## 🛠️ Tech Stack <a name="tech-stack"></a>

### Hardware
- **ESP32** — WiFi-enabled microcontroller
- **TDS Sensor** — Total Dissolved Solids (GPIO 34)
- **Turbidity Sensor** — Water clarity (GPIO 35)
- **NTC Thermistor** — Temperature with 10kΩ pull-up (GPIO 32)
- **I2C LCD Display** — 16x2 real-time display (SDA→GPIO21, SCL→GPIO22)

### Software
| Layer | Technology |
|-------|-----------|
| Firmware | Arduino C++ (ESP32) |
| Database | Firebase Realtime Database |
| AI Engine | Python + Groq API (LLaMA 3.3 70B) |
| Frontend | HTML, CSS, JavaScript (Vanilla) |
| Fonts | Bebas Neue, Sora, JetBrains Mono |
| Animations | GSAP, CSS Animations |

---

## 🔌 Hardware Setup <a name="hardware"></a>

### Wiring Diagram

```
ESP32 Pin    →    Sensor/Component
─────────────────────────────────────
GPIO 34      →    TDS Sensor (Signal)
GPIO 35      →    Turbidity Sensor (Signal)
GPIO 32      →    NTC Thermistor + 10kΩ pull-up to 3.3V
GPIO 21      →    LCD SDA (I2C)
GPIO 22      →    LCD SCL (I2C)
3.3V         →    All sensor VCC
GND          →    All sensor GND
```

### LCD I2C Address
- Default: `0x27`
- If display is off, try: `0x3F`

### Thermistor Circuit
```
3.3V ──── 10kΩ ──── GPIO32 ──── NTC ──── GND
```

---

## 📁 Project Structure <a name="structure"></a>

```
AquaGuard/
│
├── 📄 aquaguard_final.html          # Main website (Dashboard + All Pages)
│
├── 🐍 aquaguard_groq_predict.py     # AI prediction engine (Python + Groq)
│
├── ⚡ ESP32_Firebase_LCD_Fixed.ino  # ESP32 Arduino firmware
│
└── 📖 README.md                     # This file
```

---

## 🚀 Installation & Setup <a name="installation"></a>

### 1. ESP32 Firmware Upload



Required Arduino Libraries:
- `FirebaseESP32` by mobizt
- `LiquidCrystal I2C` by Frank de Brabander
- `ArduinoJson` by Benoit Blanchon

### 2. Python AI Engine

```bash
# Install dependencies
pip install groq requests



# Run karo
python aquaguard_groq_predict.py
```

### 3. Website



### Firebase Configuration

```javascript
// aquaguard_final.html mein apni Firebase URL already set hai:
const FIREBASE = "https://your-project.firebaseio.com/sensors.json"

// Database structure:
/sensors          → Live ESP32 sensor data
/ai_prediction    → Groq AI prediction results  
/history          → Sensor history log
/asha_reports     → ASHA worker field reports
/asha_alerts      → SMS alert log
```

---

## ⚙️ How It Works <a name="how-it-works"></a>

```
┌─────────────┐     WiFi      ┌──────────────┐     Fetch     ┌─────────────────┐
│   ESP32     │ ────────────► │   Firebase   │ ◄──────────── │  Python Script  │
│ 4 Sensors   │               │  Realtime DB │               │  (Groq AI)      │
└─────────────┘               └──────────────┘               └─────────────────┘
                                      │                               │
                                      │ ai_prediction                 │ LLaMA 3.3
                                      ▼                               ▼
                               ┌──────────────┐          ┌─────────────────────┐
                               │   Website    │          │  Disease Risk %      │
                               │  Dashboard   │          │  Cholera / Typhoid   │
                               └──────────────┘          │  Diarrhea / Dysentery│
                                                         └─────────────────────┘
```

**Step-by-step:**
1. **ESP32** reads sensors every 5 seconds → uploads to Firebase
2. **Python script** fetches sensor data from Firebase every 10 seconds
3. **Groq AI** (LLaMA 3.3 70B) analyzes data against WHO standards
4. **AI prediction** (disease risk %) uploaded back to Firebase
5. **Website dashboard** reads prediction → updates bars, banner, alerts
6. **ASHA workers** submit field reports → officials get SMS alerts

---

## WHO Safety Standards Used

| Parameter | Safe Limit | Unit |
|-----------|-----------|------|
| TDS | < 500 | mg/L |
| Turbidity | < 4 | NTU |
| Temperature | 10 – 30 | °C |
| Salinity | < 0.5 | ppt |
| pH | 6.5 – 8.5 | — |

---

## 👥 Team — Alpha Coders, GEC Ajmer <a name="team"></a>

| Name | Role |
|------|------|
| **Lucky Gupta** | Team Lead, Full-Stack & IoT |
| **Harish Kumar** | Frontend Developer |
| **Jiya Sapnani** | AI/ML & UI Designer |
| **Suchita Kumari** | Backend & Database |

**College:** Government Engineering College, Ajmer, Rajasthan

---

## 🏆 Achievements

- 🥇 **1st Prize** — Technova 2026 Hackathon (Live hardware demo)
- 🚀 **11th / 200+ Teams** — SGU Hackathon 2.0 (National Level)

---

## 📄 License

This project is licensed under the MIT License.

---

<div align="center">

**Built with ❤️ for India's rural communities**

*AquaGuard — Because clean water is a right, not a privilege*

</div>

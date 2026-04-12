"""
╔══════════════════════════════════════════════════════════╗
║   AquaGuard — AI Disease Risk Prediction (Groq API)      ║
║   Firebase → Groq AI → Prediction → Firebase            ║
║                                                          ║
║   Install:                                               ║
║     pip install groq requests                            ║
║                                                          ║
║   Run:                                                   ║
║     python aquaguard_groq_predict.py                     ║
╚══════════════════════════════════════════════════════════╝
"""

import requests
import json
import time
from datetime import datetime
from groq import Groq

# ══════════════════════════════════════════
# ⚙️  APNI SETTINGS YAHAN DAALO
# ══════════════════════════════════════════

GROQ_API_KEY = "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"   # ← Yahan apni key daalo

FIREBASE_BASE  = "https://aqua-82fea-default-rtdb.firebaseio.com"
SENSOR_URL     = f"{FIREBASE_BASE}/sensors.json"
PREDICT_URL    = f"{FIREBASE_BASE}/ai_prediction.json"
HISTORY_URL    = f"{FIREBASE_BASE}/history.json"
ASHA_URL       = f"{FIREBASE_BASE}/asha_reports.json"   # ← ASHA reports node

PREDICT_EVERY  = 5    # Har 5 seconds mein predict kare (faster!)

# ══════════════════════════════════════════
# STEP 1: Firebase se sensor data fetch
# ══════════════════════════════════════════
def fetch_sensors():
    # Dono URLs try karo
    urls = [
        f"{FIREBASE_BASE}/sensors.json",
        f"{FIREBASE_BASE}/sensors.json",
    ]

    for url in urls:
        try:
            print(f"  🔍 Trying: {url}")
            r = requests.get(url, timeout=3)
            d = r.json()
            print(f"  📦 Raw response: {str(d)[:200]}")  # Debug ke liye

            if not d:
                print(f"  ⚠️  Empty response from {url}")
                continue

            # Keys dono formats mein try karo (temp ya temperature)
            sensors = {
                "tds":         float(d.get("tds",         0)),
                "turbidity":   float(d.get("turbidity",   0)),
                "temperature": float(d.get("temperature", d.get("temp", 25))),
                "salinity":    float(d.get("salinity",    0)),
                "ph":          float(d.get("ph",          7.0)),
            }
            print(f"\n📡 Sensors: TDS={sensors['tds']:.0f} Turb={sensors['turbidity']:.1f} "
                  f"Temp={sensors['temperature']:.1f} Sal={sensors['salinity']:.2f} pH={sensors['ph']:.1f}")
            return sensors

        except Exception as e:
            print(f"  ❌ Error on {url}: {e}")
            continue

    print("⚠️  Koi bhi Firebase URL se data nahi mila")
    return None

# ══════════════════════════════════════════
# STEP 2: Rule-based flags (fast check)
# ══════════════════════════════════════════
def pre_analyze(s):
    flags = []
    score = 0

    if s["tds"] > 500:
        flags.append(f"HIGH TDS: {s['tds']:.0f} mg/L (limit 500)")
        score += min((s["tds"] - 500) / 250 * 30, 30)

    if s["turbidity"] > 4:
        flags.append(f"HIGH TURBIDITY: {s['turbidity']:.1f} NTU (limit 4)")
        score += min((s["turbidity"] - 4) / 4 * 35, 35)

    if s["temperature"] > 30:
        flags.append(f"HIGH TEMP: {s['temperature']:.1f}°C (limit 30)")
        score += min((s["temperature"] - 30) / 10 * 20, 20)

    if s["salinity"] > 0.5:
        flags.append(f"HIGH SALINITY: {s['salinity']:.2f} ppt (limit 0.5)")
        score += min((s["salinity"] - 0.5) / 1.0 * 15, 15)

    if s["ph"] < 6.5:
        flags.append(f"LOW pH: {s['ph']:.1f} (min 6.5)")
        score += min((6.5 - s["ph"]) / 1.5 * 25, 25)
    elif s["ph"] > 8.5:
        flags.append(f"HIGH pH: {s['ph']:.1f} (max 8.5)")
        score += min((s["ph"] - 8.5) / 1.5 * 20, 20)

    # Combined risk factors
    if s["turbidity"] > 4 and s["temperature"] > 28:
        flags.append("COMBINED: High turbidity + warm water = Cholera risk")
        score += 15

    if s["tds"] > 500 and s["ph"] < 6.8:
        flags.append("COMBINED: High TDS + acidic = Typhoid risk")
        score += 15

    if s["turbidity"] > 6 and s["tds"] > 400:
        flags.append("COMBINED: Turbid + high TDS = Bacterial load")
        score += 10

    return flags, min(score, 100)

# ══════════════════════════════════════════
# STEP 3: Groq AI se prediction lo
# ══════════════════════════════════════════
def get_groq_prediction(sensors, flags, score):
    client = Groq(api_key=GROQ_API_KEY)

    flags_text = "\n".join(f"  - {f}" for f in flags) if flags else "  - No violations"

    prompt = f"""AquaGuard AI. Sensors: TDS={sensors['tds']:.0f}mg/L Turbidity={sensors['turbidity']:.1f}NTU Temp={sensors['temperature']:.0f}C Salinity={sensors['salinity']:.2f}ppt pH={sensors['ph']:.1f}
Score:{score:.0f}/100 Flags:{len(flags)}
JSON only, no text:
{{"cholera_risk":<0-100>,"typhoid_risk":<0-100>,"diarrhea_risk":<0-100>,"dysentery_risk":<0-100>,"overall_contamination":<0-100>,"drinkable":<true/false>,"severity":"<safe|low|medium|high|critical>","primary_concern":"<10 words max>","action":"<10 words max>"}}"""

    response = client.chat.completions.create(
        model="llama-3.3-70b-versatile",             # Fastest Groq model — ~0.3s response!
        messages=[{"role": "user", "content": prompt}],
        max_tokens=180,  # Kam tokens = faster response
        temperature=0.1,  # Low temp = deterministic + faster
    )

    text = response.choices[0].message.content.strip()
    print(f"\n🤖 Groq AI response:\n{text}")

    # JSON extract karo (agar extra text ho to)
    start = text.find("{")
    end   = text.rfind("}") + 1
    clean = text[start:end]

    return json.loads(clean)

# ══════════════════════════════════════════
# STEP 4: Firebase pe upload karo
# ══════════════════════════════════════════
def upload(sensors, pred):
    payload = {
        "cholera":   pred.get("cholera_risk",        0),
        "typhoid":   pred.get("typhoid_risk",        0),
        "diarrhea":  pred.get("diarrhea_risk",       0),
        "dysentery": pred.get("dysentery_risk",      0),
        "overall":   pred.get("overall_contamination", 0),
        "drinkable": pred.get("drinkable",           True),
        "severity":  pred.get("severity",            "safe"),
        "concern":   pred.get("primary_concern",     ""),
        "action":    pred.get("action",              ""),
        "timestamp": datetime.now().isoformat(),
        "sensors":   sensors,
    }

    # Latest prediction
    r1 = requests.put(PREDICT_URL, json=payload, timeout=3)
    print(f"✅ Firebase upload: {r1.status_code}")

    # History
    requests.post(HISTORY_URL, json=payload, timeout=3)

    return payload

# ══════════════════════════════════════════
# STEP 4b: ASHA Report Firebase pe push karo
# ══════════════════════════════════════════
def push_asha_report(worker_name, village, district, total, cholera, typhoid, diarrhea, severity="safe", notes=""):
    """
    ASHA worker ka field report Firebase ke /asha_reports node pe push karo.
    Frontend (HTML dashboard) yahan se live fetch karta hai har 10s mein.

    Usage:
        push_asha_report("Sunita Devi", "Nasirabad", "Ajmer", 8, 6, 1, 1, "critical")
    """
    report = {
        "name":         worker_name,
        "village":      village,
        "district":     district,
        "total":        total,
        "cholera":      cholera,
        "typhoid":      typhoid,
        "diarrhea":     diarrhea,
        "severity":     severity,   # "safe" | "warning" | "critical"
        "notes":        notes,
        "timestamp":    datetime.now().isoformat(),
    }
    try:
        r = requests.post(ASHA_URL, json=report, timeout=3)  # POST = Firebase push (auto key)
        print(f"  👩‍⚕️ ASHA report pushed [{r.status_code}]: {worker_name} — {village} — {severity.upper()}")
        return r.status_code == 200
    except Exception as e:
        print(f"  ❌ ASHA push failed: {e}")
        return False

def seed_asha_reports():
    """
    Pehli baar run karo toh sample ASHA reports Firebase pe daal do.
    Ek baar hi chalao — dashboard pe turant cards dikhne lagenge.
    """
    print("\n📋 Seeding ASHA reports to Firebase...")
    reports = [
        ("Sunita Devi",   "Nasirabad",  "Ajmer", 8, 6, 1, 1, "critical", "High turbidity + Cholera outbreak"),
        ("Kavita Sharma", "Beawar",     "Ajmer", 5, 0, 4, 1, "warning",  "Typhoid cases rising — TDS high"),
        ("Rekha Meena",   "Mangliawas", "Ajmer", 3, 0, 1, 2, "warning",  "Diarrhea cluster — monitor"),
        ("Meera Gurjar",  "Kishangarh", "Ajmer", 0, 0, 0, 0, "safe",     "All clear — routine visit"),
        ("Priya Kumawat", "Kekri",      "Ajmer", 1, 0, 1, 0, "safe",     "1 typhoid case — isolated"),
        ("Anita Jain",    "Sarwar",     "Ajmer", 0, 0, 0, 0, "safe",     "Water quality good"),
    ]
    for r in reports:
        push_asha_report(*r)
        time.sleep(0.3)
    print("✅ ASHA reports seeded!\n")

# ══════════════════════════════════════════
# STEP 5: Console print
# ══════════════════════════════════════════
def print_result(p):
    icons = {"safe":"✅","low":"🟡","medium":"🟠","high":"🔴","critical":"🚨"}
    sev   = p.get("severity","safe")
    print(f"\n{'═'*50}")
    print(f"  {icons.get(sev,'❓')}  AquaGuard AI Result — {sev.upper()}")
    print(f"{'═'*50}")
    print(f"  Cholera:    {p['cholera']}%")
    print(f"  Typhoid:    {p['typhoid']}%")
    print(f"  Diarrhea:   {p['diarrhea']}%")
    print(f"  Dysentery:  {p['dysentery']}%")
    print(f"  Overall:    {p['overall']}% contamination")
    print(f"  Drinkable:  {'YES ✅' if p['drinkable'] else 'NO ❌'}")
    print(f"  Concern:    {p['concern']}")
    print(f"  Action:     {p['action']}")
    print(f"{'═'*50}")

# ══════════════════════════════════════════
# MAIN LOOP
# ══════════════════════════════════════════
def main():
    print("\n╔══════════════════════════════════════════╗")
    print("║   AquaGuard Groq AI Engine — Started     ║")
    print(f"║   Har {PREDICT_EVERY}s mein predict karega            ║")
    print("╚══════════════════════════════════════════╝")

    # Pehli baar ASHA sample reports seed karo (sirf ek baar)
    seed_asha_reports()

    cycle = 0
    while True:
        cycle += 1
        print(f"\n🔄 Cycle #{cycle} — {datetime.now().strftime('%H:%M:%S')}")

        sensors = fetch_sensors()
        if not sensors:
            time.sleep(PREDICT_EVERY)
            continue

        flags, score = pre_analyze(sensors)
        print(f"📊 Pre-score: {score:.0f}/100  |  Flags: {len(flags)}")

        try:
            pred    = get_groq_prediction(sensors, flags, score)
            payload = upload(sensors, pred)
            print_result(payload)
        except json.JSONDecodeError as e:
            print(f"❌ JSON error: {e}")
        except Exception as e:
            print(f"❌ Groq error: {e}")

        print(f"⏳ Next in {PREDICT_EVERY}s...")
        time.sleep(PREDICT_EVERY)

if __name__ == "__main__":
    main()

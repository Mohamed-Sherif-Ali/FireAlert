# 🔥 Fire Detection & Alert System

**ESP32-Based Fire Monitoring with Real-Time WhatsApp Notifications**

An IoT fire detection system that monitors temperature and flame conditions, providing instant WhatsApp alerts via the CallMeBot API. Features include visual/audio alarms, LCD display, and user acknowledgment capabilities.

---

## ✨ Features

### 🔥 **Fire Detection**
- **Dual Detection**: Flame sensor + temperature threshold (>60°C)
- **Real-Time Monitoring**: Continuous 500ms sensor polling
- **Instant Alerts**: Immediate activation on fire detection

### 📱 **WhatsApp Notifications**
- **CallMeBot Integration**: Free WhatsApp API service
- **Smart Alerts**: One message per fire event (prevents spam)
- **WiFi Auto-Reconnect**: Ensures alert delivery

### 🚨 **Alarm System**
- **Visual Indicator**: LED activation on fire detection
- **Audio Alert**: Buzzer activation
- **User Acknowledgment**: Push button to silence alarm
- **Auto-Shutoff**: 60-second timeout prevents endless alarms

### 📊 **Display & Monitoring**
- **16x2 LCD**: Real-time temperature and flame status
- **Serial Monitor**: Detailed system logs
- **WiFi Status**: Connection feedback

---

## 🛠️ Hardware Components

| Component | Quantity | Purpose |
|-----------|----------|---------|
| **ESP32 Dev Board** | 1 | Main microcontroller with WiFi |
| **LM35 Temperature Sensor** | 1 | Analog temperature measurement |
| **Flame Sensor Module** | 1 | IR-based flame detection |
| **16x2 I2C LCD** | 1 | Status display |
| **Active Buzzer** | 1 | Audio alarm |
| **LED** | 1 | Visual indicator |
| **Push Button** | 1 | Alarm acknowledgment |
| **220Ω Resistor** | 1 | For LED (if needed) |
| **Jumper Wires** | ~20 | Connections |
| **Breadboard** | 1 | Prototyping |

---

## 📐 Circuit Diagram

### Pin Connections

| Component | Pin | ESP32 GPIO | Notes |
|-----------|-----|------------|-------|
| **Flame Sensor** | VCC | 3.3V | Power |
| | GND | GND | Ground |
| | D0 | GPIO 15 | Digital output (LOW = flame) |
| **LM35 Temp Sensor** | VCC | 3.3V | Power (can use 5V) |
| | OUT | GPIO 34 | Analog output |
| | GND | GND | Ground |
| **LCD (I2C)** | VCC | 5V | Power |
| | GND | GND | Ground |
| | SDA | GPIO 21 | I2C data |
| | SCL | GPIO 22 | I2C clock |
| **Buzzer** | + | GPIO 18 | Positive terminal |
| | - | GND | Negative terminal |
| **LED** | Anode (+) | GPIO 4 | Through 220Ω resistor |
| | Cathode (-) | GND | Ground |
| **Button** | One side | GPIO 14 | Pull-up enabled |
| | Other side | GND | Pressed = LOW |

### Schematic

```
ESP32                    Sensors/Outputs
━━━━━                    ━━━━━━━━━━━━━━━

GPIO 15 ──────────────> Flame Sensor (D0)
GPIO 34 ──────────────> LM35 (OUT)
GPIO 21 (SDA) ────────> LCD (SDA)
GPIO 22 (SCL) ────────> LCD (SCL)
GPIO 18 ──────────────> Buzzer (+)
GPIO 4  ───[220Ω]────> LED (Anode)
GPIO 14 ──────/───────> Button
              │
             GND
```

---

## 📦 Software Requirements

### Arduino IDE Setup

1. **Install ESP32 Board Support:**
   ```
   File → Preferences → Additional Board Manager URLs
   Add: https://dl.espressif.com/dl/package_esp32_index.json
   
   Tools → Board → Boards Manager → Search "esp32" → Install
   ```

2. **Install Required Libraries:**
   ```
   Sketch → Include Library → Manage Libraries
   
   Search and install:
   - LiquidCrystal I2C (by Frank de Brabander)
   - UrlEncode (by Masayuki Sugahara)
   ```

3. **Board Configuration:**
   ```
   Tools → Board → ESP32 Arduino → ESP32 Dev Module
   Tools → Upload Speed → 115200
   Tools → Flash Frequency → 80MHz
   ```

---

## 🔧 Installation & Setup

### Step 1: Hardware Assembly

1. **Connect Components** according to pin diagram above
2. **Double-check connections** (especially power/ground)
3. **Verify I2C LCD address** (usually 0x27 or 0x3F)
   - Run I2C scanner sketch if needed

### Step 2: CallMeBot WhatsApp Setup

1. **Add CallMeBot to WhatsApp:**
   - Save `+34 621 073 059` as a contact
   - Name it "CallMeBot" or similar

2. **Get API Key:**
   - Send this exact message to CallMeBot via WhatsApp:
     ```
     I allow callmebot to send me messages
     ```
   - You'll receive a reply with your API key
   - **Save this key!** You'll need it in the code

3. **Format Phone Number:**
   - Use international format WITHOUT + or spaces
   - Example: +1 234-567-8900 → `1234567890`

### Step 3: Configure Code

Open `fire_alert_system.ino` and update these lines:

```cpp
// WiFi Credentials (line ~35)
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// CallMeBot Configuration (line ~47)
String PHONE_NUMBER = "1234567890";          // Your phone number
String API_KEY = "YOUR_API_KEY_HERE";        // Your CallMeBot API key
```

### Step 4: Upload Code

1. Connect ESP32 via USB
2. Select correct COM port: `Tools → Port`
3. Click Upload button (or Ctrl+U)
4. Wait for "Done uploading"
5. Open Serial Monitor: `Tools → Serial Monitor`
6. Set baud rate: `115200`

---

## 🚀 Usage

### Normal Operation

**LCD Display Shows:**
```
Temp: 25.3 C
Flame: NO
```

**Serial Monitor Shows:**
```
Temperature: 25.3 °C
```

### Fire Detection

**When flame detected OR temperature >60°C:**

1. **Alarm Activates:**
   - LED turns ON
   - Buzzer sounds
   - LCD shows: `!! FIRE ALERT !!`

2. **WhatsApp Message Sent:**
   ```
   🔥 FIRE ALERT! Flame detected. Take immediate action!
   ```
   or
   ```
   🌡️ HIGH TEMPERATURE ALERT! Temperature: 65.2°C. Possible fire!
   ```

3. **Acknowledgment:**
   - Press button to silence alarm
   - OR wait 60 seconds for auto-shutoff

4. **Reset:**
   - System returns to monitoring mode
   - New alerts can be sent if fire persists

---

## ⚙️ Configuration Options

### Temperature Threshold

Change fire detection temperature:
```cpp
const float TEMP_THRESHOLD = 60.0;  // Default: 60°C
```

**Recommended values:**
- **Kitchen**: 70-80°C
- **Living room**: 50-60°C
- **Server room**: 40-50°C

### Alarm Timeout

Change auto-shutoff duration:
```cpp
const unsigned long ALARM_TIMEOUT = 60000;  // Default: 60 seconds
```

### LCD I2C Address

If LCD doesn't work, try different address:
```cpp
LiquidCrystal_I2C lcd(0x3F, 16, 2);  // Try 0x3F instead of 0x27
```

**Find your LCD address:**
```cpp
// Run I2C Scanner sketch to find address
```

---

## 🐛 Troubleshooting

### WiFi Won't Connect

**Symptoms:** LCD shows "WiFi Failed!"

**Solutions:**
1. Check SSID and password (case-sensitive!)
2. Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
3. Move ESP32 closer to router
4. Check router firewall settings

### WhatsApp Message Not Sending

**Symptoms:** Serial shows error code

**Solutions:**

| Error Code | Meaning | Solution |
|------------|---------|----------|
| **-1** | Connection failed | Check WiFi connection |
| **401** | Unauthorized | Verify API key is correct |
| **403** | Forbidden | Re-register with CallMeBot |
| **500** | Server error | Wait a few minutes, try again |

**Also check:**
- Phone number format (no + or spaces)
- API key copied correctly (no extra spaces)
- CallMeBot rate limit (max ~1 msg/minute)

### LCD Shows Nothing

**Solutions:**
1. Check I2C connections (SDA/SCL swapped?)
2. Try different I2C address (0x27 or 0x3F)
3. Adjust potentiometer on LCD backpack (contrast)
4. Run I2C scanner to detect address

### Flame Sensor Always Triggered

**Solutions:**
1. Adjust sensitivity potentiometer on sensor
2. Check wiring (should be active LOW)
3. Shield sensor from ambient light
4. Verify sensor is not damaged

### Temperature Reading Wrong

**Solutions:**
1. Check LM35 wiring (VCC, OUT, GND correct?)
2. Ensure using 3.3V or 5V (not both)
3. Verify sensor orientation (flat side faces you)
4. Calculate expected reading:
   ```
   Room temp ~25°C → Should read 23-27°C
   Hand on sensor → Should jump to 30-35°C
   ```

### Button Doesn't Work

**Solutions:**
1. Check connections (one side to GPIO14, other to GND)
2. Code uses INPUT_PULLUP (no external resistor needed)
3. Try different button or check for damage

---

## 📊 System Specifications

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Temperature Range** | 0-100°C | LM35 sensor limit |
| **Temperature Accuracy** | ±0.5°C | At 25°C |
| **Flame Detection Range** | 0-100cm | Typical IR sensor |
| **Flame Detection Angle** | ~60° | Depends on sensor model |
| **Update Rate** | 500ms | Temperature readings |
| **WiFi Protocol** | 802.11 b/g/n | 2.4GHz only |
| **Operating Voltage** | 3.3V / 5V | Mixed logic levels |
| **Power Consumption** | ~200mA | With WiFi active |
| **Alert Latency** | 1-3 seconds | From detection to WhatsApp |

---

## 🔒 Safety Considerations

### ⚠️ Important Warnings

- **This is a PROTOTYPE** - Not certified for life-safety applications
- **Do NOT rely solely** on this system for fire protection
- **Install proper smoke detectors** as primary safety devices
- **Test regularly** to ensure proper operation
- **Keep fire extinguisher** accessible
- **Have evacuation plan** in place

### Best Practices

✅ **DO:**
- Test system weekly
- Keep sensors clean and unobstructed
- Use in conjunction with certified smoke alarms
- Ensure WiFi is stable and reliable
- Have backup power (battery) for ESP32
- Mount sensors properly (ceiling-mounted preferred)

❌ **DON'T:**
- Use as sole fire detection method
- Install in extreme environments (>50°C ambient)
- Expose sensors to direct sunlight
- Ignore regular maintenance
- Use damaged sensors
- Block sensor field of view

---

## 🔄 System Flowchart

```
┌─────────────────────────────────────────────────────────┐
│                    SYSTEM START                         │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼
         ┌─────────────────────────┐
         │  Initialize Hardware    │
         │  - GPIO pins            │
         │  - LCD display          │
         │  - Connect WiFi         │
         └────────────┬────────────┘
                      │
                      ▼
         ┌─────────────────────────┐
         │   MONITORING LOOP       │
         │   (Every 500ms)         │
         └────────────┬────────────┘
                      │
          ┌───────────┴───────────┐
          │                       │
          ▼                       ▼
    ┌─────────┐           ┌─────────────┐
    │ Read    │           │ Check Flame │
    │ Temp    │           │ Sensor      │
    └────┬────┘           └──────┬──────┘
         │                       │
         └───────────┬───────────┘
                     │
                     ▼
            ┌────────────────┐
            │ Fire Detected? │
            └───┬────────┬───┘
                │        │
           NO   │        │   YES
                │        │
                │        ▼
                │  ┌──────────────┐
                │  │ Activate:    │
                │  │ - LED        │
                │  │ - Buzzer     │
                │  │ - WhatsApp   │
                │  └──────┬───────┘
                │         │
                │         ▼
                │  ┌──────────────┐
                │  │ Wait for:    │
                │  │ - Button OR  │
                │  │ - 60s timeout│
                │  └──────┬───────┘
                │         │
                │         ▼
                │  ┌──────────────┐
                │  │ Deactivate   │
                │  │ Alarm        │
                │  └──────┬───────┘
                │         │
                └─────────┴───────────┐
                                      │
                     ┌────────────────┘
                     │
                     ▼
            ┌────────────────┐
            │ Update LCD     │
            └────────┬───────┘
                     │
                     ▼
            ┌────────────────┐
            │ WiFi OK?       │
            └───┬────────┬───┘
                │        │
           YES  │        │   NO
                │        │
                │        ▼
                │  ┌──────────────┐
                │  │ Reconnect    │
                │  │ WiFi         │
                │  └──────┬───────┘
                │         │
                └─────────┴───────────┐
                                      │
                     ┌────────────────┘
                     │
                     ▼
              (Loop continues)
```

---

## 📈 Future Enhancements

### Planned Features
- [ ] Multiple sensor support (different rooms)
- [ ] Data logging to SD card
- [ ] Web dashboard for monitoring
- [ ] Battery backup with low-battery alert
- [ ] Email notifications (in addition to WhatsApp)
- [ ] MQTT integration for smart home
- [ ] Mobile app for configuration
- [ ] Siren/strobe light support
- [ ] Fire department auto-dial (via GSM module)
- [ ] Machine learning for false positive reduction

---

## 🎓 Learning Outcomes

This project demonstrates:
- **IoT Integration**: ESP32 WiFi capabilities
- **Sensor Interfacing**: Analog (LM35) and digital (flame sensor)
- **API Communication**: HTTP GET requests
- **Display Control**: I2C LCD communication
- **Interrupt Handling**: Button input with debouncing
- **State Management**: Alarm activation/deactivation logic
- **Error Handling**: WiFi reconnection, timeout handling
- **User Experience**: Visual/audio feedback, acknowledgment

---

## 📄 License

MIT License - See LICENSE file for details

---

## 👤 Author

**Mohamed Sherif Ali**  
 AI & Computer VisionEngineer
---

## 🙏 Acknowledgments

- **CallMeBot** - Free WhatsApp API service
- **ESP32 Community** - Excellent documentation and libraries
- **Arduino** - Development environment

---

## 📞 Support

**Issues or Questions?**
- Open an issue on GitHub
- Check troubleshooting section above
- Test with Serial Monitor for debugging

---

## ⚠️ Disclaimer

This fire detection system is a DIY project for educational and experimental purposes. It is NOT a replacement for certified fire safety equipment. For life-safety applications, always use UL-listed or equivalent certified smoke detectors and fire alarm systems. The author is not responsible for any damages or injuries resulting from the use of this system.

**Always follow local fire safety codes and regulations.**

---

**🔥 Stay Safe! Test Your System Regularly! 🔥**

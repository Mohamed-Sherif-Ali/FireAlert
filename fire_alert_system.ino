/*
 * ═══════════════════════════════════════════════════════════════════════════
 * Fire Detection & Alert System with WhatsApp Notifications
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * Monitors temperature and flame conditions using ESP32 with real-time alerts
 * 
 * Hardware:
 *   - ESP32 Development Board
 *   - LM35 Temperature Sensor (analog)
 *   - Flame Sensor Module (digital)
 *   - Buzzer (active or passive)
 *   - LED indicator
 *   - Push button (acknowledgment)
 *   - I2C 16x2 LCD Display
 * 
 * Features:
 *   - Real-time temperature monitoring on LCD
 *   - Flame detection with visual/audio alarm
 *   - WhatsApp notifications via CallMeBot API
 *   - Temperature threshold alerts (>60°C)
 *   - User acknowledgment via button
 *   - Auto-retry on WiFi disconnection
 *   - One alert per fire event (prevents spam)
 * 
 * Author: Mohamed Sherif Ali
 * License: MIT
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <UrlEncode.h>

// ═══════════════════════════════════════════════════════════════════════════
// PIN DEFINITIONS
// ═══════════════════════════════════════════════════════════════════════════

#define BUTTON_PIN  14    // Push button for alert acknowledgment (INPUT_PULLUP)
#define BUZZER_PIN  18    // Buzzer for audio alarm
#define LED_PIN     4     // LED indicator for visual alarm
#define FLAME_PIN   15    // Flame sensor digital output (LOW = flame detected)
#define TEMP_PIN    34    // LM35 temperature sensor analog input

// ═══════════════════════════════════════════════════════════════════════════
// WIFI CREDENTIALS
// ═══════════════════════════════════════════════════════════════════════════
// Replace with your network credentials

const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ═══════════════════════════════════════════════════════════════════════════
// CALLMEBOT WHATSAPP API CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════
// Setup Instructions:
// 1. Save +34 621073059 to WhatsApp contacts as "CallMeBot"
// 2. Send "I allow callmebot to send me messages" to this number
// 3. You'll receive your API key - paste it below
// 4. Format phone number with country code (e.g., +1234567890 → 1234567890)

String PHONE_NUMBER = "1234567890";          // Your phone number (no + or spaces)
String API_KEY = "YOUR_API_KEY_HERE";        // Your CallMeBot API key

// ═══════════════════════════════════════════════════════════════════════════
// SYSTEM CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════

// Temperature threshold for fire alert (Celsius)
const float TEMP_THRESHOLD = 60.0;           // Alert when temp exceeds 60°C

// Timing parameters
const unsigned long TEMP_READ_INTERVAL = 500;      // Read temperature every 500ms
const unsigned long ALARM_TIMEOUT = 60000;         // Auto-shutoff alarm after 60 seconds
const unsigned long WIFI_RETRY_INTERVAL = 5000;   // Retry WiFi every 5 seconds if disconnected
const int BUTTON_DEBOUNCE_MS = 50;                 // Button debounce delay

// WiFi connection timeout
const unsigned long WIFI_CONNECT_TIMEOUT = 20000;  // 20 seconds max to connect

// ═══════════════════════════════════════════════════════════════════════════
// SYSTEM STATE VARIABLES
// ═══════════════════════════════════════════════════════════════════════════

bool fireAlertSent = false;           // Tracks if WhatsApp message already sent
bool alarmActive = false;             // Tracks if alarm is currently active
unsigned long alarmStartTime = 0;     // Timestamp when alarm started
unsigned long lastTempRead = 0;       // Timestamp of last temperature reading
unsigned long lastWiFiCheck = 0;      // Timestamp of last WiFi status check

// ═══════════════════════════════════════════════════════════════════════════
// LCD DISPLAY OBJECT
// ═══════════════════════════════════════════════════════════════════════════
// I2C address 0x27 is common, but some displays use 0x3F
// If LCD doesn't work, try changing to 0x3F

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Address 0x27, 16 columns, 2 rows

// ═══════════════════════════════════════════════════════════════════════════
// FUNCTION: SEND WHATSAPP MESSAGE
// ═══════════════════════════════════════════════════════════════════════════

void sendWhatsAppAlert(String message) {
  /*
   * Sends WhatsApp message using CallMeBot API
   * 
   * Process:
   * 1. Build API URL with phone number, API key, and message
   * 2. URL-encode message to handle special characters
   * 3. Send HTTP GET request
   * 4. Check response code (200 = success)
   * 
   * Args:
   *   message: Text message to send
   * 
   * Note: CallMeBot has rate limits - max ~1 message per minute per user
   */
  
  // Check WiFi connection before attempting to send
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ Cannot send WhatsApp: WiFi not connected");
    return;
  }
  
  // Build API URL
  // Format: https://api.callmebot.com/whatsapp.php?phone=XXXX&apikey=XXXX&text=MESSAGE
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + PHONE_NUMBER + 
               "&apikey=" + API_KEY + 
               "&text=" + urlEncode(message);
  
  Serial.println("📤 Sending WhatsApp message...");
  
  // Initialize HTTP client
  HTTPClient http;
  http.begin(url);
  
  // Send GET request (CallMeBot uses GET, not POST)
  int httpResponseCode = http.GET();
  
  // Check response
  if (httpResponseCode == 200) {
    Serial.println("✅ WhatsApp message sent successfully!");
  } else {
    Serial.print("❌ Failed to send message. HTTP code: ");
    Serial.println(httpResponseCode);
    
    // Print response body for debugging
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response: " + response);
    }
  }
  
  // Clean up
  http.end();
}

// ═══════════════════════════════════════════════════════════════════════════
// FUNCTION: CONNECT TO WIFI
// ═══════════════════════════════════════════════════════════════════════════

bool connectToWiFi() {
  /*
   * Attempts to connect to WiFi with timeout
   * 
   * Returns:
   *   true if connected successfully
   *   false if connection failed or timed out
   */
  
  Serial.println("\n🌐 Connecting to WiFi...");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);
  
  // Start connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Wait for connection with timeout
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && 
         millis() - startAttempt < WIFI_CONNECT_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }
  
  // Check if connected
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(2000);  // Show IP for 2 seconds
    
    return true;
  } else {
    Serial.println("\n❌ WiFi connection failed!");
    
    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
    lcd.setCursor(0, 1);
    lcd.print("Check Settings");
    
    return false;
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// FUNCTION: READ TEMPERATURE
// ═══════════════════════════════════════════════════════════════════════════

float readTemperature() {
  /*
   * Reads temperature from LM35 sensor
   * 
   * LM35 Conversion:
   * - ESP32 ADC: 12-bit (0-4095)
   * - Voltage range: 0-3.3V
   * - LM35 output: 10mV per °C
   * 
   * Formula:
   * 1. Convert ADC to voltage: voltage = (ADC / 4095) × 3.3V
   * 2. Convert voltage to temp: temp = voltage × 100
   * 
   * Returns:
   *   Temperature in Celsius
   */
  
  // Read analog value (0-4095 for 12-bit ADC)
  int rawValue = analogRead(TEMP_PIN);
  
  // Convert to voltage (ESP32 ADC reference is 3.3V)
  float voltage = rawValue * (3.3 / 4095.0);
  
  // Convert voltage to temperature
  // LM35 outputs 10mV per degree Celsius
  // So 1V = 100°C → multiply voltage by 100
  float temperatureC = voltage * 100.0;
  
  return temperatureC;
}

// ═══════════════════════════════════════════════════════════════════════════
// FUNCTION: ACTIVATE ALARM
// ═══════════════════════════════════════════════════════════════════════════

void activateAlarm(String alertMessage) {
  /*
   * Activates visual and audio alarm, sends WhatsApp alert
   * 
   * Args:
   *   alertMessage: Message to send via WhatsApp
   */
  
  if (!alarmActive) {
    // Turn on alarm outputs
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    
    // Record alarm start time
    alarmStartTime = millis();
    alarmActive = true;
    
    Serial.println("🚨 ALARM ACTIVATED!");
    
    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("!! FIRE ALERT !!");
    lcd.setCursor(0, 1);
    lcd.print("Press Button");
    
    // Send WhatsApp alert (only once per event)
    if (!fireAlertSent) {
      sendWhatsAppAlert(alertMessage);
      fireAlertSent = true;
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// FUNCTION: DEACTIVATE ALARM
// ═══════════════════════════════════════════════════════════════════════════

void deactivateAlarm() {
  /*
   * Turns off alarm outputs and resets state
   */
  
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  alarmActive = false;
  
  Serial.println("✅ Alarm deactivated");
  
  // Update LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Alarm Reset");
  delay(1000);
}

// ═══════════════════════════════════════════════════════════════════════════
// FUNCTION: CHECK BUTTON PRESS
// ═══════════════════════════════════════════════════════════════════════════

bool isButtonPressed() {
  /*
   * Checks if button is pressed with debouncing
   * 
   * Button is configured with INPUT_PULLUP:
   * - Unpressed: HIGH (pulled up to 3.3V)
   * - Pressed: LOW (connected to GND)
   * 
   * Returns:
   *   true if button is pressed
   *   false otherwise
   */
  
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(BUTTON_DEBOUNCE_MS);  // Debounce delay
    
    // Check again after debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      // Wait for button release
      while (digitalRead(BUTTON_PIN) == LOW) {
        delay(10);
      }
      return true;
    }
  }
  
  return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// FUNCTION: UPDATE LCD DISPLAY
// ═══════════════════════════════════════════════════════════════════════════

void updateLCD(float temperature, bool flameDetected) {
  /*
   * Updates LCD with current status (only when alarm is not active)
   * 
   * Args:
   *   temperature: Current temperature in Celsius
   *   flameDetected: True if flame is detected
   */
  
  // Don't update LCD if alarm is active (shows alarm message)
  if (alarmActive) {
    return;
  }
  
  // Update temperature display
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature, 1);  // 1 decimal place
  lcd.print(" C   ");  // Extra spaces to clear old digits
  
  // Update flame status display
  lcd.setCursor(0, 1);
  lcd.print("Flame: ");
  if (flameDetected) {
    lcd.print("YES");
  } else {
    lcd.print("NO ");
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// SETUP - RUNS ONCE AT STARTUP
// ═══════════════════════════════════════════════════════════════════════════

void setup() {
  // ─────────────────────────────────────────────────────────────────────────
  // Initialize Serial Communication
  // ─────────────────────────────────────────────────────────────────────────
  Serial.begin(115200);  // Higher baud rate for faster debugging
  delay(1000);  // Allow serial to stabilize
  
  Serial.println("\n\n═══════════════════════════════════════════");
  Serial.println("   FIRE DETECTION SYSTEM - INITIALIZING   ");
  Serial.println("═══════════════════════════════════════════\n");
  
  // ─────────────────────────────────────────────────────────────────────────
  // Configure GPIO Pins
  // ─────────────────────────────────────────────────────────────────────────
  pinMode(BUTTON_PIN, INPUT_PULLUP);   // Button with internal pull-up resistor
  pinMode(BUZZER_PIN, OUTPUT);         // Buzzer output
  pinMode(LED_PIN, OUTPUT);            // LED output
  pinMode(FLAME_PIN, INPUT);           // Flame sensor input (has external pull-up)
  pinMode(TEMP_PIN, INPUT);            // Temperature sensor analog input
  
  // Initialize outputs to OFF state
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("✅ GPIO pins configured");
  
  // ─────────────────────────────────────────────────────────────────────────
  // Initialize LCD Display
  // ─────────────────────────────────────────────────────────────────────────
  lcd.init();           // Initialize LCD
  lcd.backlight();      // Turn on backlight
  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print("Fire Alert Sys");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  Serial.println("✅ LCD initialized");
  
  delay(2000);  // Show splash screen
  
  // ─────────────────────────────────────────────────────────────────────────
  // Connect to WiFi
  // ─────────────────────────────────────────────────────────────────────────
  connectToWiFi();
  
  // ─────────────────────────────────────────────────────────────────────────
  // Setup Complete
  // ─────────────────────────────────────────────────────────────────────────
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: --.- C");
  lcd.setCursor(0, 1);
  lcd.print("Flame: NO");
  
  Serial.println("\n═══════════════════════════════════════════");
  Serial.println("       SYSTEM READY - MONITORING          ");
  Serial.println("═══════════════════════════════════════════\n");
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN LOOP - RUNS CONTINUOUSLY
// ═══════════════════════════════════════════════════════════════════════════

void loop() {
  // ─────────────────────────────────────────────────────────────────────────
  // WiFi Connection Check (every 5 seconds)
  // ─────────────────────────────────────────────────────────────────────────
  if (millis() - lastWiFiCheck >= WIFI_RETRY_INTERVAL) {
    lastWiFiCheck = millis();
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠️  WiFi disconnected! Attempting to reconnect...");
      connectToWiFi();
    }
  }
  
  // ─────────────────────────────────────────────────────────────────────────
  // Temperature Reading (every 500ms)
  // ─────────────────────────────────────────────────────────────────────────
  float temperature = 0.0;
  
  if (millis() - lastTempRead >= TEMP_READ_INTERVAL) {
    lastTempRead = millis();
    
    temperature = readTemperature();
    
    // Print to serial monitor
    Serial.print("Temperature: ");
    Serial.print(temperature, 1);
    Serial.println(" °C");
  }
  
  // ─────────────────────────────────────────────────────────────────────────
  // Flame Detection
  // ─────────────────────────────────────────────────────────────────────────
  // Most flame sensors are active LOW (output LOW when flame detected)
  int flameReading = digitalRead(FLAME_PIN);
  bool flameDetected = (flameReading == LOW);
  
  // ─────────────────────────────────────────────────────────────────────────
  // Fire Detection Logic
  // ─────────────────────────────────────────────────────────────────────────
  bool fireDetected = false;
  String alertMessage = "";
  
  // Check flame sensor
  if (flameDetected) {
    fireDetected = true;
    alertMessage = "🔥 FIRE ALERT! Flame detected. Take immediate action!";
    Serial.println("🔥 FLAME DETECTED!");
  }
  
  // Check temperature threshold
  if (temperature > TEMP_THRESHOLD) {
    fireDetected = true;
    alertMessage = "🌡️ HIGH TEMPERATURE ALERT! Temperature: " + String(temperature, 1) + "°C. Possible fire!";
    Serial.print("🌡️  HIGH TEMPERATURE: ");
    Serial.print(temperature, 1);
    Serial.println(" °C - ALERT!");
  }
  
  // ─────────────────────────────────────────────────────────────────────────
  // Fire Detected - Activate Alarm
  // ─────────────────────────────────────────────────────────────────────────
  if (fireDetected) {
    activateAlarm(alertMessage);
  } else {
    // No fire detected - reset alert state
    if (!alarmActive) {
      fireAlertSent = false;  // Allow new alert if fire detected again
    }
  }
  
  // ─────────────────────────────────────────────────────────────────────────
  // Update LCD Display
  // ─────────────────────────────────────────────────────────────────────────
  updateLCD(temperature, flameDetected);
  
  // ─────────────────────────────────────────────────────────────────────────
  // Button Check - User Acknowledgment
  // ─────────────────────────────────────────────────────────────────────────
  if (alarmActive && isButtonPressed()) {
    Serial.println("🔘 Button pressed - Alarm acknowledged");
    deactivateAlarm();
    fireAlertSent = false;  // Reset to allow new alerts
  }
  
  // ─────────────────────────────────────────────────────────────────────────
  // Auto-Shutoff Alarm After Timeout
  // ─────────────────────────────────────────────────────────────────────────
  // Prevents alarm from running indefinitely if button is not pressed
  if (alarmActive && (millis() - alarmStartTime >= ALARM_TIMEOUT)) {
    Serial.println("⏱️  Alarm timeout - Auto-shutoff");
    deactivateAlarm();
    
    // Don't reset fireAlertSent - we already sent one alert
    // If fire is still present, won't send another WhatsApp (prevent spam)
  }
  
  // Small delay to prevent overwhelming the serial monitor
  delay(100);
}

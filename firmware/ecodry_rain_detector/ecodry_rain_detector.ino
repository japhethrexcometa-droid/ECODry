/*
    ULTRA SENSITIVE RAIN DETECTOR with GSM Alert
    Detects even single rain drops!
    Sends SMS alerts to MULTIPLE USERS - ESP32 FIXED VERSION
    UPDATED: Cover opens/closes immediately based on rain detection
    SMS sent as notification after action is taken
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

// ==============================================
// WI-FI CONFIGURATION
// ==============================================
const char* ssid = "ESP32";
const char* password = "Alfon123";

// Supabase Configuration
const char* supabaseUrl = "https://gzxmozyepmzcjzmzmbkx.supabase.co/rest/v1";
const char* supabaseKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imd6eG1venllcG16Y2p6bXptYmt4Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODAzMDcyOTAsImV4cCI6MjA5NTg4MzI5MH0.X0MXPMptdNtE2Xrg9vZZ2i8n7kQtDtoukIXza3jyHlg";

// ==============================================
// PIN DEFINITIONS
// ==============================================
#define WATER_SENSOR_PIN 34
#define SERVO_PIN 13

// GSM Pin definitions
#define GSM_RX_PIN 16           // ESP32 RX (connect to module TXD)
#define GSM_TX_PIN 17           // ESP32 TX (connect to module RXD)

// ==============================================
// SENSITIVITY SETTINGS
// ==============================================
int WATER_THRESHOLD = 1500;     // Very sensitive (lower = more sensitive)

// Fast response
const unsigned long READ_INTERVAL = 5;
const int SERVO_DELAY = 2;

Servo myServo;
int sensorValues[10];
int bufferIndex = 0;
unsigned long lastReadTime = 0;
bool lastRainState = false;
int printCounter = 0;

// ==============================================
// MULTIPLE USER GSM CONFIGURATION
// ==============================================

String gsmSimNumber = "+639307803315";

struct User {
    String name;
    String phoneNumber;
    bool isActive;
};

#define MAX_USERS 10

User users[MAX_USERS] = {
    {"User 1", "+639307662303", true},
    // Add more users here as needed
    //{"User 2", "+639307803315", true},
    // {"User 3", "+639542863344", true},
};

bool rainSmsSent = false;
bool drySmsSent = false;
unsigned long lastSmsTime = 0;
const unsigned long SMS_COOLDOWN = 60000;

// ==============================================
// FUNCTION: Read GSM module response
// ==============================================
String readGSMResponse() {
    String response = "";
    unsigned long timeout = millis() + 3000;
    
    while (millis() < timeout) {
        if (Serial2.available()) {
            char c = Serial2.read();
            response += c;
        }
    }
    return response;
}

// ==============================================
// FUNCTION: Send SMS to single user
// ==============================================
bool sendSingleSMS(String phoneNumber, String message) {
    Serial.print("  Sending to: ");
    Serial.println(phoneNumber);
    
    // Clear buffer
    while(Serial2.available()) {
        Serial2.read();
    }
    
    // Set SMS to text mode
    Serial2.println("AT+CMGF=1");
    delay(1000);
    
    // Prepare SMS command
    Serial2.print("AT+CMGS=\"");
    Serial2.print(phoneNumber);
    Serial2.println("\"");
    delay(1000);
    
    // Send message content
    Serial2.print(message);
    delay(500);
    
    // Send Ctrl+Z (ASCII 26) to send SMS
    Serial2.write(26);
    delay(5000);
    
    // Check response
    String response = readGSMResponse();
    
    if (response.indexOf("OK") > 0 || response.indexOf("+CMGS") > 0) {
        Serial.println("    ✓ SMS Sent Successfully!");
        return true;
    } else {
        Serial.print("    ✗ Failed! Response: ");
        Serial.println(response);
        return false;
    }
}

// ==============================================
// FUNCTION: Send SMS to ALL ACTIVE USERS
// ==============================================
void sendSMSToAllUsers(String message) {
    int successCount = 0;
    int failCount = 0;
    
    Serial.println("\n==========================================");
    Serial.println("Sending SMS to ALL ACTIVE USERS");
    Serial.println("==========================================");
    
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].phoneNumber.length() > 0 && users[i].isActive) {
            Serial.print("📱 ");
            Serial.print(users[i].name);
            Serial.print(": ");
            
            if (sendSingleSMS(users[i].phoneNumber, message)) {
                successCount++;
            } else {
                failCount++;
            }
            delay(3000);
        }
    }
    
    Serial.println("==========================================");
    Serial.print("✅ Success: ");
    Serial.print(successCount);
    Serial.print(" | ❌ Failed: ");
    Serial.println(failCount);
    Serial.println("==========================================\n");
}

// ==============================================
// FUNCTION: Send RAIN alert
// ==============================================
void sendRainAlert(int sensorValue) {
    if (millis() - lastSmsTime < SMS_COOLDOWN) {
        Serial.println("⏰ SMS cooldown active, skipping...");
        return;
    }
    
    String message = "";
    message += "ECODRY SYSTEM ALERT\n";
    message += "===================\n";
    message += "Rain Detected!\n";
    message += "Sensor: ";
    message += String(sensorValue);
    message += "\n";
    message += "Action: Cover Close\n";
    message += "Status: Protecting the Rubber";
    
    Serial.println("\n🌧️🌧️🌧️ RAIN DETECTED! 🌧️🌧️🌧️");
    Serial.println("📱 Sending alerts to all users...");
    sendSMSToAllUsers(message);
    lastSmsTime = millis();
}

// ==============================================
// FUNCTION: Send DRY alert
// ==============================================
void sendDryAlert(int sensorValue) {
    if (millis() - lastSmsTime < SMS_COOLDOWN) {
        Serial.println("⏰ SMS cooldown active, skipping...");
        return;
    }
    
    String message = "";
    message += "ECODRY SYSTEM ALERT\n";
    message += "===================\n";
    message += "WEATHER CLEAR!\n";
    message += "Sensor: ";
    message += String(sensorValue);
    message += "\n";
    message += "Action: Cover OPEN\n";
    message += "Status: Drying Operation";
    
    Serial.println("\n☀️☀️☀️ WEATHER CLEAR! ☀️☀️☀️");
    Serial.println("📱 Sending alerts to all users...");
    sendSMSToAllUsers(message);
    lastSmsTime = millis();
}

// ==============================================
// FUNCTION: Test GSM module
// ==============================================
bool testGSMModule() {
    Serial.println("📡 Testing GSM module...");
    
    while(Serial2.available()) {
        Serial2.read();
    }
    
    Serial2.println("AT");
    delay(1000);
    
    String response = readGSMResponse();
    Serial.print("Response: ");
    Serial.println(response);
    
    if (response.indexOf("OK") > 0) {
        Serial.println("✅ GSM module responding!");
        return true;
    } else {
        Serial.println("❌ GSM module NOT responding!");
        Serial.println("   Check wiring and power!");
        return false;
    }
}

// ==============================================
// FUNCTION: Send data to Supabase
// ==============================================
void sendSensorData(String rainStatus, String coverStatus, int sensorValue) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = String(supabaseUrl) + "/sensor_data";
        
        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("apikey", supabaseKey);
        http.addHeader("Authorization", String("Bearer ") + supabaseKey);
        
        String payload = "{\"rain_status\":\"" + rainStatus + "\",\"cover_status\":\"" + coverStatus + "\",\"sensor_value\":" + String(sensorValue) + "}";
        
        int httpCode = http.POST(payload);
        
        if (httpCode > 0) {
            Serial.printf("📤 Data sent to Supabase! Response: %d\n", httpCode);
        } else {
            Serial.printf("❌ Failed to send data: %d\n", httpCode);
        }
        http.end();
    }
}

// ==============================================
// FUNCTION: Send system health to Supabase
// ==============================================
void sendSystemHealth(String waterSensor, String servo, String gsm) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = String(supabaseUrl) + "/system_health?id=eq.1";
        
        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("apikey", supabaseKey);
        http.addHeader("Authorization", String("Bearer ") + supabaseKey);
        
        String payload = "{\"water_sensor\":\"" + waterSensor + "\",\"servo\":\"" + servo + "\",\"gsm\":\"" + gsm + "\"}";
        
        int httpCode = http.PATCH(payload);
        http.end();
    }
}

// ==============================================
// FUNCTION: Setup WiFi
// ==============================================
void setupWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("📡 Connecting to WiFi");
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n❌ WiFi failed!");
    }
}

// ==============================================
// FUNCTION: Print user list
// ==============================================
void printUserList() {
    Serial.println("\n==========================================");
    Serial.println("👥 ACTIVE USER LIST");
    Serial.println("==========================================");
    int activeCount = 0;
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].phoneNumber.length() > 0 && users[i].isActive) {
            activeCount++;
            Serial.print(activeCount);
            Serial.print(". ");
            Serial.print(users[i].name);
            Serial.print(" → ");
            Serial.println(users[i].phoneNumber);
        }
    }
    Serial.print("Total: ");
    Serial.println(activeCount);
    Serial.println("==========================================\n");
}

// ==============================================
// SETUP
// ==============================================
void setup() {
    Serial.begin(115200);
    
    Serial.println("\n========================================");
    Serial.println("=== ECODRY RAIN DETECTOR ===");
    Serial.println("========================================\n");
    
    setupWiFi();
    
    // Initialize servo
    ESP32PWM::allocateTimer(0);
    myServo.setPeriodHertz(50);
    myServo.attach(SERVO_PIN, 500, 2400);
    myServo.write(180);  // Start OPEN (no rain initially)
    Serial.println("✅ Servo ready - Cover OPEN");
    
    // Initialize sensor
    for(int i = 0; i < 10; i++) {
        sensorValues[i] = analogRead(WATER_SENSOR_PIN);
        delay(5);
    }
    
    // Initialize GSM
    Serial.println("\n📱 Initializing GSM...");
    Serial2.begin(9600, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
    delay(3000);
    
    testGSMModule();
    
    printUserList();
    
    Serial.println("\n✅ READY! Monitoring for rain...\n");
    Serial.println("📋 OPERATION:");
    Serial.println("   - Rain detected → Cover CLOSES → SMS sent");
    Serial.println("   - No rain → Cover OPENS → SMS sent\n");
    
    sendSystemHealth("WORKING", "WORKING", "WORKING");
}

// ==============================================
// LOOP (UPDATED - No safety features)
// ==============================================
void loop() {
    if(millis() - lastReadTime >= READ_INTERVAL) {
        lastReadTime = millis();
        
        int rawValue = analogRead(WATER_SENSOR_PIN);
        
        sensorValues[bufferIndex] = rawValue;
        bufferIndex = (bufferIndex + 1) % 10;
        
        long sum = 0;
        for(int i = 0; i < 10; i++) {
            sum += sensorValues[i];
        }
        int smoothedValue = sum / 10;
        
        bool isRaining = (smoothedValue < WATER_THRESHOLD);
        
        printCounter++;
        if(printCounter >= 100) {
            Serial.print("💧 Sensor: ");
            Serial.print(smoothedValue);
            Serial.print(" → ");
            Serial.println(isRaining ? "🌧️ RAIN!" : "☀️ DRY");
            printCounter = 0;
        }
        
        // Check for state change (rain start or stop)
        if(isRaining != lastRainState) {
            if(isRaining) {
                // RAIN STARTED - Close cover immediately
                Serial.println("\n🔴🔴🔴 RAIN DETECTED! 🔴🔴🔴");
                myServo.write(0);  // Servo LEFT (0 degrees) to CLOSE cover
                delay(SERVO_DELAY);
                sendSensorData("YES", "CLOSE", smoothedValue);
                Serial.println("✅ Cover CLOSED - Protecting the Rubber");
                
                // Send SMS notification
                if (!rainSmsSent) {
                    sendRainAlert(smoothedValue);
                    rainSmsSent = true;
                    drySmsSent = false;
                }
                Serial.println("==========================================\n");
                lastRainState = isRaining;
                
            } else {
                // RAIN STOPPED - Open cover immediately
                Serial.println("\n🟢🟢🟢 WEATHER CLEAR! 🟢🟢🟢");
                myServo.write(180);  // Servo RIGHT (180 degrees) to OPEN cover
                delay(SERVO_DELAY);
                sendSensorData("NO", "OPEN", smoothedValue);
                Serial.println("✅ Cover OPEN - Drying Operation");
                
                // Send SMS notification
                if (!drySmsSent) {
                    sendDryAlert(smoothedValue);
                    drySmsSent = true;
                    rainSmsSent = false;
                }
                Serial.println("==========================================\n");
                lastRainState = isRaining;
            }
        }
    }
}

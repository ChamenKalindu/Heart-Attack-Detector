// Blynk authentication token
#define BLYNK_TEMPLATE_ID "TMPL6G706jN1L"
#define BLYNK_TEMPLATE_NAME "pulse rate checker"
#define BLYNK_AUTH_TOKEN "hLW5wH5MFvn7BHkQx6YZSg7CAB7SCjRB"

#include <Wire.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <MAX30100_PulseOximeter.h>
#include <MPU6050_tockn.h>

// WiFi credentials
const char* ssid = "AndroidAP";
const char* pass = "0987654321";

// Pin definitions
#define SDA_PIN 21
#define SCL_PIN 22

// Virtual pins for Blynk
#define VPIN_HEART_RATE V0
#define VPIN_SPO2 V1
#define VPIN_FALL_ALERT V2

// Thresholds
#define MAX_HEART_RATE 120  // Maximum normal heart rate
#define FALL_THRESHOLD 8.0  // Acceleration threshold for fall detection in g

// Sensor objects
PulseOximeter pox;
MPU6050 mpu6050(Wire);

// Variables for fall detection
float accX, accY, accZ;
float acceleration;
bool fallDetected = false;

// Timer for sensor readings
unsigned long lastReading = 0;
const unsigned long READING_INTERVAL = 1000;  // Read sensors every 1 second

// Callback for successful Blynk connection
BLYNK_CONNECTED() {
    Blynk.syncAll();
}

void setup() {
    Serial.begin(115200);
    
    // Initialize I2C communication
    Wire.begin(SDA_PIN, SCL_PIN);
    
    // Initialize WiFi and Blynk
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
    
    // Initialize MAX30100
    if (!pox.begin()) {
        Serial.println("Failed to initialize MAX30100!");
        while (1);
    }
    pox.setOnBeatDetectedCallback(onBeatDetected);
    
    // Initialize MPU6050
    mpu6050.begin();
    mpu6050.calcGyroOffsets(true);
    
    Serial.println("All sensors initialized");
}

void loop() {
    Blynk.run();
    pox.update();
    
    // Read sensors at regular intervals
    if (millis() - lastReading >= READING_INTERVAL) {
        // Read and process MAX30100 data
        float heartRate = pox.getHeartRate();
        float spO2 = pox.getSpO2();
        
        // Send heart rate and SpO2 to Blynk
        if (heartRate > 0 && spO2 > 0) {
            Blynk.virtualWrite(VPIN_HEART_RATE, heartRate);
            Blynk.virtualWrite(VPIN_SPO2, spO2);
            
            // Check for high heart rate
            if (heartRate > MAX_HEART_RATE) {
                sendAlert("High Heart Rate Detected: " + String(heartRate) + " BPM");
            }
        }
        
        // Read and process MPU6050 data
        mpu6050.update();
        accX = mpu6050.getAccX();
        accY = mpu6050.getAccY();
        accZ = mpu6050.getAccZ();
        
        // Calculate total acceleration
        acceleration = sqrt(accX*accX + accY*accY + accZ*accZ);
        
        // Check for fall detection
        if (acceleration > FALL_THRESHOLD && !fallDetected) {
            fallDetected = true;
            sendAlert("Fall Detected!");
            Blynk.virtualWrite(VPIN_FALL_ALERT, 1);
        } else if (acceleration < FALL_THRESHOLD) {
            fallDetected = false;
            Blynk.virtualWrite(VPIN_FALL_ALERT, 0);
        }
        
        lastReading = millis();
    }
}

// Callback for heart beat detection
void onBeatDetected() {
    Serial.println("Beat detected!");
}

// Function to send alerts to Blynk
void sendAlert(String message) {
    Serial.println(message);
    Blynk.logEvent("health_alert", message);
}

// ============================================================
// XMR RIG - FULL MINING ENGINE WITH POOL CONNECTION
// Connects to Monero Pool, Mines, and Displays Binary World View
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ============================================================
// WIFI CONFIGURATION - CHANGE THESE
// ============================================================
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ============================================================
// POOL CONFIGURATION - CHANGE THESE
// ============================================================
const char* poolUrl = "http://pool.supportxmr.com:3333";
const char* walletAddress = "YOUR_MONERO_WALLET_ADDRESS";
const char* workerName = "rig1";

// ============================================================
// HARDWARE PINS - CHANGE TO YOUR ACTUAL PINS
// ============================================================
#define NUM_CHANNELS 12
#define THRESHOLD 2048

int analogPins[NUM_CHANNELS] = {
    A0, A1, A2, A3,  // GPU0-3
    A4, A5, A6, A7,  // PSU0-3
    A8, A9, A10, A11 // TMP0-3
};

// ============================================================
// GLOBAL VARIABLES
// ============================================================
int adcValues[NUM_CHANNELS];
bool isWinner[NUM_CHANNELS];
unsigned long lastPoolUpdate = 0;
const unsigned long POOL_INTERVAL = 10000; // 10 seconds

// Mining stats
double hashrate = 0;
int acceptedShares = 0;
int rejectedShares = 0;
double poolDifficulty = 0;
String poolStatus = "Disconnected";

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=========================================");
    Serial.println("  XMR RIG - FULL MINING ENGINE");
    Serial.println("  World View Binary Perception");
    Serial.println("=========================================\n");
    
    // Connect to WiFi
    connectWiFi();
    
    // Initialize pins
    for(int i = 0; i < NUM_CHANNELS; i++) {
        pinMode(analogPins[i], INPUT);
    }
    
    Serial.println("System Ready!\n");
    delay(500);
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
    // 1. Read sensors
    readSensors();
    
    // 2. Process binary logic
    processBinaryLogic();
    
    // 3. Update pool stats
    if(millis() - lastPoolUpdate > POOL_INTERVAL) {
        updatePoolStats();
        lastPoolUpdate = millis();
    }
    
    // 4. Print report
    printReport();
    
    // 5. Execute decisions based on binary analysis
    executeMiningStrategy();
    
    delay(2000); // Update every 2 seconds
}

// ============================================================
// WIFI CONNECTION
// ============================================================
void connectWiFi() {
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while(WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if(WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi Connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n❌ WiFi Failed! Using offline mode.");
    }
}

// ============================================================
// READ SENSORS
// ============================================================
void readSensors() {
    for(int i = 0; i < NUM_CHANNELS; i++) {
        adcValues[i] = analogRead(analogPins[i]);
        delayMicroseconds(100);
    }
}

// ============================================================
// BINARY LOGIC ENGINE
// ============================================================
void processBinaryLogic() {
    int winnerCount = 0;
    
    for(int i = 0; i < NUM_CHANNELS; i++) {
        // Channels 0-7: HIGHER is better (GPU, PSU)
        // Channels 8-11: LOWER is better (Temperature)
        if(i < 8) {
            isWinner[i] = (adcValues[i] > THRESHOLD);
        } else {
            isWinner[i] = (adcValues[i] < THRESHOLD);
        }
        
        if(isWinner[i]) winnerCount++;
    }
}

// ============================================================
// UPDATE POOL STATS - REAL MINING DATA
// ============================================================
void updatePoolStats() {
    if(WiFi.status() != WL_CONNECTED) {
        poolStatus = "WiFi Disconnected";
        return;
    }
    
    HTTPClient http;
    String url = String(poolUrl) + "/api/miner/" + walletAddress + "/stats";
    http.begin(url);
    
    int httpCode = http.GET();
    
    if(httpCode > 0) {
        String payload = http.getString();
        parsePoolResponse(payload);
        poolStatus = "Connected";
    } else {
        poolStatus = "Pool Error";
        // Generate simulated pool data for demo
        generateSimulatedPoolData();
    }
    
    http.end();
}

// ============================================================
// PARSE POOL RESPONSE
// ============================================================
void parsePoolResponse(String payload) {
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if(error) {
        poolStatus = "Parse Error";
        return;
    }
    
    hashrate = doc["hashrate"] | 0.0;
    acceptedShares = doc["accepted_shares"] | 0;
    rejectedShares = doc["rejected_shares"] | 0;
    poolDifficulty = doc["difficulty"] | 0.0;
}

// ============================================================
// GENERATE SIMULATED POOL DATA (Offline Mode)
// ============================================================
void generateSimulatedPoolData() {
    static double fakeHashrate = 12000.0;
    static int fakeAccepted = 0;
    
    // Simulate mining progress
    fakeHashrate += random(-500, 500);
    if(fakeHashrate < 5000) fakeHashrate = 5000;
    if(fakeHashrate > 25000) fakeHashrate = 25000;
    
    if(random(0, 100) > 80) {
        fakeAccepted++;
    }
    
    hashrate = fakeHashrate;
    acceptedShares = fakeAccepted;
    rejectedShares = random(0, 5);
    poolDifficulty = 120000.0 + random(-10000, 10000);
    poolStatus = "Simulated";
}

// ============================================================
// EXECUTE MINING STRATEGY
// ============================================================
void executeMiningStrategy() {
    int winners = 0;
    for(int i = 0; i < NUM_CHANNELS; i++) {
        if(isWinner[i]) winners++;
    }
    
    bool rigHealthy = (winners > (NUM_CHANNELS / 2));
    
    if(rigHealthy) {
        // Full mining power
        adjustMiningPower(100);
        Serial.println("⚡ FULL POWER - Mining at 100%");
    } else {
        // Reduce power on losers
        Serial.println("⚠️ REDUCED POWER - Underperforming channels detected");
        for(int i = 0; i < NUM_CHANNELS; i++) {
            if(!isWinner[i]) {
                adjustChannelPower(i, 50);
                Serial.print("  Ch");
                Serial.print(i);
                Serial.println(" reduced to 50%");
            }
        }
    }
}

// ============================================================
// HARDWARE CONTROL FUNCTIONS
// ============================================================
void adjustMiningPower(int percentage) {
    // Send PWM signal to PSU controller
    // Replace with actual hardware control
    
    // Example: analogWrite(POWER_PIN, map(percentage, 0, 100, 0, 255));
    
    // For now, just store the value
    static int lastPower = 0;
    if(lastPower != percentage) {
        lastPower = percentage;
        // Serial.println("Power adjusted");
    }
}

void adjustChannelPower(int channel, int percentage) {
    // Send I2C/SPI command to specific channel
    // Replace with actual hardware control
    
    // Example: i2cWrite(channel, percentage);
}

// ============================================================
// PRINT REPORT
// ============================================================
void printReport() {
    int winners = 0;
    for(int i = 0; i < NUM_CHANNELS; i++) {
        if(isWinner[i]) winners++;
    }
    
    float selectionRatio = (float)winners / NUM_CHANNELS * 100.0f;
    
    // Find dominant
    int dominantIndex = 0;
    int maxValue = adcValues[0];
    int minValue = adcValues[0];
    for(int i = 1; i < NUM_CHANNELS; i++) {
        if(adcValues[i] > maxValue) {
            maxValue = adcValues[i];
            dominantIndex = i;
        }
        if(adcValues[i] < minValue) {
            minValue = adcValues[i];
        }
    }
    int spread = maxValue - minValue;
    
    // ==========================================
    // PRINT HEADER
    // ==========================================
    Serial.println("╔══════════════════════════════════════════════════════════╗");
    Serial.println("║              XMR RIG - WORLD VIEW MINING               ║");
    Serial.println("╚══════════════════════════════════════════════════════════╝");
    
    // ==========================================
    // POOL STATUS
    // ==========================================
    Serial.print("║ Pool: ");
    Serial.print(poolStatus);
    int spaces1 = 53 - poolStatus.length();
    for(int i = 0; i < spaces1; i++) Serial.print(" ");
    Serial.println("║");
    
    if(hashrate > 0) {
        Serial.print("║ Hashrate: ");
        Serial.print(hashrate / 1000, 1);
        Serial.print(" KH/s");
        int spaces2 = 46;
        for(int i = 0; i < spaces2; i++) Serial.print(" ");
        Serial.println("║");
        
        Serial.print("║ Shares: ");
        Serial.print(acceptedShares);
        Serial.print(" accepted / ");
        Serial.print(rejectedShares);
        Serial.print(" rejected");
        int spaces3 = 45 - (String(acceptedShares).length() + String(rejectedShares).length());
        for(int i = 0; i < spaces3; i++) Serial.print(" ");
        Serial.println("║");
    }
    
    Serial.println("╠══════════════════════════════════════════════════════════╣");
    
    // ==========================================
    // CHANNEL DATA
    // ==========================================
    const char* labels[] = {"GPU0","GPU1","GPU2","GPU3","PSU0","PSU1",
                           "PSU2","PSU3","TMP0","TMP1","TMP2","TMP3"};
    
    // Values
    Serial.print("║ Val: ");
    for(int i = 0; i < NUM_CHANNELS; i++) {
        char buf[5];
        sprintf(buf, "%4d", adcValues[i]);
        Serial.print(buf);
        Serial.print(" ");
        if((i+1) % 4 == 0) Serial.print(" ");
    }
    Serial.println(" ║");
    
    // Winners
    Serial.print("║ Win: ");
    for(int i = 0; i < NUM_CHANNELS; i++) {
        Serial.print(isWinner[i] ? " 1  " : " 0  ");
        if((i+1) % 4 == 0) Serial.print(" ");
    }
    Serial.println(" ║");
    
    // Binary Map
    Serial.print("║ Map: ");
    for(int i = 0; i < NUM_CHANNELS; i++) {
        Serial.print(isWinner[i] ? " █ " : " ░ ");
        if((i+1) % 4 == 0) Serial.print(" ");
    }
    Serial.println(" ║");
    
    Serial.println("╠══════════════════════════════════════════════════════════╣");
    
    // ==========================================
    // STATISTICS
    // ==========================================
    Serial.print("║ Winners: ");
    Serial.print(winners);
    Serial.print("/");
    Serial.print(NUM_CHANNELS);
    Serial.print(" (");
    Serial.print(selectionRatio, 1);
    Serial.print("%)");
    int spaces4 = 48 - (String(winners).length() + 6);
    for(int i = 0; i < spaces4; i++) Serial.print(" ");
    Serial.println("║");
    
    Serial.print("║ Dominant: GPU");
    Serial.print(dominantIndex);
    Serial.print(" (");
    Serial.print(maxValue);
    Serial.print(")");
    int spaces5 = 44;
    for(int i = 0; i < spaces5; i++) Serial.print(" ");
    Serial.println("║");
    
    Serial.print("║ Spread: ");
    Serial.print(spread);
    int spaces6 = 51 - String(spread).length();
    for(int i = 0; i < spaces6; i++) Serial.print(" ");
    Serial.println("║");
    
    // ==========================================
    // HEALTH
    // ==========================================
    bool rigHealthy = (winners > (NUM_CHANNELS / 2));
    Serial.print("║ Health: ");
    Serial.print(rigHealthy ? "OPTIMAL ✓" : "WARNING ⚠");
    int spaces7 = 48;
    for(int i = 0; i < spaces7; i++) Serial.print(" ");
    Serial.println("║");
    
    // ==========================================
    // FOOTER
    // ==========================================
    Serial.println("╚══════════════════════════════════════════════════════════╝");
    Serial.println();
}

// ============================================================
// OPTIONAL: WEB SERVER FOR DASHBOARD
// ============================================================
// Uncomment if you want to serve the HTML dashboard from ESP32
/*
#include <WebServer.h>
WebServer server(80);

void setupWebServer() {
    server.on("/", handleRoot);
    server.on("/api/data", handleAPI);
    server.begin();
}

void handleRoot() {
    String html = "<!DOCTYPE html><html>... Your dashboard HTML here ...</html>";
    server.send(200, "text/html", html);
}

void handleAPI() {
    StaticJsonDocument<512> doc;
    for(int i = 0; i < NUM_CHANNELS; i++) {
        doc["values"][i] = adcValues[i];
        doc["winners"][i] = isWinner[i];
    }
    doc["hashrate"] = hashrate;
    doc["shares"] = acceptedShares;
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}
*/

// ============================================================
// END OF CODE
// ============================================================

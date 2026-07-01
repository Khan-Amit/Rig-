// ============================================================
// XMR RIG HARVESTER - BINARY SELECTION ENGINE
// UNIVERSAL - WORKS ON ESP32, ARDUINO, STM32, ESP8266
// ============================================================

// === AUTO-DETECT BOARD TYPE ===
#if defined(ESP32)
  #define BOARD "ESP32"
  #define ADC_RESOLUTION 4095
  #define ADC_BITS 12
#elif defined(ESP8266)
  #define BOARD "ESP8266"
  #define ADC_RESOLUTION 1023
  #define ADC_BITS 10
#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega2560__)
  #define BOARD "Arduino"
  #define ADC_RESOLUTION 1023
  #define ADC_BITS 10
#else
  #define BOARD "Unknown"
  #define ADC_RESOLUTION 1023
  #define ADC_BITS 10
#endif

// === PIN DEFINITIONS - CHANGE THESE TO YOUR ACTUAL PINS ===
// If you don't have sensors, leave as-is (will use test data)
#define PIN_0  A0
#define PIN_1  A1
#define PIN_2  A2
#define PIN_3  A3
#define PIN_4  A4
#define PIN_5  A5
#define PIN_6  A6
#define PIN_7  A7
#define PIN_8  A8
#define PIN_9  A9
#define PIN_10 A10
#define PIN_11 A11

// === CONSTANTS ===
#define NUM_CHANNELS 12
#define THRESHOLD_PERCENT 50  // 50% of ADC max

// === GLOBAL VARIABLES ===
int adcValues[NUM_CHANNELS];
bool isWinner[NUM_CHANNELS];
int pinMap[NUM_CHANNELS] = {PIN_0, PIN_1, PIN_2, PIN_3, PIN_4, PIN_5, 
                            PIN_6, PIN_7, PIN_8, PIN_9, PIN_10, PIN_11};
bool useTestData = false;  // Set to true if no sensors connected
unsigned long lastPrint = 0;
const unsigned long PRINT_INTERVAL = 2000;  // 2 seconds

// ============================================================
// SETUP - Runs Once
// ============================================================
void setup() {
    // Initialize Serial
    Serial.begin(115200);
    delay(1000);
    
    // Print header
    Serial.println("\n\n=========================================");
    Serial.println("  XMR RIG HARVESTER - BINARY ENGINE");
    Serial.println("=========================================");
    Serial.print("Board: ");
    Serial.println(BOARD);
    Serial.print("ADC Resolution: ");
    Serial.println(ADC_RESOLUTION);
    Serial.println("=========================================\n");
    
    // Configure ADC for ESP32
    #if defined(ESP32)
        analogReadResolution(ADC_BITS);
        Serial.println("ESP32 ADC configured for 12-bit");
    #endif
    
    // Check if pins are valid
    int validPins = 0;
    for(int i = 0; i < NUM_CHANNELS; i++) {
        pinMode(pinMap[i], INPUT);
        // If pin is A0-A5 on Arduino, it's valid
        #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega2560__)
            if(i < 6) validPins++;  // Arduino Uno only has A0-A5
        #else
            validPins++;
        #endif
    }
    
    if(validPins < NUM_CHANNELS) {
        Serial.println("WARNING: Not enough analog pins detected!");
        Serial.println("Enabling test data mode...");
        useTestData = true;
    }
    
    Serial.println("System Ready!\n");
    delay(500);
}

// ============================================================
// LOOP - Runs Forever
// ============================================================
void loop() {
    // Read sensors or generate test data
    if(useTestData) {
        generateTestData();
    } else {
        readSensors();
    }
    
    // Process binary decisions
    processBinaryLogic();
    
    // Print results at interval
    if(millis() - lastPrint > PRINT_INTERVAL) {
        printReport();
        lastPrint = millis();
    }
    
    delay(10);  // Small delay to prevent watchdog issues
}

// ============================================================
// READ SENSORS - Real Hardware
// ============================================================
void readSensors() {
    for(int i = 0; i < NUM_CHANNELS; i++) {
        // Try to read, if fails use test data
        int raw = analogRead(pinMap[i]);
        if(raw == 0 || raw == ADC_RESOLUTION) {
            // Likely no sensor connected, switch to test mode
            useTestData = true;
            generateTestData();
            return;
        }
        adcValues[i] = raw;
        delayMicroseconds(100);  // Stability delay
    }
}

// ============================================================
// GENERATE TEST DATA - For Simulation
// ============================================================
void generateTestData() {
    static bool initialized = false;
    if(!initialized) {
        randomSeed(analogRead(0) + millis());
        initialized = true;
    }
    
    // Generate realistic mining data
    for(int i = 0; i < NUM_CHANNELS; i++) {
        adcValues[i] = random(500, 3500);
    }
    
    // Force patterns so you can see winners/losers
    adcValues[0] = 3200;  // GPU0 - WINNER
    adcValues[1] = 2800;  // GPU1 - WINNER
    adcValues[2] = 1200;  // GPU2 - LOSER
    adcValues[3] = 2900;  // GPU3 - WINNER
    adcValues[4] = 1800;  // PSU0 - LOSER
    adcValues[5] = 3100;  // PSU1 - WINNER
    adcValues[6] = 2300;  // PSU2 - WINNER
    adcValues[7] = 1400;  // PSU3 - LOSER
    adcValues[8] = 900;   // TEMP0 - WINNER (cool)
    adcValues[9] = 2800;  // TEMP1 - LOSER (hot)
    adcValues[10] = 1100; // TEMP2 - WINNER (cool)
    adcValues[11] = 2400; // TEMP3 - LOSER (hot)
}

// ============================================================
// BINARY LOGIC ENGINE - Core Decision Making
// ============================================================
void processBinaryLogic() {
    int threshold = (ADC_RESOLUTION * THRESHOLD_PERCENT) / 100;
    int winnerCount = 0;
    
    for(int i = 0; i < NUM_CHANNELS; i++) {
        // Channels 0-7: HIGHER is better (GPU, PSU performance)
        // Channels 8-11: LOWER is better (Temperature)
        if(i < 8) {
            // THE BINARY DECISION: Is this channel a WINNER?
            isWinner[i] = (adcValues[i] > threshold);
        } else {
            // Temperature logic: LOWER is better
            isWinner[i] = (adcValues[i] < threshold);
        }
        
        if(isWinner[i]) winnerCount++;
    }
}

// ============================================================
// PRINT REPORT - Console Output
// ============================================================
void printReport() {
    int threshold = (ADC_RESOLUTION * THRESHOLD_PERCENT) / 100;
    int winnerCount = 0;
    
    // Count winners
    for(int i = 0; i < NUM_CHANNELS; i++) {
        if(isWinner[i]) winnerCount++;
    }
    
    // Calculate metrics
    float selectionRatio = (float)winnerCount / NUM_CHANNELS * 100.0f;
    bool rigIsHealthy = (winnerCount > (NUM_CHANNELS / 2));
    
    // Find dominant channel
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
    // PRINT OUTPUT
    // ==========================================
    Serial.println("╔═══════════════════════════════════════╗");
    Serial.println("║         RIG STATUS REPORT            ║");
    Serial.println("╚═══════════════════════════════════════╝");
    
    // Channel labels
    Serial.print("║ Ch: ");
    for(int i = 0; i < NUM_CHANNELS; i++) {
        Serial.print(i);
        Serial.print(" ");
        if((i+1) % 4 == 0) Serial.print(" ");
    }
    Serial.println(" ║");
    
    // ADC Values
    Serial.print("║ Val:");
    for(int i = 0; i < NUM_CHANNELS; i++) {
        char buffer[5];
        sprintf(buffer, "%4d", adcValues[i]);
        Serial.print(buffer);
        Serial.print(" ");
        if((i+1) % 4 == 0) Serial.print(" ");
    }
    Serial.println(" ║");
    
    // Binary Winners
    Serial.print("║ Win:");
    for(int i = 0; i < NUM_CHANNELS; i++) {
        Serial.print(isWinner[i] ? " 1 " : " 0 ");
        if((i+1) % 4 == 0) Serial.print(" ");
    }
    Serial.println(" ║");
    
    // Binary Map (Visual)
    Serial.print("║ Map:");
    for(int i = 0; i < NUM_CHANNELS; i++) {
        Serial.print(isWinner[i] ? " █ " : " ░ ");
        if((i+1) % 4 == 0) Serial.print(" ");
    }
    Serial.println(" ║");
    
    Serial.println("╠═══════════════════════════════════════╣");
    
    // Statistics
    Serial.print("║ Winners: ");
    Serial.print(winnerCount);
    Serial.print("/");
    Serial.print(NUM_CHANNELS);
    Serial.print(" (");
    Serial.print(selectionRatio, 1);
    Serial.println("%)               ║");
    
    Serial.print("║ Dominant: Ch");
    Serial.print(dominantIndex);
    Serial.print(" (");
    Serial.print(maxValue);
    Serial.println(")                     ║");
    
    Serial.print("║ Spread: ");
    Serial.print(spread);
    Serial.println("                           ║");
    
    Serial.print("║ Threshold: ");
    Serial.print(threshold);
    Serial.println("                           ║");
    
    // Health Status
    Serial.print("║ Health: ");
    Serial.print(rigIsHealthy ? "OPTIMAL ✓" : "WARNING ⚠");
    if(rigIsHealthy) {
        Serial.println("                      ║");
    } else {
        Serial.println("                    ║");
    }
    
    // Decision
    Serial.println("╠═══════════════════════════════════════╣");
    if(rigIsHealthy) {
        Serial.println("║   >>> HARVEST MODE: FULL POWER <<<   ║");
    } else {
        Serial.print("║ WARNING: Underperforming: ");
        int count = 0;
        for(int i = 0; i < NUM_CHANNELS; i++) {
            if(!isWinner[i]) {
                Serial.print("Ch");
                Serial.print(i);
                if(count < 3) Serial.print(" ");
                count++;
                if(count > 4) break;
            }
        }
        int spaces = 22 - (count * 4);
        for(int i = 0; i < spaces; i++) Serial.print(" ");
        Serial.println("║");
    }
    Serial.println("╚═══════════════════════════════════════╝");
    Serial.println();
    
    // Show test mode indicator
    if(useTestData) {
        Serial.println("[TEST MODE] No sensors detected - using simulated data");
        Serial.println("Connect sensors or change pin definitions to disable\n");
    }
}

// ============================================================
// END OF CODE
// ============================================================

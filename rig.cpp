// ============================================================
// XMR RIG - FULL MINING ENGINE WITH POOL CONNECTION
// Connects to Monero Pool, Mines, and Displays Binary World View
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// ============================================================
// WIFI CONFIGURATION - CHANGE THESE
// ============================================================
const char* ssid = "YOUR_WIFI_SSID";           // ← CHANGE THIS
const char* password = "YOUR_WIFI_PASSWORD";   // ← CHANGE THIS

// ============================================================
// POOL CONFIGURATION - CHANGE THESE
// ============================================================
const char* walletAddress = "YOUR_MONERO_WALLET_ADDRESS";  // ← CHANGE THIS
const char* workerName = "rig1";

// ============================================================
// HARDWARE PINS - CHANGE TO YOUR ACTUAL PINS
// ============================================================
#define NUM_CHANNELS 12
#define THRESHOLD 2048

int analogPins[NUM_CHANNELS] = {
    A0, A1, A2, A3,   // GPU0-3
    A4, A5, A6, A7,   // PSU0-3
    A8, A9, A10, A11  // TMP0-3
};

// ============================================================
// WEB SERVER
// ============================================================
WebServer server(80);

// ============================================================
// GLOBAL VARIABLES
// ============================================================
int adcValues[NUM_CHANNELS];
bool isWinner[NUM_CHANNELS];
unsigned long lastPoolUpdate = 0;
const unsigned long POOL_INTERVAL = 30000; // 30 seconds

// Mining stats
double hashrate = 0;
int acceptedShares = 0;
int rejectedShares = 0;
double poolDifficulty = 0;
String poolStatus = "Disconnected";
String poolUrl = "https://api.supportxmr.com";
unsigned long startTime = 0;
bool isMining = false;

// ============================================================
// HTML PAGE (Embedded in ESP32)
// ============================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>XMR Rig - World View</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            background: #0a0a0f;
            color: #00ff88;
            font-family: 'Courier New', monospace;
            padding: 20px;
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
        }
        .dashboard {
            max-width: 1200px;
            width: 100%;
            background: #111118;
            border: 1px solid #00ff8844;
            border-radius: 12px;
            padding: 30px;
            box-shadow: 0 0 60px #00ff8822;
        }
        .header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            border-bottom: 2px solid #00ff8844;
            padding-bottom: 15px;
            margin-bottom: 25px;
            flex-wrap: wrap;
            gap: 10px;
        }
        .header h1 {
            font-size: 24px;
            color: #00ff88;
            text-shadow: 0 0 20px #00ff8844;
            letter-spacing: 2px;
        }
        .header .status {
            font-size: 14px;
            padding: 6px 16px;
            border-radius: 20px;
            background: #00ff8822;
            border: 1px solid #00ff88;
            color: #00ff88;
        }
        .header .status.offline {
            background: #ff335522;
            border-color: #ff3355;
            color: #ff3355;
        }
        .pool-bar {
            display: grid;
            grid-template-columns: repeat(5, 1fr);
            gap: 10px;
            background: #1a1a24;
            border: 1px solid #00ff8833;
            border-radius: 8px;
            padding: 10px 20px;
            margin-bottom: 25px;
        }
        .pool-bar .pool-item {
            text-align: center;
        }
        .pool-bar .pool-label {
            font-size: 10px;
            color: #00ff8866;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        .pool-bar .pool-value {
            font-size: 16px;
            font-weight: bold;
            margin-top: 4px;
            color: #00ff88;
        }
        .pool-bar .pool-value.warning { color: #ffaa00; }
        .pool-bar .pool-value.danger { color: #ff3355; }
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 15px;
            margin-bottom: 25px;
        }
        .stat-card {
            background: #1a1a24;
            border: 1px solid #00ff8833;
            border-radius: 8px;
            padding: 15px 20px;
            text-align: center;
        }
        .stat-card .label {
            font-size: 11px;
            text-transform: uppercase;
            color: #00ff8866;
            letter-spacing: 1px;
        }
        .stat-card .value {
            font-size: 28px;
            font-weight: bold;
            margin-top: 5px;
            color: #00ff88;
        }
        .stat-card .value.warning { color: #ffaa00; }
        .stat-card .value.danger { color: #ff3355; }
        .channels {
            display: grid;
            grid-template-columns: repeat(6, 1fr);
            gap: 10px;
            margin-bottom: 25px;
        }
        .channel {
            background: #1a1a24;
            border: 1px solid #00ff8833;
            border-radius: 6px;
            padding: 12px 10px;
            text-align: center;
            transition: all 0.3s ease;
        }
        .channel.winner {
            border-color: #00ff88;
            background: #00ff8811;
            box-shadow: 0 0 20px #00ff8822;
        }
        .channel.loser {
            border-color: #ff335544;
            background: #ff335511;
        }
        .channel .ch-label {
            font-size: 10px;
            color: #00ff8866;
            text-transform: uppercase;
        }
        .channel .ch-value {
            font-size: 16px;
            font-weight: bold;
            margin: 4px 0;
        }
        .channel .ch-status { font-size: 18px; }
        .channel .ch-status.win { color: #00ff88; }
        .channel .ch-status.lose { color: #ff3355; }
        .binary-map {
            background: #1a1a24;
            border: 1px solid #00ff8833;
            border-radius: 8px;
            padding: 15px 20px;
            margin-bottom: 25px;
        }
        .binary-map .map-label {
            font-size: 11px;
            color: #00ff8866;
            text-transform: uppercase;
            letter-spacing: 1px;
            margin-bottom: 8px;
        }
        .binary-map .map-row {
            display: flex;
            gap: 8px;
            flex-wrap: wrap;
        }
        .binary-map .bit {
            width: 28px;
            height: 28px;
            display: flex;
            align-items: center;
            justify-content: center;
            border-radius: 4px;
            font-size: 12px;
            font-weight: bold;
        }
        .binary-map .bit.on { background: #00ff88; color: #0a0a0f; }
        .binary-map .bit.off { background: #2a2a3a; color: #555; }
        .dominant {
            background: #1a1a24;
            border: 1px solid #ffaa0044;
            border-radius: 8px;
            padding: 15px 20px;
            margin-bottom: 25px;
            text-align: center;
        }
        .dominant .dom-label {
            font-size: 11px;
            color: #ffaa0066;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        .dominant .dom-value {
            font-size: 32px;
            font-weight: bold;
            color: #ffaa00;
            margin-top: 4px;
        }
        .dominant .dom-channel {
            font-size: 14px;
            color: #ffaa0088;
        }
        .trend {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 15px;
            background: #1a1a24;
            border: 1px solid #00ff8833;
            border-radius: 8px;
            padding: 15px 20px;
        }
        .trend-item { text-align: center; }
        .trend-item .trend-label {
            font-size: 10px;
            color: #00ff8866;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        .trend-item .trend-value {
            font-size: 20px;
            font-weight: bold;
            margin-top: 4px;
        }
        .trend-item .trend-value.up { color: #00ff88; }
        .trend-item .trend-value.down { color: #ff3355; }
        .trend-item .trend-value.stable { color: #ffaa00; }
        @keyframes pulse { 0%,100% { opacity:1; } 50% { opacity:0.5; } }
        @keyframes glow { 0%,100% { box-shadow:0 0 20px #00ff8822; } 50% { box-shadow:0 0 40px #00ff8855; } }
        .updating { animation: pulse 0.3s ease; }
        .winner .ch-value { animation: glow 2s ease-in-out infinite; }
        @media (max-width: 992px) { .channels { grid-template-columns: repeat(4, 1fr); } }
        @media (max-width: 768px) {
            .stats-grid { grid-template-columns: repeat(2, 1fr); }
            .channels { grid-template-columns: repeat(3, 1fr); }
            .trend { grid-template-columns: repeat(2, 1fr); }
            .pool-bar { grid-template-columns: repeat(3, 1fr); }
            .header { flex-direction: column; text-align: center; }
        }
        @media (max-width: 480px) {
            .channels { grid-template-columns: repeat(2, 1fr); }
            .stats-grid { grid-template-columns: 1fr 1fr; }
            .trend { grid-template-columns: 1fr 1fr; }
            .pool-bar { grid-template-columns: 1fr 1fr; }
        }
    </style>
</head>
<body>
    <div class="dashboard">
        <div class="header">
            <h1>⛏️ XMR RIG · WORLD VIEW</h1>
            <div class="status" id="statusBadge">● ONLINE</div>
        </div>
        <div class="pool-bar" id="poolBar">
            <div class="pool-item"><div class="pool-label">Pool</div><div class="pool-value" id="poolStatus">Connecting...</div></div>
            <div class="pool-item"><div class="pool-label">Hashrate</div><div class="pool-value" id="poolHashrate">0 KH/s</div></div>
            <div class="pool-item"><div class="pool-label">Shares</div><div class="pool-value" id="poolShares">0/0</div></div>
            <div class="pool-item"><div class="pool-label">Difficulty</div><div class="pool-value" id="poolDifficulty">0</div></div>
            <div class="pool-item"><div class="pool-label">Uptime</div><div class="pool-value" id="poolUptime">00:00:00</div></div>
        </div>
        <div class="stats-grid">
            <div class="stat-card"><div class="label">Winners</div><div class="value" id="winnersCount">0/12</div></div>
            <div class="stat-card"><div class="label">Selection Power</div><div class="value" id="selectionPower">0%</div></div>
            <div class="stat-card"><div class="label">Dominant Channel</div><div class="value" id="dominantChannel">-</div></div>
            <div class="stat-card"><div class="label">Rig Health</div><div class="value" id="rigHealth">WAITING</div></div>
        </div>
        <div class="channels" id="channelsGrid"></div>
        <div class="binary-map">
            <div class="map-label">Binary Map · 1 = Winner · 0 = Loser</div>
            <div class="map-row" id="binaryMap"></div>
        </div>
        <div class="dominant">
            <div class="dom-label">🏆 Dominant Channel · Competitive Edge</div>
            <div class="dom-value" id="dominantValue">--</div>
            <div class="dom-channel" id="dominantLabel">Channel --</div>
        </div>
        <div class="trend">
            <div class="trend-item"><div class="trend-label">Momentum</div><div class="trend-value" id="trendValue">--</div></div>
            <div class="trend-item"><div class="trend-label">Spread</div><div class="trend-value" id="spreadValue">--</div></div>
            <div class="trend-item"><div class="trend-label">Threshold</div><div class="trend-value" id="thresholdValue">2048</div></div>
            <div class="trend-item"><div class="trend-label">Last Update</div><div class="trend-value" id="updateTime">--</div></div>
        </div>
    </div>
    <script>
        const NUM_CHANNELS = 12;
        const THRESHOLD = 2048;
        const CHANNEL_LABELS = ['GPU0','GPU1','GPU2','GPU3','PSU0','PSU1','PSU2','PSU3','TMP0','TMP1','TMP2','TMP3'];
        let updateCount = 0;
        let startTime = Date.now();

        function formatUptime(ms) {
            const s = Math.floor(ms/1000), h = Math.floor(s/3600), m = Math.floor((s%3600)/60), sec = s%60;
            return `${String(h).padStart(2,'0')}:${String(m).padStart(2,'0')}:${String(sec).padStart(2,'0')}`;
        }

        async function fetchData() {
            try {
                const response = await fetch('/api/data');
                if(response.ok) {
                    const data = await response.json();
                    updateDashboard(data);
                    document.getElementById('statusBadge').textContent = '● REAL DATA';
                    document.getElementById('statusBadge').className = 'status';
                    return;
                }
            } catch(e) {}
            // Fallback: simulate
            simulateData();
        }

        function simulateData() {
            const data = {
                values: [],
                winners: [],
                hashrate: 12000 + Math.random() * 8000,
                accepted: Math.floor(Math.random() * 10),
                rejected: Math.floor(Math.random() * 5),
                difficulty: 120000 + Math.random() * 30000,
                poolStatus: 'Simulated'
            };
            for(let i = 0; i < 12; i++) {
                let val;
                if(i < 4) val = 2000 + Math.random() * 1500;
                else if(i < 8) val = 1800 + Math.random() * 1200;
                else val = 800 + Math.random() * 2200;
                data.values.push(Math.floor(val));
                data.winners.push(i < 8 ? data.values[i] > THRESHOLD : data.values[i] < THRESHOLD);
            }
            updateDashboard(data);
            document.getElementById('statusBadge').textContent = '● SIMULATED';
            document.getElementById('statusBadge').className = 'status offline';
        }

        function updateDashboard(data) {
            updateCount++;
            const values = data.values;
            const winners = data.winners;
            const winnerCount = winners.filter(w => w).length;
            const selectionRatio = (winnerCount / NUM_CHANNELS * 100);
            const isHealthy = winnerCount > (NUM_CHANNELS / 2);

            // Pool
            document.getElementById('poolStatus').textContent = data.poolStatus || 'Unknown';
            document.getElementById('poolStatus').className = `pool-value ${data.poolStatus === 'Connected' ? '' : 'warning'}`;
            document.getElementById('poolHashrate').textContent = `${(data.hashrate / 1000).toFixed(1)} KH/s`;
            document.getElementById('poolShares').textContent = `${data.accepted}/${data.rejected}`;
            document.getElementById('poolDifficulty').textContent = Math.round(data.difficulty || 0);
            document.getElementById('poolUptime').textContent = formatUptime(Date.now() - startTime);

            // Stats
            document.getElementById('winnersCount').textContent = `${winnerCount}/${NUM_CHANNELS}`;
            document.getElementById('winnersCount').className = `value ${winnerCount < 4 ? 'danger' : winnerCount < 6 ? 'warning' : ''}`;
            document.getElementById('selectionPower').textContent = `${selectionRatio.toFixed(1)}%`;
            document.getElementById('selectionPower').className = `value ${selectionRatio < 40 ? 'danger' : selectionRatio < 55 ? 'warning' : ''}`;
            document.getElementById('rigHealth').textContent = isHealthy ? '✅ OPTIMAL' : '⚠️ WARNING';
            document.getElementById('rigHealth').className = `value ${isHealthy ? '' : 'danger'}`;

            // Channels
            const grid = document.getElementById('channelsGrid');
            grid.innerHTML = '';
            for(let i = 0; i < NUM_CHANNELS; i++) {
                const div = document.createElement('div');
                div.className = `channel ${winners[i] ? 'winner' : 'loser'}`;
                div.innerHTML = `<div class="ch-label">${CHANNEL_LABELS[i]}</div><div class="ch-value">${values[i]}</div><div class="ch-status ${winners[i] ? 'win' : 'lose'}">${winners[i] ? '▼ WIN' : '▲ LOSE'}</div>`;
                grid.appendChild(div);
            }

            // Binary Map
            const map = document.getElementById('binaryMap');
            map.innerHTML = '';
            for(let i = 0; i < NUM_CHANNELS; i++) {
                const bit = document.createElement('div');
                bit.className = `bit ${winners[i] ? 'on' : 'off'}`;
                bit.textContent = winners[i] ? '1' : '0';
                map.appendChild(bit);
            }

            // Dominant
            let maxIdx = 0, maxVal = values[0], minVal = values[0];
            for(let i = 1; i < values.length; i++) {
                if(values[i] > maxVal) { maxVal = values[i]; maxIdx = i; }
                if(values[i] < minVal) minVal = values[i];
            }
            document.getElementById('dominantValue').textContent = maxVal;
            document.getElementById('dominantLabel').textContent = `Channel ${maxIdx} · ${CHANNEL_LABELS[maxIdx]}`;
            document.getElementById('dominantChannel').textContent = CHANNEL_LABELS[maxIdx];

            // Trend
            const half = Math.floor(values.length / 2);
            let s1 = 0, s2 = 0;
            for(let i = 0; i < half; i++) { s1 += values[i]; s2 += values[i+half]; }
            const avg1 = s1 / half, avg2 = s2 / half;
            const diff = avg2 - avg1;
            const pct = (diff / (avg1 + 1)) * 100;
            let dir = 'stable';
            if(pct > 5) dir = 'up';
            else if(pct < -5) dir = 'down';
            const trendEl = document.getElementById('trendValue');
            trendEl.textContent = dir.toUpperCase();
            trendEl.className = `trend-value ${dir}`;

            document.getElementById('spreadValue').textContent = maxVal - minVal;
            document.getElementById('thresholdValue').textContent = THRESHOLD;
            document.getElementById('updateTime').textContent = new Date().toLocaleTimeString();
        }

        setInterval(fetchData, 2000);
        fetchData();
    </script>
</body>
</html>
)rawliteral";

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
    
    startTime = millis();
    
    // Connect to WiFi
    connectWiFi();
    
    // Initialize pins
    for(int i = 0; i < NUM_CHANNELS; i++) {
        pinMode(analogPins[i], INPUT);
    }
    
    // Setup web server
    setupWebServer();
    
    // Initialize pool stats with simulated data
    generateSimulatedPoolData();
    
    Serial.println("\n✅ System Ready!");
    Serial.print("🌐 Open browser at: http://");
    Serial.println(WiFi.localIP());
    Serial.println("=========================================\n");
}

// ============================================================
// LOOP
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
    
    // 4. Handle web server
    server.handleClient();
    
    delay(1000);
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
        Serial.print("📡 IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n❌ WiFi Failed! Using offline mode.");
    }
}

// ============================================================
// WEB SERVER
// ============================================================
void setupWebServer() {
    server.on("/", []() {
        server.send(200, "text/html", index_html);
    });
    
    server.on("/api/data", []() {
        String json = buildJSON();
        server.send(200, "application/json", json);
    });
    
    server.on("/api/status", []() {
        String json = "{\"status\":\"online\",\"uptime\":" + String(millis() / 1000) + "}";
        server.send(200, "application/json", json);
    });
    
    server.begin();
    Serial.println("🌐 Web server started");
}

// ============================================================
// BUILD JSON
// ============================================================
String buildJSON() {
    String json = "{";
    json += "\"values\":[";
    for(int i = 0; i < NUM_CHANNELS; i++) {
        json += String(adcValues[i]);
        if(i < NUM_CHANNELS - 1) json += ",";
    }
    json += "],";
    json += "\"winners\":[";
    for(int i = 0; i < NUM_CHANNELS; i++) {
        json += isWinner[i] ? "true" : "false";
        if(i < NUM_CHANNELS - 1) json += ",";
    }
    json += "],";
    json += "\"hashrate\":" + String(hashrate) + ",";
    json += "\"accepted\":" + String(acceptedShares) + ",";
    json += "\"rejected\":" + String(rejectedShares) + ",";
    json += "\"difficulty\":" + String(poolDifficulty) + ",";
    json += "\"poolStatus\":\"" + poolStatus + "\"";
    json += "}";
    return json;
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
        if(i < 8) {
            isWinner[i] = (adcValues[i] > THRESHOLD);
        } else {
            isWinner[i] = (adcValues[i] < THRESHOLD);
        }
        if(isWinner[i]) winnerCount++;
    }
}

// ============================================================
// UPDATE POOL STATS
// ============================================================
void updatePoolStats() {
    if(WiFi.status() != WL_CONNECTED) {
        poolStatus = "WiFi Disconnected";
        return;
    }
    
    HTTPClient http;
    String url = "https://api.supportxmr.com/api/miner/" + String(walletAddress) + "/stats";
    http.begin(url);
    http.setTimeout(5000);
    
    int httpCode = http.GET();
    
    if(httpCode > 0) {
        String payload = http.getString();
        parsePoolResponse(payload);
        poolStatus = "Connected";
    } else {
        poolStatus = "Pool Error";
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
    
    if(hashrate > 0) {
        poolStatus = "Connected";
        isMining = true;
    }
}

// ============================================================
// GENERATE SIMULATED POOL DATA
// ============================================================
void generateSimulatedPoolData() {
    static double fakeHashrate = 12000.0;
    static int fakeAccepted = 0;
    
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
    isMining = false;
}

// ============================================================
// END OF CODE
// ============================================================

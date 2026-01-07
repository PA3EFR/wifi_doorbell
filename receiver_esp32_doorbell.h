/**
 * ESP32 Remote Deurbel - Ontvanger (Zolder)
 * ============================================
 * 
 * Dit is de ontvangereenheid die op zolder wordt geplaatst.
 * Het systeem fungeert als HTTP-webserver en luistert naar GET-requests
 * op het pad /ring van de zender.
 * 
 * Na ontvangst van een geldige request wordt de melodie geactiveerd
 * en wordt een bevestigingsteruggezonden naar de zender.
 * 
 * Functionaliteiten:
 * - HTTP-webserver voor signaalontvangst
 * - Non-blocking melodie afspeel functie met Arduino tone()
 * - Vier-tonige melodie: C, E, G, High C
 * - Automatische WiFi herverbinding bij verbindingsverlies
 * - Visuele LED feedback
 * - Bevestiging terugsturen naar zender via HTTP response
 * - Deurbel-indicator LED knippert 60s na elke activatie
 * 
 * Hardware: ESP32 Lite bordje
 * Pin aansluitingen:
 *   - GPIO 16: Passieve buzzer (melodie)
 *   - GPIO 22: Status LED / Deurbel-indicator
 *   - GPIO 23: Netwerk status LED
 * 
 * Auteur: MiniMax Agent
 * Datum: Januari 2026
 */

// ============================================
// CONFIGURATIE - AANPASSEN AAN UW NETWERK
// ============================================

// WiFi-instellingen
const char* ssid = "Scouternet_Attick_24";              // Uw WiFi-netwerknaam
const char* password = "22832115";                      // Uw WiFi-wachtwoord

// Netwerkinstellingen (statische IP)
IPAddress gateway(192, 168, 170, 1);                    // IP van uw router
IPAddress subnet(255, 255, 255, 0);                     // Subnet mask
IPAddress dns(8, 8, 8, 8);                              // Google DNS (fallback)

// Statische IP-adressen voor de ESP32's
IPAddress ip_sender(192, 168, 170, 201);                // Zender (voordeur)
IPAddress ip_receiver(192, 168, 170, 202);              // Ontvanger (zolder)

// HTTP-instellingen
const int httpPort = 80;                                // HTTP poort
const char* doorbellPath = "/ring";                     // URL path voor deurbel signaal

// ============================================
// PIN EN BUZZER CONFIGURATIE
// ============================================

const int BUZZER_PIN = 16;                              // Buzzer op GPIO 16
const int RECEIVER_LED_PIN = 22;                        // Status LED op GPIO 22
const int NETWORK_LED_PIN = 23;                         // Netwerk status LED op GPIO 23

// ============================================
// MELODIE DEFINITIES
// ============================================
// Frequencies in Hz (A4 = 440 Hz standaard)
const int NOTE_C4 = 262;                                // Midden C (Do)
const int NOTE_E4 = 330;                                // E (Mi)
const int NOTE_G4 = 392;                                // G (Sol)
const int NOTE_C5 = 523;                                // Hoge C (Do octaaf hoger)

// Melodie structuur: {frequentie, duur_in_ms}
struct Note {
    int frequency;
    unsigned long duration;
};

const Note MELODY[] = {
    {NOTE_C4, 200},     // C - 0.5 seconden
    {NOTE_E4, 200},     // E - 0.5 seconden
    {NOTE_G4, 200},     // G - 0.5 seconden
    {NOTE_C5, 400}     // Hoge C - 4 seconden
};

const int MELODY_LENGTH = sizeof(MELODY) / sizeof(MELODY[0]);

// ============================================
// OVERIGE VARIABELEN (NIET AANPASSEN)
// ============================================

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

// WiFi variabelen
WiFiServer server(httpPort);
WiFiClient client;

// WiFi status tracking
bool wifiWasConnected = false;

// Melodie afspeel variabelen
unsigned long melodyStartTime = 0;
int currentNoteIndex = 0;
bool isPlayingMelody = false;

// Deurbel-indicator variabelen
bool doorbellIndicatorActive = false;
unsigned long doorbellIndicatorStartTime = 0;
const unsigned long DOORBELL_INDICATOR_DURATION = 60000; // 60 seconden
const unsigned long DOORBELL_LED_INTERVAL = 500;         // 500ms aan, 500ms uit (1 Hz)

void setup() {
    // Seriële communicatie starten
    Serial.begin(115200);
    Serial.println();
    Serial.println("========================================");
    Serial.println("   ESP32 Deurbel Ontvanger - Zolder     ");
    Serial.println("========================================");
    Serial.println("Opstarten...");
    Serial.println();
    
    // Pinnen configureren
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);                      // Buzzer uit bij opstarten
    
    pinMode(RECEIVER_LED_PIN, OUTPUT);
    pinMode(NETWORK_LED_PIN, OUTPUT);
    digitalWrite(RECEIVER_LED_PIN, LOW);                // LED uit bij opstarten
    digitalWrite(NETWORK_LED_PIN, LOW);                 // Netwerk LED uit bij opstarten
    
    Serial.println("Pinnen geconfigureerd:");
    Serial.print("  - Buzzer: GPIO ");
    Serial.println(BUZZER_PIN);
    Serial.print("  - Status/Indicator LED: GPIO ");
    Serial.println(RECEIVER_LED_PIN);
    Serial.print("  - Netwerk status LED: GPIO ");
    Serial.println(NETWORK_LED_PIN);
    Serial.println();
    
    // Status LED knipperen tijdens WiFi verbinding
    Serial.println("Verbinden met WiFi...");
    for (int i = 0; i < 10; i++) {
        digitalWrite(RECEIVER_LED_PIN, !digitalRead(RECEIVER_LED_PIN));
        delay(100);
    }
    
    // Statisch IP configureren
    Serial.println("Statische IP configureren...");
    if (!WiFi.config(ip_receiver, gateway, subnet, dns)) {
        Serial.println("FOUT: Kon statische IP niet configureren!");
        while (true);                                  // Blokkeer bij fout
    }
    Serial.print("  - IP adres: ");
    Serial.println(ip_receiver);
    Serial.print("  - Gateway: ");
    Serial.println(gateway);
    Serial.println();
    
    // Verbinden met WiFi
    Serial.print("Verbinden met WiFi-netwerk: ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);
    
    // Wachten tot verbonden
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        digitalWrite(RECEIVER_LED_PIN, !digitalRead(RECEIVER_LED_PIN)); // Knipper tijdens verbinden
    }
    
    // Verbonden - LED aan
    digitalWrite(RECEIVER_LED_PIN, HIGH);
    digitalWrite(NETWORK_LED_PIN, HIGH);                // Netwerk LED permanent aan
    wifiWasConnected = true;
    Serial.println();
    Serial.println();
    Serial.println("WiFi verbonden!");
    Serial.println("----------------------------------------");
    Serial.print("IP adres: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC adres: ");
    Serial.println(WiFi.macAddress());
    Serial.print("HTTP Server gestart op: http://");
    Serial.print(ip_receiver);
    Serial.print(doorbellPath);
    Serial.print(":");
    Serial.println(httpPort);
    Serial.print("Verwacht signalen van: ");
    Serial.println(ip_sender);
    Serial.println("----------------------------------------");
    
    // HTTP server starten
    server.begin();
    Serial.println();
    Serial.println("Systeem is klaar voor gebruik!");
    Serial.println();
}

void loop() {
    // WiFi verbindingsstatus controleren
    if (WiFi.status() != WL_CONNECTED) {
        if (wifiWasConnected) {
            Serial.println("Waarschuwing: WiFi verbinding verbroken!");
            wifiWasConnected = false;
        }
        handleDisconnection();
        return;
    } else {
        if (!wifiWasConnected) {
            Serial.println("WiFi weer verbonden!");
            wifiWasConnected = true;
            digitalWrite(NETWORK_LED_PIN, HIGH);        // Netwerk LED weer inschakelen
            server.begin();                            // HTTP server opnieuw starten na reconnect
        }
    }
    
    // Controleren of er een HTTP-client verbinding maakt
    client = server.available();
    
    if (client) {
        Serial.println("Client verbonden!");
        
        // Wacht tot er data beschikbaar is
        while (!client.available()) {
            delay(1);
        }
        
        // Lees de request
        String request = client.readStringUntil('\r');
        Serial.print("Request ontvangen: ");
        Serial.println(request);
        
        // Controleer of het een GET request is op /ring
        if (request.indexOf("GET /ring") >= 0 || request.indexOf("GET /ring ") >= 0) {
            Serial.println(">>> DEURBEL SIGNAAL ONTVANGEN! <<<");
            startMelody();
            
            // Stuur HTTP 200 OK response met QSL bevestiging
            Serial.println("Versturen van bevestiging (QSL)...");
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.println();
            client.println("QSL");
            
            // Start deurbel-indicator (LED knippert 60 seconden)
            startDoorbellIndicator();
        } else {
            // Onbekende request, stuur 404
            client.println("HTTP/1.1 404 Not Found");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.println();
            client.println("Not Found");
        }
        
        // Kleine vertraging en verbinding sluiten
        delay(100);
        client.stop();
        Serial.println("Client verwerkt en verbroken");
    }
    
    // Non-blocking melodie update
    updateMelody();
    
    // Non-blocking deurbel-indicator update
    updateDoorbellIndicator();
}

void startMelody() {
    // Voorkom dat melodie opnieuw start als hij al speelt
    if (!isPlayingMelody) {
        Serial.println();
        Serial.println("========================================");
        Serial.println("         *** DING DONG! ***            ");
        Serial.println("========================================");
        Serial.println();
        
        Serial.println("Start melodie afspeel:");
        Serial.print("  - Aantal noten: ");
        Serial.println(MELODY_LENGTH);
        
        // Eerste noot starten
        currentNoteIndex = 0;
        melodyStartTime = millis();
        isPlayingMelody = true;
        
        // Speel eerste noot met tone()
        const Note& firstNote = MELODY[currentNoteIndex];
        Serial.print("  - Noot 1: ");
        Serial.print(getNoteName(firstNote.frequency));
        Serial.print(" (");
        Serial.print(firstNote.frequency);
        Serial.print(" Hz) - ");
        Serial.print(firstNote.duration);
        Serial.println(" ms");
        
        tone(BUZZER_PIN, firstNote.frequency);
        
        digitalWrite(RECEIVER_LED_PIN, LOW);             // LED uit tijdens melodie
    } else {
        Serial.println("Melodie wordt al afgespeeld, signaal wordt nog steeds verwerkt");
    }
}

void updateMelody() {
    if (!isPlayingMelody) return;
    
    unsigned long elapsedTime = millis() - melodyStartTime;
    const Note& currentNote = MELODY[currentNoteIndex];
    
    // Controleer of de huidige noot is afgelopen
    if (elapsedTime >= currentNote.duration) {
        // Ga naar de volgende noot
        currentNoteIndex++;
        
        // Reset timer voor de volgende noot
        melodyStartTime = millis();
        
        // Check of we alle noten hebben gehad
        if (currentNoteIndex >= MELODY_LENGTH) {
            // Melodie is klaar
            stopMelody();
            return;
        }
        
        // Speel de volgende noot af met tone()
        const Note& nextNote = MELODY[currentNoteIndex];
        Serial.print("  - Noot ");
        Serial.print(currentNoteIndex + 1);
        Serial.print(": ");
        Serial.print(getNoteName(nextNote.frequency));
        Serial.print(" (");
        Serial.print(nextNote.frequency);
        Serial.print(" Hz) - ");
        Serial.print(nextNote.duration);
        Serial.println(" ms");
        
        tone(BUZZER_PIN, nextNote.frequency);
    }
}

void stopMelody() {
    isPlayingMelody = false;
    currentNoteIndex = 0;
    
    // Buzzer uitschakelen
    noTone(BUZZER_PIN);
    
    Serial.println("  - Melodie voltooid!");
    Serial.println("========================================");
    Serial.println();
}

void startDoorbellIndicator() {
    // Start deurbel-indicator LED knipperen
    doorbellIndicatorActive = true;
    doorbellIndicatorStartTime = millis();
    digitalWrite(RECEIVER_LED_PIN, HIGH);               // LED aan bij start
    
    Serial.println("Deurbel-indicator geactiveerd (60s knipperen)");
}

void updateDoorbellIndicator() {
    if (!doorbellIndicatorActive) return;
    
    unsigned long elapsedTime = millis() - doorbellIndicatorStartTime;
    
    // Controleer of de 60 seconden zijn verstreken
    if (elapsedTime >= DOORBELL_INDICATOR_DURATION) {
        // Indicator uitschakelen
        doorbellIndicatorActive = false;
        digitalWrite(RECEIVER_LED_PIN, HIGH);           // LED weer aan (ruststand)
        Serial.println("Deurbel-indicator beeindigd");
        return;
    }
    
    // LED knipperen met 1 Hz frequentie (500ms aan, 500ms uit)
    unsigned long cycleTime = elapsedTime % 1000;
    
    if (cycleTime < 500) {
        digitalWrite(RECEIVER_LED_PIN, HIGH);           // LED aan
    } else {
        digitalWrite(RECEIVER_LED_PIN, LOW);            // LED uit
    }
}

String getNoteName(int frequency) {
    // Converteer frequentie naar notennaam
    switch (frequency) {
        case NOTE_C4:  return "C (Do)";
        case NOTE_E4:  return "E (Mi)";
        case NOTE_G4:  return "G (Sol)";
        case NOTE_C5:  return "High C (Do hoog)";
        default:       return "Onbekend";
    }
}

void handleDisconnection() {
    static unsigned long lastReconnectAttempt = 0;
    
    // LED knippert langzaam bij verbindingsproblemen
    if (millis() - lastReconnectAttempt > 1000) {
        lastReconnectAttempt = millis();
        digitalWrite(RECEIVER_LED_PIN, !digitalRead(RECEIVER_LED_PIN));
        digitalWrite(NETWORK_LED_PIN, LOW);            // Netwerk LED uit tijdens verbindingsproblemen
    }
    
    // Wacht even en probeer opnieuw
    delay(100);
    
    WiFi.disconnect();
    WiFi.reconnect();
}

/**
 * ESP32 Remote Deurbel - Zender (Voordeur)
 * ============================================
 * 
 * Dit is de zendereenheid die bij de voordeur wordt geplaatst.
 * Wanneer op de drukknop wordt gedrukt, verstuurt dit systeem
 * een UDP "RING" signaal naar de ontvanger op zolder.
 * 
 * Functionaliteiten:
 * - Drukknop detectie met software debouncing
 * - Anti-spam beveiliging (2 seconden wachttijd tussen signalen)
 * - Redundante signaalverzending (3 pakketten voor betrouwbaarheid)
 * - Automatische WiFi herverbinding bij verbindingsverlies
 * - Visuele LED feedback
 * 
 * Hardware: ESP32 Lite bordje
 * Pin aansluitingen:
 *   - GPIO 13: Drukknop (met interne pull-up)
 *   - GPIO 22: Status LED
 * 
 * Auteur: MiniMax Agent
 * Datum: December 2025
 */

// ============================================
// CONFIGURATIE - AANPASSEN AAN UW NETWERK
// ============================================

// WiFi-instellingen
const char* ssid = "UW_WIFI_SSID";                    // <<< Vul uw WiFi-netwerknaam in
const char* password = "UW_WIFI_WACHTWOORD";          // <<< Vul uw WiFi-wachtwoord in

// Netwerkinstellingen (statische IP)
IPAddress gateway(192, 168, 2, 254);                  // IP van uw router
IPAddress subnet(255, 255, 255, 0);                   // Subnet mask
IPAddress dns(8, 8, 8, 8);                            // Google DNS (fallback)

// Statische IP-adressen voor de ESP32's
IPAddress ip_sender(192, 168, 2, 201);                // Zender (voordeur)
IPAddress ip_receiver(192, 168, 2, 202);              // Ontvanger (zolder)

// UDP-instellingen
const int udpPort = 4210;                             // Poort voor communicatie
const char* doorbellPayload = "RING";                 // Signaal payload

// Pin definities
const int BUTTON_PIN = 13;                            // Drukknop op GPIO 13
const int SENDER_LED_PIN = 22;                        // Status LED op GPIO 22

// ============================================
// OVERIGE VARIABELEN (NIET AANPASSEN)
// ============================================

#include <WiFi.h>
#include <WiFiUdp.h>

WiFiUDP udp;

// Pin status variabelen
int lastButtonState = HIGH;                           // Vorige knopstatus (HIGH = niet ingedrukt)
unsigned long lastDebounceTime = 0;                   // Tijd van laatste statusverandering
bool buttonProcessed = false;                         // Flag om herhaalde signalering te voorkomen

// Timings
const unsigned long DEBOUNCE_DELAY = 50;              // Debounce tijd in milliseconden
const unsigned long ANTI_SPAM_DELAY = 2000;           // Minimum tijd tussen signalen
unsigned long lastSignalTime = 0;                     // Tijd van laatste verzonden signaal

void setup() {
    // Seriële communicatie starten voor debugging
    Serial.begin(115200);
    Serial.println();
    Serial.println("========================================");
    Serial.println("   ESP32 Deurbel Zender - Voordeur     ");
    Serial.println("========================================");
    Serial.println("Opstarten...");
    Serial.println();
    
    // Pinnen configureren
    pinMode(BUTTON_PIN, INPUT_PULLUP);                // Interne pull-up weerstand inschakelen
    pinMode(SENDER_LED_PIN, OUTPUT);
    digitalWrite(SENDER_LED_PIN, LOW);                // LED uit bij opstarten
    
    Serial.println("Pinnen geconfigureerd:");
    Serial.print("  - Drukknop: GPIO ");
    Serial.println(BUTTON_PIN);
    Serial.print("  - Status LED: GPIO ");
    Serial.println(SENDER_LED_PIN);
    Serial.println();
    
    // Status LED knipperen tijdens WiFi verbinding
    Serial.println("Verbinden met WiFi...");
    for (int i = 0; i < 10; i++) {
        digitalWrite(SENDER_LED_PIN, !digitalRead(SENDER_LED_PIN));
        delay(100);
    }
    
    // Statisch IP configureren
    Serial.println("Statische IP configureren...");
    if (!WiFi.config(ip_sender, gateway, subnet, dns)) {
        Serial.println("FOUT: Kon statische IP niet configureren!");
        while (true);                                // Blokkeer bij fout
    }
    Serial.print("  - IP adres: ");
    Serial.println(ip_sender);
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
        digitalWrite(SENDER_LED_PIN, !digitalRead(SENDER_LED_PIN)); // Knipper tijdens verbinden
    }
    
    // Verbonden - LED aan
    digitalWrite(SENDER_LED_PIN, HIGH);
    Serial.println();
    Serial.println();
    Serial.println("WiFi verbonden!");
    Serial.println("----------------------------------------");
    Serial.print("IP adres: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC adres: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Zend naar: ");
    Serial.print(ip_receiver);
    Serial.print(":");
    Serial.println(udpPort);
    Serial.println("----------------------------------------");
    Serial.println("Systeem is klaar voor gebruik!");
    Serial.println();
}

void loop() {
    // WiFi verbindingsstatus controleren
    if (WiFi.status() != WL_CONNECTED) {
        handleDisconnection();
        return;
    }
    
    // Drukknop uitlezen
    int reading = digitalRead(BUTTON_PIN);
    
    // Debounce logica: als de status is veranderd, reset de timer
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }
    
    // Na de debounce-tijd: is de status nog steeds stabiel?
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
        // Knop is ingedrukt (LOW bij pull-up) en nog niet verwerkt
        if (reading == LOW && !buttonProcessed) {
            // Check anti-spam timing
            if (millis() - lastSignalTime > ANTI_SPAM_DELAY) {
                sendDoorbellSignal();
                lastSignalTime = millis();
            } else {
                Serial.println("Anti-spam: signaal geblokkeerd (nog geen 2 seconden)");
            }
            buttonProcessed = true;
        }
        
        // Reset de verwerkingsflag wanneer knop wordt losgelaten
        if (reading == HIGH) {
            buttonProcessed = false;
        }
    }
    
    // Huidige status opslaan voor volgende iteratie
    lastButtonState = reading;
}

void sendDoorbellSignal() {
    Serial.println(">>> Deurbel ingedrukt! Signaal wordt verzonden...");
    
    // Verstuur het signaal 3 keer voor redundantie (UDP is niet gegarandeerd)
    for (int i = 0; i < 3; i++) {
        udp.beginPacket(ip_receiver, udpPort);
        udp.print(doorbellPayload);
        udp.endPacket();
        
        // Visuele feedback: korte LED flits
        digitalWrite(SENDER_LED_PIN, LOW);
        delay(50);
        digitalWrite(SENDER_LED_PIN, HIGH);
        delay(50);
        
        Serial.print("  Pakket ");
        Serial.print(i + 1);
        Serial.println(" verzonden");
    }
    
    Serial.println(">>> Alle signalen verzonden naar ontvanger");
    Serial.print("  Doel: ");
    Serial.print(ip_receiver);
    Serial.print(":");
    Serial.println(udpPort);
    Serial.println();
}

void handleDisconnection() {
    static unsigned long lastReconnectAttempt = 0;
    
    // Elke 5 seconden opnieuw proberen
    if (millis() - lastReconnectAttempt > 5000) {
        lastReconnectAttempt = millis();
        Serial.println("WiFi verbinding verloren! Opnieuw verbinden...");
        
        digitalWrite(SENDER_LED_PIN, LOW);            // LED uit bij verbindingsproblemen
        
        WiFi.disconnect();
        WiFi.reconnect();
    }
}

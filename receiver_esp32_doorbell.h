/**
 * ESP32 Remote Deurbel - Ontvanger (Zolder)
 * ============================================
 * 
 * Dit is de ontvangereenheid die op zolder wordt geplaatst.
 * Het systeem luistert continu naar UDP "RING" signalen van
 * de zender en activeert de zoemer wanneer een signaal wordt ontvangen.
 * 
 * Functionaliteiten:
 * - Continues UDP signaalontvangst
 * - Non-blocking zoemer aansturing (blokkeert niet)
 * - Automatische WiFi herverbinding bij verbindingsverlies
 * - Visuele LED feedback
 * 
 * Hardware: ESP32 Lite bordje
 * Pin aansluitingen:
 *   - GPIO 16: Actieve zoemer (5V)
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
const int BUZZER_PIN = 16;                            // Zoemer op GPIO 16
const int RECEIVER_LED_PIN = 22;                      // Status LED op GPIO 22

// ============================================
// OVERIGE VARIABELEN (NIET AANPASSEN)
// ============================================

#include <WiFi.h>
#include <WiFiUdp.h>

WiFiUDP udp;
char packetBuffer[255];                               // Buffer voor binnenkomende UDP-pakketten

// Zoemer timing variabelen (non-blocking)
unsigned long buzzerStartTime = 0;
bool isBuzzing = false;
const unsigned long BUZZER_DURATION = 1500;           // Zoemer duur in milliseconden

// WiFi status tracking
bool wifiWasConnected = false;

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
    pinMode(RECEIVER_LED_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);                    // Zoemer uit bij opstarten
    digitalWrite(RECEIVER_LED_PIN, LOW);              // LED uit bij opstarten
    
    Serial.println("Pinnen geconfigureerd:");
    Serial.print("  - Zoemer: GPIO ");
    Serial.println(BUZZER_PIN);
    Serial.print("  - Status LED: GPIO ");
    Serial.println(RECEIVER_LED_PIN);
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
        while (true);                                // Blokkeer bij fout
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
    wifiWasConnected = true;
    Serial.println();
    Serial.println();
    Serial.println("WiFi verbonden!");
    Serial.println("----------------------------------------");
    Serial.print("IP adres: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC adres: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Luisteren op poort: ");
    Serial.println(udpPort);
    Serial.print("Verwacht signaal van: ");
    Serial.println(ip_sender);
    Serial.println("----------------------------------------");
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
            udp.begin(udpPort);                       // UDP opnieuw starten na reconnect
        }
    }
    
    // Controleren of er UDP-pakketten zijn binnengekomen
    int packetSize = udp.parsePacket();
    
    if (packetSize) {
        // Pakket ontvangen
        int len = udp.read(packetBuffer, 255);
        
        // Null-terminate de string
        if (len > 0) {
            packetBuffer[len] = 0;
        }
        
        // Log ontvangst
        Serial.print("Ontvangen: ");
        Serial.print(packetBuffer);
        Serial.print(" van ");
        Serial.println(udp.remoteIP().toString());
        
        // Controleren of het het deurbel-signaal is
        if (strcmp(packetBuffer, doorbellPayload) == 0) {
            triggerBuzzer();
        }
    }
    
    // Non-blocking buzzer management
    updateBuzzer();
}

void triggerBuzzer() {
    // Voorkom dat de zoemer opnieuw start als hij al afgaat
    if (!isBuzzing) {
        Serial.println();
        Serial.println("========================================");
        Serial.println("         *** DING DONG! ***            ");
        Serial.println("========================================");
        Serial.println();
        
        // Zoemer inschakelen
        digitalWrite(BUZZER_PIN, HIGH);
        digitalWrite(RECEIVER_LED_PIN, LOW);          // LED uit tijdens zoemen
        
        buzzerStartTime = millis();
        isBuzzing = true;
    } else {
        Serial.println("Zoemer is al actief, signaal genegeerd");
    }
}

void updateBuzzer() {
    // Check of de zoemer-duur is verstreken
    if (isBuzzing && (millis() - buzzerStartTime > BUZZER_DURATION)) {
        digitalWrite(BUZZER_PIN, LOW);                // Zoemer uitschakelen
        digitalWrite(RECEIVER_LED_PIN, HIGH);         // LED weer aan
        isBuzzing = false;
        Serial.println("Zoemer cyclus voltooid");
    }
}

void handleDisconnection() {
    static unsigned long lastReconnectAttempt = 0;
    
    // LED knippert langzaam bij verbindingsproblemen
    if (millis() - lastReconnectAttempt > 1000) {
        lastReconnectAttempt = millis();
        digitalWrite(RECEIVER_LED_PIN, !digitalRead(RECEIVER_LED_PIN));
    }
    
    // Wacht even en probeer opnieuw
    delay(100);
    
    WiFi.disconnect();
    WiFi.reconnect();
}

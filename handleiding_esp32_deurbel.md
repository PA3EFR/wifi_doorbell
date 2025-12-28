# Handleiding ESP32 Remote Deurbel Systeem

## 1. Inleiding

Dit document beschrijft de volledige handleiding voor het bouwen van een remote deurbel systeem met behulp van twee ESP32 Lite microcontrollers. Het systeem maakt het mogelijk om vanaf de voordeur een signaal te verzenden dat op zolder een zoemer activeert, zonder dat er bedrading tussen deze locaties nodig is. De communicatie vindt plaats via het lokale WiFi-netwerk, wat betekent dat er geen externe servers, cloudservices of MQTT-brokers aan te pas komen.

Het systeem is ontworpen met nadruk op betrouwbaarheid en eenvoud. UDP wordt gebruikt als communicatieprotocol vanwege de lage latentie en de snelle "fire-and-forget" karakteristiek die bijzonder geschikt is voor toepassing zoals een deurbel waarbij een snelle reactie belangrijker is dan gegarandeerde levering. Om de betrouwbaarheid te verhogen worden signalen redundant verzonden, wat eventuele packet loss door tijdelijke WiFi-storingen compenseert.

De handleiding is opgedeeld in logische stappen, beginnend met de benodigde materialen en hardware-aansluitingen, gevolgd door de software-installatie en configuratie. Tot slot wordt een testprocedure beschreven om te verifiëren dat alles correct werkt, samen met tips voor het oplossen van veelvoorkomende problemen.

## 2. Systeem Architectuur

Het systeem bestaat uit twee onafhankelijke ESP32 Lite modules die via het lokale WiFi-netwerk met elkaar communiceren. De zendereenheid bevindt zich bij de voordeur en is uitgerust met een drukknop die, wanneer deze wordt ingedrukt, een UDP-pakket verstuurt naar de ontvangereenheid op zolder. De ontvanger luistert continu naar binnenkomende pakketten en activeert een zoemer zodra het deurbelsignaal wordt ontvangen.

Een belangrijk aspect van het ontwerp is het gebruik van statische IP-adressen. In plaats van te vertrouwen op DHCP-leasevernieuwingen, krijgen beide ESP32-bordjes een vast IP-adres toegewezen binnen de range 192.168.2.1-192.168.2.250. Dit elimineert de vertraging die ontstaat wanneer apparaten hun IP-adres moeten vernieuwen en zorgt ervoor dat de zender altijd precies weet naar welk adres het signaal moet worden verzonden.

De communicatie verloopt via UDP-poort 4210, een willekeurige poort die buiten de standaard gereserveerde ranges valt en daardoor weinig kans heeft op conflicten met andere netwerkservices. De payload van het signaal is de eenvoudige string "RING", wat de verwerking aan de ontvangerzijde minimaliseert en de kans op misinterpretatie verkleint.

### 2.1 IP-Adresindeling

De volgende IP-adressen zijn gereserveerd voor dit systeem en moeten binnen uw router of DHCP-server worden geconfigureerd om conflicten met andere apparaten te voorkomen:

| Apparaat | Functie | IP-adres | MAC-adres |
|----------|---------|----------|-----------|
| Zender | Drukknop detectie | 192.168.2.201 | Wordt automatisch toegewezen |
| Ontvanger | Zoemer aansturing | 192.168.2.202 | Wordt automatisch toegewezen |
| Router | Gateway | 192.168.2.254 | - |

Het gateway-adres moet worden aangepast aan uw specifieke routerconfiguratie. Veel routers gebruiken 192.168.2.1 of 192.168.2.254 als standaard gateway-adres. Raadpleeg de documentatie van uw router of de netwerkinstellingen van een reeds verbonden apparaat om het juiste gateway-adres te bepalen.

### 2.2 Communicatieprotocol

Het communicatieprotocol is bewust eenvoudig gehouden om de betrouwbaarheid te maximaliseren en de complexiteit te minimaliseren. Wanneer de drukknop wordt ingedrukt, verstuurt de zender driemaal snel achter elkaar het pakket "RING" naar de ontvanger. Elke verzending vindt plaats met een interval van 50 milliseconden, wat voldoende tijd geeft voor netwerkverwerking zonder merkbare vertraging voor de gebruiker.

De redundantie in de verzending is noodzakelijk omdat UDP een connectionless protocol is dat geen bevestiging van levering geeft. Hoewel WiFi-netwerken over het algemeen betrouwbaar zijn, kunnen tijdelijke storingen, interferentie of netwerkcongestie ervoor zorgen dat individuele pakketten verloren gaan. Door drie pakketten te versturen is de kans dat alle drie verloren gaan statistisch verwaarloosbaar, terwijl de ontvanger het signaal toch slechts eenmaal verwerkt.

Aan de ontvangerzijde wordt een non-blocking aansturing van de zoemer gebruikt. Dit betekent dat de microcontroller niet hoeft te wachten tot de zoemer klaar is met afgaan, maar direct kan door gaan met lu naar nieuwe signalen. Dit is essentieel omdat de ESP32 anders bezet zou zijn met wachten en mogelijk inkomende pakketten zou missen.

## 3. Benodigde Materialen

Voor de realisatie van dit remote deurbel systeem zijn de volgende componenten nodig. De totale kosten blijven relatief laag doordat standaard ESP32 Lite bordjes en eenvoudige componenten worden gebruikt die verkrijgbaar zijn bij reguliere elektronicawinkels of online platforms.

### 3.1 Hoofdcomponenten

De ESP32 Lite, ook wel Lolin32 Lite genoemd, is een compact ontwikkelbordje gebaseerd op de ESP32-WROOM-32 chip. Dit bordje beschikt over geïntegreerde WiFi en Bluetooth functionaliteit, wat het uitstekend geschikt maakt voor IoT-toepassing zoals deze deurbel. Het bordje heeft voldoende GPIO-pinnen voor de aansluiting van de drukknop, zoemer en LED's, en het lage stroomverbruik maakt continue werking mogelijk zonder oververhitting.

Voor de voeding wordt een standaard USB-voedingsadapter van 5V/1A aanbevolen, wat gebruikelijk is voor ESP32-ontwikkelborden. Het is belangrijk om de units permanent van stroom te voorzien via een USB-netzwachtel, in plaats van ze op een powerbank of losse batterijen aan te sluiten. Een deurbel moet altijd beschikbaar zijn en een batterij kan leeg raken op het moment dat u dat niet wilt.

| Component | Specificatie | Aantal | Geschatte prijs |
|-----------|--------------|--------|-----------------|
| ESP32 Lite bordje | Lolin32 of compatible | 2 | €8-15 per stuk |
| Actieve zoemer | 5V met geïntegreerde oscillator | 1 | €1-3 |
| Drukknop | Momentary push-button, NO | 1 | €0,50-2 |
| Weerstand | 220Ω (LED serieweerstand) | 2 | €0,10-0,50 |
| Weerstand | 10kΩ (optioneel) | 1 | €0,10-0,50 |
| USB-netzwachtel | 5V/1A | 2 | €5-10 |
| Montagemateriaal | Schroeven, kabelgoten | - | €5-15 |
| Verbindingsdraden | Dupont jumper wires | 1 set | €2-5 |

### 3.2 Optionele Componenten

Voor een nettere installatie kunnen optionele componenten worden overwogen. Een small PCB-bordje maakt het mogelijk om de componenten permanent te solderen in plaats van met losse draden te werken. Een behuizing van kunststof of 3D-geprint materiaal beschermt de elektronica tegen stof en vocht, hoewel dit niet strikt noodzakelijk is voor binnengebruik.

Indien de zendereenheid op een locatie wordt geplaatst waar geen stopcontact beschikbaar is, kan worden gekozen voor een batterijgevoede oplossing. Dit vereist echter aanpassingen in de code om het stroomverbruik te minimaliseren en de levensduur van de batterij te maximaliseren. Een dergelijke implementatie valt buiten de scope van deze handleiding.

## 4. Aansluitschema

De hardware-aansluitingen zijn bewust eenvoudig gehouden om de betrouwbaarheid te maximaliseren en de kans op aansluitfouten te minimaliseren. Beide ESP32 bordjes maken gebruik van de interne pull-up weerstand voor de drukknop, wat externe componenten bespaart en de bedrading vereenvoudigt.

### 4.1 Zendereenheid (Voordeur)

De drukknop wordt aangesloten op GPIO 13 met de andere aansluiting naar GND. De interne pull-up weerstand van de ESP32 wordt geactiveerd via de pinMode-instelling INPUT_PULLUP, wat betekent dat de pin standaard HIGH is en naar LOW gaat wanneer de knop wordt ingedrukt. Dit is de standaard configuratie voor drukknoppen op microcontrollers.

De status LED op GPIO 22 is aangesloten met een 220Ω serieweerstand naar de anode van de LED, terwijl de cathode naar GND gaat. De serieweerstand is noodzakelijk om de stroom door de LED te begrenzen en zowel de LED als de ESP32 te beschadigen. Let op dat sommige ESP32-bordjes een ingebouwde LED op GPIO 2 hebben; controleer de pinout van uw specifieke bordje en pas de code dienovereenkomstig aan.

```
Zendereenheid - Aansluitschema
==============================

ESP32 Lite Bordje
┌─────────────────────────────────────────────────────────┐
│                                                         │
│   3.3V ───────────────────────────────────────────────  │
│                                                         │
│   GPIO 13 ───────┬──────────────────────────────────    │
│                  │                                     │
│              ┌───┴───┐                                 │
│              │ Druk- │                                 │
│              │ knop  │                                 │
│              └───┬───┘                                 │
│                  │                                     │
│   GND ───────────┴──────────────────────────────────    │
│                                                         │
│   GPIO 22 ───────┬────[220Ω]────┬──[LED+]── 3.3V/5V     │
│                  │              │                         │
│   GND ───────────┴──────────────┴──[LED-]── GND          │
│                                                         │
│   5V ────────────────────────────────────────────────   │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 4.2 Ontvangereenheid (Zolder)

De actieve zoemer wordt aangesloten op GPIO 16, waarbij een HIGH-signaal de zoemer activeert en een LOW-signaal de zoemer uitschakelt. Actieve zoemers hebben een ingebouwde oscillator en vereisen alleen een gelijkspanningssignaal om geluid te produceren, wat de aansturing vereenvoudigt ten opzichte van passieve zoemers die een PWM-signaal nodig hebben.

De meeste actieve zoemers werken op 5V, wat de standaard USB-voedingsspanning is. Sommige kleinere zoemers werken ook op 3.3V, wat direct vanaf de ESP32 kan worden gevoed. Raadpleeg de specificaties van uw specifieke zoemer om de juiste voedingsspanning te bepalen.

```
Ontvangereenheid - Aansluitschema
=================================

ESP32 Lite Bordje
┌─────────────────────────────────────────────────────────┐
│                                                         │
│   5V ────────┬──────────────────────────────────────    │
│              │                                         │
│          ┌───┴───┐                                     │
│          │ Actief│                                     │
│          │ Zoemer│                                     │
│          └───┬───┘                                     │
│              │                                         │
│   GPIO 16 ───┴──────────────────────────────────────    │
│                                                         │
│   GPIO 22 ───────┬────[220Ω]────┬──[LED+]── 3.3V/5V     │
│                  │              │                         │
│   GND ───────────┴──────────────┴──[LED-]── GND          │
│                                                         │
│   5V ────────────────────────────────────────────────   │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 4.3 Voedingsschema

Beide units worden gevoed via een USB-kabel aangesloten op een 5V/1A netswachtel. De ESP32 Lite bordjes hebben een ingebouwde spanningsregelaar die de 5V USB-spanning omzet naar 3.3V voor de processor en de GPIO-pinnen. Dit betekent dat de zoemer direct vanaf de 5V USB-spanning kan worden gevoed, terwijl de GPIO-pin van de ESP32 alleen het aan/uit-signaal levert.

Het is belangrijk om een netswachtel te gebruiken die voldoende stroom kan leveren. Hoewel de ESP32 zelf weinig stroom verbruikt, kan een actieve zoemer piekstromen trekken die een goedkope of zwakke netswachtel kunnen overbelasten. Een netswachtel van minimaal 1A wordt daarom aanbevolen.

## 5. Software Installatie

De software voor het deurbel systeem bestaat uit twee afzonderlijke Arduino-sketches: één voor de zendereenheid en één voor de ontvangereenheid. Beide sketches bevatten alle benodigde code inclusief configuratie-instellingen die aan het begin van het bestand kunnen worden aangepast.

### 5.1 Voorbereiding

Voordat de software kan worden geüpload, moet de Arduino IDE worden geconfigureerd voor het werken met ESP32-bordjes. Volg de onderstaande stappen om de ontwikkelomgeving in te stellen:

Het toevoegen van ESP32-ondersteuning aan de Arduino IDE begint met het openen van Voorkeuren via het menu Arduino en het toevoegen van de ESP32 board manager URL. Vervolgens wordt via Boordbeheerder het ESP32-platform gedownload en geïnstalleerd. Na de installatie verschijnen de verschillende ESP32-bordjes in de lijst met beschikbare borden.

De exacte URL voor de ESP32 board manager is https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json. Deze URL moet worden toegevoegd aan het veld "Additional Boards Manager URLs" in de Voorkeuren. Na het herstarten van Arduino IDE kan via Boordbeheerder worden gezocht naar "ESP32" en het pakket worden geïnstalleerd.

### 5.2 Bordselectie en Poort

Na de installatie van het ESP32-platform selecteert u het juiste bord via het menu Hulpmiddelen > Bord. Voor de meeste Lolin32 Lite bordjes is "ESP32 Dev Module" de juiste keuze, hoewel sommige specifieke bordjes een eigen optie hebben. Raadpleeg de documentatie van uw bordje als u niet zeker bent van de juiste selectie.

De volgende instellingen worden aanbevolen voor optimale werking:

| Instelling | Waarde | Toelichting |
|------------|--------|-------------|
| Bord | ESP32 Dev Module | Of specifiek bordtype |
| Upload Speed | 115200 | Balans tussen snelheid en betrouwbaarheid |
| CPU Frequency | 240MHz | Maximale snelheid |
| Flash Mode | DOUT | Compatibiliteit met meeste bordjes |
| Flash Size | 4MB | Voldoende voor alle sketches |

De COM-poort selecteert u via Hulpmiddelen > Poort. Na het aansluiten van de ESP32 via USB verschijnt een nieuwe poort in de lijst, meestal aangeduid als COM3, COM4 of hoger onder Windows, of als /dev/ttyUSB0 onder Linux. Als er meerdere poorten worden getoond en u niet zeker weet welke de juiste is, kunt u de ESP32 loskoppelen en opnieuw aansluiten terwijl u let op welke poort verdwijnt en weer verschijnt.

### 5.3 Code Uploaden

Voor het uploaden van de code naar de zendereenheid opent u het bestand sender_esp32_doorbell.h in de Arduino IDE. Kopieer de volledige inhoud naar een nieuw sketch-bestand en sla dit op als sender_esp32_doorbell.ino. Pas vervolgens de WiFi-instellingen aan het begin van het bestand aan door UW_WIFI_SSID te vervangen door uw netwerknaam en UW_WIFI_WACHTWOORD te vervangen door uw wachtwoord.

De gateway-instelling moet overeenkomen met het IP-adres van uw router. Dit is vaak 192.168.2.1 of 192.168.2.254, maar kan afwijken afhankelijk van uw netwerkconfiguratie. Raadpleeg de routerdocumentatie of de netwerkinstellingen van een aangesloten apparaat als u niet zeker bent van het juiste adres.

Nadat alle instellingen zijn aangepast, upload u de sketch naar de ESP32 via Sketch > Upload. Tijdens het uploaden knippert de blauwe LED op de ESP32 snel. Als de upload is voltooid, opent u de Seriële Monitor via Hulpmiddelen > Seriële Monitor en stelt u de snelheid in op 115200 baud. U zou berichten moeten zien over het verbinden met WiFi en het verkrijgen van het IP-adres.

Herhaal dit proces voor de ontvangereenheid met het bestand receiver_esp32_doorbell.h. Let op dat beide sketches dezelfde WiFi-instellingen moeten gebruiken, maar dat ze elk naar hun eigen ESP32 worden geüpload.

## 6. Configuratie

De configuratie van het systeem bestaat uit het aanpassen van de netwerkinstellingen aan uw specifieke WiFi-netwerk en het eventueel aanpassen van timings en gevoeligheden aan uw voorkeuren.

### 6.1 WiFi-instellingen

In het begin van beide sketch-bestanden vindt u een sectie met configuratie-instellingen die moet worden aangepast aan uw netwerk. De meest kritische instellingen zijn de netwerknaam (ssid) en het wachtwoord (password), zonder welke geen verbinding met het WiFi-netwerk tot stand kan komen.

```cpp
// WiFi-instellingen
const char* ssid = "UW_WIFI_SSID";                    // Vul uw WiFi-netwerknaam in
const char* password = "UW_WIFI_WACHTWOORD";          // Vul uw WiFi-wachtwoord in
```

Let op dat het wachtwoord hoofdlettergevoelig is en exact moet overeenkomen met het wachtwoord dat u gebruikt om andere apparaten met het netwerk te verbinden. Veel netwerken gebruiken WPA2 of WPA3-beveiliging, wat door de ESP32 wordt ondersteund.

### 6.2 Netwerkadressering

De gateway-instelling moet overeenkomen met het IP-adres van uw router. Dit is het adres dat apparaten gebruiken om communicatie buiten het lokale netwerk te routeren. Bij Nederlandse internetproviders is dit vaak 192.168.2.1 of 192.168.2.254, maar dit kan variëren.

```cpp
// Netwerkinstellingen (statische IP)
IPAddress gateway(192, 168, 2, 254);                  // IP van uw router
IPAddress subnet(255, 255, 255, 0);                   // Subnet mask
IPAddress dns(8, 8, 8, 8);                            // Google DNS (fallback)
```

Het subnet mask 255.255.255.0 is geschikt voor de meeste thuisnetwerken met maximaal 254 adressen in het lokale netwerk. De DNS-instelling van 8.8.8.8 is de publieke DNS-server van Google en wordt gebruikt als fallback wanneer de router geen DNS-diensten levert.

### 6.3 IP-adres reservering

Hoewel de sketches statische IP-adressen configureren, wordt het aanbevolen om deze adressen ook te reserveren in de DHCP-server van uw router. Dit voorkomt conflicten als de DHCP-server per ongeluk hetzelfde adres zou toewijzen aan een ander apparaat.

Om een DHCP-reservering toe te voegen, logt u in op de webinterface van uw router en navigeert u naar de DHCP-instellingen. Zoek naar een optie zoals "DHCP Reservation", "Static DHCP" of "Address Reservation". Voeg de MAC-adressen van beide ESP32-bordjes toe en wijs de gewenste IP-adressen toe. De MAC-adressen worden weergegeven in de seriële monitor bij het opstarten van de ESP32.

### 6.4 Optionele aanpassingen

Indien gewenst kunnen de volgende parameters worden aangepast aan uw voorkeuren:

| Parameter | Standaard waarde | Bereik | Beschrijving |
|-----------|------------------|--------|--------------|
| DEBOUNCE_DELAY | 50 | 10-200 ms | Tijd om ruis te filteren |
| ANTI_SPAM_DELAY | 2000 | 500-5000 ms | Min tijd tussen signalen |
| BUZZER_DURATION | 1500 | 500-3000 ms | Hoe lang de zoemer klinkt |
| udpPort | 4210 | 1024-65535 | Communicatiepoort |

Het verlagen van DEBOUNCE_DELAY maakt de drukknop gevoeliger, maar kan ook leiden tot onbedoelde triggers door elektrische ruis. Het verhogen van ANTI_SPAM_DELAY voorkomt herhaalde signalen als de knop wordt vastgehouden, maar kan hinderlijk zijn als u snel meerdere keren wilt bellen.

## 7. Testprocedure

Na het aansluiten van alle componenten en het uploaden van de juiste code naar beide units, is het belangrijk om systematisch te verifiëren dat alles correct werkt. De onderstaande testprocedure doorloopt alle kritische functies en identificeert eventuele problemen voordat het systeem in gebruik wordt genomen.

### 7.1 Test ontvanger

Begin met het aansluiten van de ontvangereenheid op zolder en open de seriële monitor op 115200 baud. Wacht tot u de volgende berichten ziet verschijnen:

```
========================================
   ESP32 Deurbel Ontvanger - Zolder     
========================================
Opstarten...
Pinnen geconfigureerd:
  - Zoemer: GPIO 16
  - Status LED: GPIO 22

Verbinden met WiFi...
.........
WiFi verbonden!
----------------------------------------
IP adres: 192.168.2.202
MAC adres: xx:xx:xx:xx:xx:xx
Luisteren op poort: 4210
Verwacht signaal van: 192.168.2.201
----------------------------------------
Systeem is klaar voor gebruik!
```

Het IP-adres moet 192.168.2.202 zijn. Als de ontvanger een ander adres krijgt of niet kan verbinden, controleer dan de WiFi-instellingen en de gateway-configuratie. De status LED moet continu branden, wat aangeeft dat de WiFi-verbinding actief is en de ontvanger klaar is om signalen te ontvangen.

### 7.2 Test zender

Sluit vervolgens de zendereenheid aan bij de voordeur en open eveneens de seriële monitor. Na het opstarten zou u vergelijkbare berichten moeten zien, maar met IP-adres 192.168.2.201:

```
========================================
   ESP32 Deurbel Zender - Voordeur       
========================================
Opstarten...
Pinnen geconfigureerd:
  - Drukknop: GPIO 13
  - Status LED: GPIO 22

Verbinden met WiFi...
WiFi verbonden!
----------------------------------------
IP adres: 192.168.2.201
MAC adres: xx:xx:xx:xx:xx:xx
Zend naar: 192.168.2.202:4210
----------------------------------------
Systeem is klaar voor gebruik!
```

Controleer dat beide units verbonden zijn met hetzelfde WiFi-netwerk en dat ze elkaar kunnen "zien" op het netwerk. U kunt dit verifiëren door vanaf een computer een ping-opdracht uit te voeren naar beide IP-adressen.

### 7.3 Integratietest

Nu beide units operationeel zijn, drukt u op de drukknop bij de voordeur. In de seriële monitor van de zender zou u de volgende berichten moeten zien:

```
>>> Deurbel ingedrukt! Signaal wordt verzonden...
  Pakket 1 verzonden
  Pakket 2 verzonden
  Pakket 3 verzonden
>>> Alle signalen verzonden naar ontvanger
  Doel: 192.168.2.202:4210
```

Gelijkertijd zou in de seriële monitor van de ontvanger het volgende moeten verschijnen:

```
Ontvangen: RING van 192.168.2.201

========================================
         *** DING DONG! ***
========================================

Zoemer cyclus voltooid
```

De zoemer op de ontvanger moet gedurende ongeveer 1,5 seconden geluid maken en de LED moet kort uitgaan tijdens het afgaan van de zoemer. Als de zoemer niet afgaat, controleer dan de aansluitingen en de polariteit van de zoemer.

### 7.4 Bereiktest

Plaats beide units op hun definitieve locaties en voer opnieuw de integratietest uit. UDP-signalen gaan door muren, maar de WiFi-signaalsterkte kan een beperkende factor zijn. Als het signaal niet aankomt, probeer dan een WiFi-extender of overweeg het verplaatsen van de router naar een centrale locatie.

U kunt de signaalsterkte controleren door aan de zenderzijde de volgende regel toe te voegen aan de loop-functie:

```cpp
Serial.print("RSSI: ");
Serial.println(WiFi.RSSI());
```

Een RSSI-waarde boven -70 dBm wordt over het algemeen als voldoende beschouwd voor betrouwbare communicatie. Waarden beneden -80 dBm kunnen leiden tot onbetrouwbare prestaties.

## 8. Probleemoplossing

Ondanks de eenvoud van het ontwerp kunnen zich problemen voordoen tijdens de installatie of het gebruik. De onderstaande sectie beschrijft veelvoorkomende problemen en hun oplossingen.

### 8.1 WiFi-verbindingsproblemen

Het meest voorkomende probleem is het niet tot stand komen van een WiFi-verbinding. Controleer eerst dat de netwerknaam en het wachtwoord correct zijn ingevoerd en dat er geen typefouten in staan. WiFi-wachtwoorden zijn hoofdlettergevoelig, dus "MijnNetwerk" is anders dan "mijnnetwerk".

Controleer ook dat het ESP32-bordje binnen het bereik van de WiFi-router is en dat er voldoende signaalsterkte is. Dikke betonnen muren, metalen structuren en andere elektronische apparaten kunnen het WiFi-signaal verstoren. Als de signaalsterkte zwak is, overweeg dan het gebruik van een WiFi-extender of het verplaatsen van de router.

Sommige routers hebben "AP-isolatie" of "client-isolatie" ingeschakeld, wat voorkomt dat apparaten op hetzelfde netwerk direct met elkaar communiceren. Deze functie moet worden uitgeschakeld in de routerinstellingen om het deurbel systeem te laten werken. Raadpleeg de routerdocumentatie voor instructies over het vinden en uitschakelen van deze instelling.

### 8.2 Geen signaalontvangst

Als de WiFi-verbinding wel tot stand komt maar de ontvanger niet reageert op drukknopindrukkingen, controleer dan de volgende zaken. Zorg ervoor dat beide units met hetzelfde WiFi-netwerk zijn verbonden en niet met verschillende netwerken of gastnetwerken. Gastnetwerken zijn vaak geïsoleerd van het hoofdnetwerk en staan directe communicatie niet toe.

Verifieer dat de UDP-poort niet wordt geblokkeerd door een firewall op de router. Hoewel poort 4213 een niet-standaard poort is, blokkeren sommige routers onbekende inkomende verbindingen. Controleer de firewall-instellingen van uw router en zorg ervoor dat UDP-verkeer op poort 4210 is toegestaan.

Controleer ook dat de IP-adressen in de sketches correct zijn geconfigureerd. De zender moet het IP-adres van de ontvanger kennen om het signaal te verzenden. Als deze adressen verkeerd zijn geconfigureerd, wordt het pakket naar het verkeerde apparaat of een niet-bestaand adres verzonden.

### 8.3 Valse triggers

Als de deurbel afgaat zonder dat de knop wordt ingedrukt, is er waarschijnlijk sprake van ruis op de drukknopingang. Hoewel de interne pull-up weerstand normaal gesproken stabiel is, kunnen externe factoren zoals lange kabeltrajecten of nabije elektrische apparaten storing veroorzaken.

Verhoog de DEBOUNCE_DELAY in de sketch van 50 naar 100 of zelfs 200 milliseconden om meer ruis te filteren. U kunt ook een externe pull-down weerstand van 10kΩ toevoegen parallel aan de drukknop voor extra stabiliteit. Een condensator van 100nF over de drukknop kan ook helpen om高频 ruis te filteren.

### 8.4 Zoemer werkt niet

Als de zoemer niet afgaat terwijl de seriële monitor wel signaalontvangst aangeeft, controleer dan de aansluitingen. Actieve zoemers hebben een polariteit en werken alleen wanneer ze correct zijn aangesloten. Verwissel de + en - aansluitingen als de zoemer niet werkt.

Controleer ook dat de zoemer werkt door hem direct op 5V en GND aan te sluiten, buiten de ESP32 om. Als de zoemer dan wel geluid maakt, ligt het probleem in de GPIO-aansturing of de software. Verifieer dat de BUZZER_PIN correct is gedefinieerd in de sketch en overeenkomt met de fysieke aansluiting.

## 9. Onderhoud en Uitbreidingen

Het deurbel systeem is ontworpen voor betrouwbaarheid met minimaal onderhoud. Door de robuuste software met automatische herverbinding en foutafhandeling zou het systeem maandenlang probleemloos moeten werken zonder tussenkomst.

### 9.1 Periodiek onderhoud

Hoewel het systeem automatisch herstelt van de meeste problemen, is het raadzaam om periodiek de werking te verifiëren. Dit kan door eenvoudigweg op de deurbel te drukken en te controleren of het geluid op zolder hoorbaar is. Een maandelijkse test is voldoende om te verifiëren dat alle componenten nog correct functioneren.

Indien de units op een stoffige of vochtige locatie zijn geplaatst, kan periodieke reiniging nodig zijn. Gebruik een droge doek om stof te verwijderen en vermijd het gebruik van vloeibare reinigingsmiddelen in de buurt van de elektronica.

### 9.2 Automatische herstart

Voor langdurig betrouwbaar gebruik kan een periodieke automatische herstart worden geïmplementeerd. Microcontrollers kunnen na langdurig continu gebruik last krijgen van geheugenlekkages of vastgelopen netwerkstack. Een wekelijkse herstart voorkomt dit probleem en zorgt ervoor dat het systeem fris start.

De volgende code kan worden toegevoegd aan beide sketches om een wekelijkse herstart te implementeren:

```cpp
// Variabele voor herstart-timer
unsigned long lastRestartTime = 0;
const unsigned long RESTART_INTERVAL = 604800000; // 7 dagen in milliseconden

// Voeg dit toe aan de loop-functie:
if (millis() - lastRestartTime > RESTART_INTERVAL) {
    Serial.println("Geplande herstart...");
    ESP.restart();
}
```

### 9.3 Optionele uitbreidingen

Voor gebruikers die het systeem verder willen aanpassen, zijn diverse uitbreidingen mogelijk. Een meer complexe toevoeging zou het implementeren van verschillende zoemerpatronen zijn om onderscheid te maken tussen verschillende deuren of bezoekers. De voordeur kan bijvoorbeeld een tweeledig "ding-dong" patroon hebben, terwijl de achterdeur een enkelvoudig signaal geeft.

Een andere uitbreiding is het toevoegen van batterijbewaking voor de zendereenheid, mocht deze op een locatie worden geplaatst waar geen stopcontact beschikbaar is. Een spanningsdeler aangesloten op een analoge pin kan de batterijspanning monitoren en een waarschuwing versturen wanneer de batterij bijna leeg is.

Voor integratie met smart home systemen kan de ontvanger worden uitgebreid met HTTP-requests naar een home automation server, mits deze in hetzelfde netwerk opereert. Dit valt echter buiten de scope van deze handleiding en vereist aanvullende kennis van het specifieke home automation systeem.

## 10. Technische Specificaties

Deze sectie geeft een overzicht van de volledige technische specificaties van het systeem voor referentie en toekomstig onderhoud.

### 10.1 Netwerkspecificaties

| Parameter | Waarde |
|-----------|--------|
| Protocol | UDP |
| Poort | 4210 |
| Payload | "RING" |
| Redundantie | 3 pakketten per druk |
| Inter-pakket interval | 50 ms |

### 10.2 Stroomverbruik

| Toestand | Stroomverbruik |
|----------|----------------|
| In rust (WiFi verbonden) | 80-100 mA |
| Tijdens signaalverzending | 120-150 mA |
| Tijdens zoemen | 150-200 mA |
| In diepe slaag (niet geïmplementeerd) | - |

### 10.3 Timings

| Functie | Waarde |
|---------|--------|
| Debounce vertraging | 50 ms |
| Anti-spam interval | 2000 ms |
| Zoemer duur | 1500 ms |
| WiFi reconnect interval | 5000 ms |

Dit document is opgesteld om u te begeleiden bij het bouwen, installeren en onderhouden van uw ESP32 remote deurbel systeem. Bij vragen of problemen die niet in deze handleiding worden behandeld, raadpleeg dan de Arduino- en ESP32-community forums voor aanvullende ondersteuning.

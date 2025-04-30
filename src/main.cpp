#include "WiFi.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include "HW_SendRsV3.h"
#include <ArduinoJson.h> // Utilisé pour traiter les données JSON

// Inclure les informations sensibles depuis un fichier séparé
#include "secrets.h"


// Définir les variables de connexion WiFi
// Les valeurs sont définies dans secrets.h:
// #define WIFI_SSID "votre_ssid" 
// #define WIFI_PASSWORD "votre_mot_de_passe"
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// LED sur GPIO2
const int ledPin = 2;

const int NUM_LEDS = 4;
const int NUM_LEDS_MATRIXCONF = 8;
const int LED_PINS[NUM_LEDS] = {15, 4, 22, 23};  // D2, D4, D22, D23
const int LED_PINS_MATRIXCONF[NUM_LEDS_MATRIXCONF] = {13, 12, 14, 27, 26, 25, 33, 32};  // D2, D4, D22, D23

bool ledStates[NUM_LEDS] = {false, false, false, false};
bool ledStatesMatrixConf[NUM_LEDS_MATRIXCONF] = {false, false, false, false, false, false, false, false};


unsigned long previousMillis = 0;
const long interval = 100;  // Intervalle de 100ms entre chaque LED
int currentLed = 0;



// Structure pour stocker les configurations
struct EffectConfig {
    const char* order;
    uint8_t muteConfig;
};

// Table de vérité pour toutes les configurations possibles
const EffectConfig MUTE_CONFIGS[] = {
    // Un seul effet
    {"1", 0b00100111},    // SPX seul
    {"2", 0b01101100},    // DIGI seul
    {"3", 0b10110100},    // BBE seul
    
    // Deux effets
    {"12", 0b01100011},   // SPX -> DIGI
    {"13", 0b10000111},   // SPX -> BBE
    {"21", 0b00101101},   // DIGI -> SPX 00101101
    {"23", 0b10011100},   // DIGI -> BBE 10011100
    {"31", 0b00110110},   // BBE -> SPX  00110110
    {"32", 0b01111000},   // BBE -> DIGI 01111000
    
    // Trois effets
    {"123", 0b10010011},  // SPX -> DIGI -> BBE 10010011
    {"132", 0b01001011},  // SPX -> BBE -> DIGI 01001011
    {"213", 0b10001101},  // DIGI -> SPX -> BBE 10001101
    {"231", 0b00011110},  // DIGI -> BBE -> SPX 00011110
    {"312", 0b01110010},  // BBE -> SPX -> DIGI 01110010
    {"321", 0b00111001}   // BBE -> DIGI -> SPX 00111001
};


// Fonction pour faire clignoter la LED
void blinkLED() {
    for (int i = 0; i < 3; i++) {  // Clignote 3 fois
        digitalWrite(ledPin, HIGH);   // Allumer la LED
        delay(100);                   // Attendre 200 ms
        digitalWrite(ledPin, LOW);    // Éteindre la LED
        delay(100);                   // Attendre 200 ms
    }
}



// Fonction pour obtenir la configuration en fonction de l'ordre des effets
uint8_t getMuteConfig(const JsonArray& order) {
    if (order.size() == 0) return 0b11100100;
    
    String orderStr = "";
    for(JsonVariant v : order) {
        orderStr += v.as<String>();
    }
    
    for(const EffectConfig& config : MUTE_CONFIGS) {
        if (orderStr == config.order) {
            return config.muteConfig;
        }
    }
    
    return 0b11100100;
}

uint8_t updateEffects(const JsonArray& order) {
    // Réinitialiser les LEDs
    for(int i = 0; i < NUM_LEDS; i++) {
        ledStates[i] = false;
        digitalWrite(LED_PINS[i], LOW);
    }
    
    // Debug: Afficher l'ordre reçu
    Serial.println("Ordre des effets reçu:");
    for(JsonVariant v : order) {
        Serial.print("Effet: ");
        Serial.println(v.as<String>());
    }
    
    // Allumer les LEDs des effets utilisés
    for(JsonVariant v : order) {
        int effectNum = v.as<String>().toInt();
        if(effectNum > 0 && effectNum <= NUM_LEDS) {
            Serial.print("Activation LED pour effet ");
            Serial.println(effectNum);
            ledStates[effectNum - 1] = true;
            digitalWrite(LED_PINS[effectNum - 1], HIGH);
        }
    }
    
    // Debug: Afficher l'état final des LEDs
    Serial.println("État final des LEDs:");
    for(int i = 0; i < NUM_LEDS; i++) {
        Serial.print("LED ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(ledStates[i] ? "ON" : "OFF");
    }
    
    // Obtenir la configuration de routage
    uint8_t muteConfig = getMuteConfig(order);
    
    return muteConfig;
}


AsyncWebServer server(80);



// Structure pour stocker l'état complet
struct StateConfig {
    uint8_t muteConfigByte;
    bool ledStates[NUM_LEDS];
    bool ledStatesMatrixConf[NUM_LEDS_MATRIXCONF];
};

// Fonction pour sauvegarder la configuration complète
void saveState(uint8_t muteConfigByte, bool ledStates[], bool ledStatesMatrixConf[]) {
    StateConfig state;
    state.muteConfigByte = muteConfigByte;
    memcpy(state.ledStates, ledStates, sizeof(bool) * NUM_LEDS);
    memcpy(state.ledStatesMatrixConf, ledStatesMatrixConf, sizeof(bool) * NUM_LEDS_MATRIXCONF);

    // Debug: Afficher les données avant la sauvegarde
    Serial.println("\n--- Sauvegarde de l'état ---");
    Serial.print("muteConfigByte: 0x");
    Serial.println(muteConfigByte, HEX);
    
    Serial.println("États des LEDs principales:");
    for(int i = 0; i < NUM_LEDS; i++) {
        Serial.print("LED ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(state.ledStates[i] ? "ON" : "OFF");
    }
    
    Serial.println("États des LEDs Matrix:");
    for(int i = 0; i < NUM_LEDS_MATRIXCONF; i++) {
        Serial.print("LED Matrix ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(state.ledStatesMatrixConf[i] ? "ON" : "OFF");
    }

    File file = SPIFFS.open("/state_config.bin", "w");
    if(!file) {
        Serial.println("Erreur lors de l'ouverture du fichier en écriture");
        return;
    }

    size_t bytesWritten = file.write((uint8_t*)&state, sizeof(StateConfig));
    file.close();

    if(bytesWritten == sizeof(StateConfig)) {
        Serial.print("Configuration complète sauvegardée (");
        Serial.print(bytesWritten);
        Serial.println(" octets)");
    } else {
        Serial.println("Erreur lors de la sauvegarde: nombre d'octets incorrect");
    }
    Serial.println("---------------------------\n");
}

// Fonction pour charger la configuration complète
StateConfig loadState() {
    StateConfig state;
    // Valeurs par défaut
    state.muteConfigByte = 0b11100100;
    memset(state.ledStates, 0, sizeof(bool) * NUM_LEDS);
    memset(state.ledStatesMatrixConf, 0, sizeof(bool) * NUM_LEDS_MATRIXCONF);

    File file = SPIFFS.open("/state_config.bin", "r");
    if(!file) {
        Serial.println("Aucune configuration trouvée, utilisation des valeurs par défaut");
        return state;
    }

    size_t bytesRead = file.read((uint8_t*)&state, sizeof(StateConfig));
    file.close();

    if(bytesRead == sizeof(StateConfig)) {
        Serial.println("\n--- Chargement de l'état ---");
        Serial.print("Configuration chargée (");
        Serial.print(bytesRead);
        Serial.println(" octets)");
        
        Serial.print("muteConfigByte: 0x");
        Serial.println(state.muteConfigByte, HEX);
        
        Serial.println("États des LEDs principales:");
        for(int i = 0; i < NUM_LEDS; i++) {
            Serial.print("LED ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.println(state.ledStates[i] ? "ON" : "OFF");
        }
        
        Serial.println("États des LEDs Matrix:");
        for(int i = 0; i < NUM_LEDS_MATRIXCONF; i++) {
            Serial.print("LED Matrix ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(state.ledStatesMatrixConf[i] ? "ON" : "OFF");
        }
        Serial.println("---------------------------\n");
    } else {
        Serial.println("Erreur lors du chargement: nombre d'octets incorrect");
        // Utiliser les valeurs par défaut en cas d'erreur
        state.muteConfigByte = 0b11100100;
        memset(state.ledStates, 0, sizeof(bool) * NUM_LEDS);
        memset(state.ledStatesMatrixConf, 0, sizeof(bool) * NUM_LEDS_MATRIXCONF);
    }

    return state;
}

// Fonction pour sauvegarder la configuration mute
void saveMuteConfig(uint8_t muteConfigByte) {
    File file = SPIFFS.open("/mute_config.bin", "w");
    if(!file) {
        Serial.println("Erreur lors de l'ouverture du fichier en écriture");
        return;
    }
    file.write(&muteConfigByte, 1);
    file.close();
    Serial.println("Configuration mute sauvegardée");
}

// Fonction pour charger la configuration mute
uint8_t loadMuteConfig() {
    uint8_t muteConfigByte = 0b11100100; // Valeur par défaut
    File file = SPIFFS.open("/mute_config.bin", "r");
    if(!file) {
        Serial.println("Aucune configuration mute trouvée, utilisation des valeurs par défaut");
        return muteConfigByte;
    }
    file.read(&muteConfigByte, 1);
    file.close();
    Serial.println("Configuration mute chargée");
    return muteConfigByte;
}



void setup() {
    // Configurer GPIO2 comme sortie pour la LED
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);  // Éteindre la LED au départ

    // Configuration des pins LED
    for(int i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW);
    }

    // Configuration des pins LED en sortie
    for(int i = 0; i < NUM_LEDS_MATRIXCONF; i++) {
        pinMode(LED_PINS_MATRIXCONF[i], OUTPUT);
    }

    Serial.begin(115200);
    
    setupSerialConfig();

    // Initialisation de SPIFFS
    if (!SPIFFS.begin()) {
        Serial.println("Une erreur s'est produite lors du montage de SPIFFS");
        return;
    }

    // Charger la configuration complète
    StateConfig state = loadState();
    
    // Restaurer les états des LEDs
    memcpy(ledStates, state.ledStates, sizeof(bool) * NUM_LEDS);
    memcpy(ledStatesMatrixConf, state.ledStatesMatrixConf, sizeof(bool) * NUM_LEDS_MATRIXCONF);
    
    // Appliquer les états des LEDs
    for(int i = 0; i < NUM_LEDS; i++) {
        digitalWrite(LED_PINS[i], ledStates[i] ? HIGH : LOW);
    }
    
    for(int i = 0; i < NUM_LEDS_MATRIXCONF; i++) {
        digitalWrite(LED_PINS_MATRIXCONF[i], ledStatesMatrixConf[i] ? HIGH : LOW);
    }

    // Appliquer la configuration mute
    SendRsBuffer[0] = 1;  // Numéro de preset
    SendRsBuffer[1] = state.muteConfigByte;
    HW_SendRsV3(NewPreset, 0, 2);

    // Connexion au WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    Serial.println("Connected, IP address: ");
    Serial.println(WiFi.localIP());

    // Routes pour servir les fichiers HTML, JS, CSS et favicon
    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/index.html", "text/html");
    });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/script.js", "text/javascript");
    });

    server.on("/Sortable.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/Sortable.min.js", "text/javascript");
    });

    server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/styles.css", "text/css");
    });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/favicon.ico", "image/x-icon");
    });

    server.on("/bbe.jpg", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/bbe.jpg", "image/x-icon");
    });

    server.on("/rack2.jpg", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/rack2.jpg", "image/x-icon");
    });

    server.on("/digitech_studioquad_1.jpg", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/digitech_studioquad_1.jpg", "image/x-icon");
    });

    server.on("/yamaha_spx90_mkii.jpg", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/yamaha_spx90_mkii.jpg", "image/x-icon");
    });

    server.on("/Korg-A3.jpg", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/Korg-A3.jpg", "image/x-icon");
    });

    server.on("/aphex-204.jpg", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(SPIFFS, "/aphex-204.jpg", "image/x-icon");
    });


    // Route pour recevoir l'ordre de la liste
    server.on("/update-order", HTTP_POST, [](AsyncWebServerRequest * request) {}, NULL,
    [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
        String body = "";
        for (size_t i = 0; i < len; i++) {
            body += (char)data[i];
        }

        Serial.println("Received body:");
        Serial.println(body);

        JsonDocument doc; // OK pour v7.x
        DeserializationError error = deserializeJson(doc, body);

        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            request->send(400, "application/json", "{\"status\":\"error\", \"message\": \"Invalid JSON format\"}");
            return;
        }

        // Récupérer l'ordre des éléments de la liste 2
        JsonArray order = doc["order"];
        
        // Réinitialiser l'état des LEDs et marquer tous les effets comme non utilisés
        bool effectUsed[NUM_LEDS] = {false, false, false};
        for(int i = 0; i < NUM_LEDS; i++) {
            ledStates[i] = false;
            digitalWrite(LED_PINS[i], LOW);
        }

        // Obtenir la configuration depuis updateEffects
        uint8_t muteConfigByte = updateEffects(order);

        // Debug: Afficher l'état des LEDs avant la mise à jour Matrix
        Serial.println("État des LEDs principales:");
        for(int i = 0; i < NUM_LEDS; i++) {
            Serial.print("LED ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.println(ledStates[i] ? "ON" : "OFF");
        }

        // Mise à jour des LEDs de configuration Matrix
        Serial.println("Mise à jour des LEDs Matrix:");
        for(int i = 0; i < NUM_LEDS_MATRIXCONF; i++) {
            ledStatesMatrixConf[i] = (muteConfigByte & (1 << i)) != 0;
            digitalWrite(LED_PINS_MATRIXCONF[i], ledStatesMatrixConf[i] ? HIGH : LOW);
            Serial.print("LED Matrix ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(ledStatesMatrixConf[i] ? "ON" : "OFF");
        }

        // Debug: Afficher les données avant la sauvegarde
        Serial.println("Données à sauvegarder:");
        Serial.print("muteConfigByte: 0x");
        Serial.println(muteConfigByte, HEX);

        // Sauvegarder la configuration complète
        saveState(muteConfigByte, ledStates, ledStatesMatrixConf);

        // Envoi de la commande NewPreset
        SendRsBuffer[0] = 1;  // Numéro de preset
        SendRsBuffer[1] = muteConfigByte;
        HW_SendRsV3(NewPreset, 0, 2);

        // Afficher la trame complète avant l'envoi
        Serial.println("Trame HW_SendRs complète (hex):");
        Serial.print("Command: 0x");
        Serial.println(NewPreset, HEX);
        Serial.print("Data: ");
        for(int i = 0; i < 10; i++) {
            if(SendRsData[i] < 0x10) Serial.print("0"); // Ajoute un 0 pour les valeurs < 16
            Serial.print(SendRsData[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        Serial.println("--------------------------");
         Serial.println("LastCheckSum: 0x");
         Serial.println(LastCheckSum, HEX);

        // Clignotement de la LED pour indiquer la réception
        blinkLED();

        // Répondre à la requête
        request->send(200, "application/json", "{\"status\":\"success\"}");
        
    });


    // Route pour recevoir la commande AboutVersion
    server.on("/send-about-version", HTTP_POST, [](AsyncWebServerRequest * request) {

        // Afficher le message "Matrix GC v0.0" dans le terminal série
        Serial.println("Matrix GC v0.0");

        // Envoyer la commande de changement de preset


        // Utiliser memcpy pour copier la chaîne "bienvenue" dans SendRsData
        const char* message = "Matrix GC v0.0";
        memcpy(SendRsBuffer, message, strlen(message));
        HW_SendRsV3(AboutVersion , 0xFF, strlen(message)); // Ajustez les paramètres si nécessaire
        //   mySerial.println("Matrix GC v0.0");

        // Réponse au client
        request->send(200, "application/json", "{\"status\":\"success\", \"message\":\"Matrix GC v0.0\"}");
    });

    // Route pour changer le preset
    server.on("/change-preset", HTTP_POST, [](AsyncWebServerRequest * request) {
        if (request->hasParam("preset", true)) {
            String presetValue = request->getParam("preset", true)->value();



            SendRsBuffer[0] =  presetValue.toInt();
            // Envoyer la commande de changement de preset
            HW_SendRsV3(PresetChange , 0, 1); // Ajustez les paramètres si nécessaire

            // Affiche la commande dans le terminal série
            Serial.printf("Changement de preset : %s\n", presetValue.c_str());

            // Réponse au client
            request->send(200, "application/json", "{\"status\":\"success\", \"message\":\"Preset changed\"}");
        } else {
            request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Preset not provided\"}");
        }
    });

    // Route pour la commande de reset
    server.on("/reset", HTTP_POST, [](AsyncWebServerRequest * request) {
        Serial.println("Reset command received");

        // Réinitialiser les états des LEDs
        for(int i = 0; i < NUM_LEDS; i++) {
            ledStates[i] = false;
            digitalWrite(LED_PINS[i], LOW);
        }
        
        // Réinitialiser les LEDs Matrix avec la configuration par défaut
        uint8_t defaultMuteConfig = 0b11100100;
        for(int i = 0; i < NUM_LEDS_MATRIXCONF; i++) {
            ledStatesMatrixConf[i] = (defaultMuteConfig & (1 << i)) != 0;
            digitalWrite(LED_PINS_MATRIXCONF[i], ledStatesMatrixConf[i] ? HIGH : LOW);
        }

        // Sauvegarder l'état réinitialisé
        saveState(defaultMuteConfig, ledStates, ledStatesMatrixConf);

        // Prépare le buffer pour la commande de reset
        SendRsBuffer[0] = 0;
        HW_SendRsV3(Reset, 0, 0);

        // Répondre à la requête pour indiquer que la commande a été envoyée
        request->send(200, "application/json", "{\"status\":\"success\", \"message\": \"Reset command sent\"}");

        // Attendre 1 seconde avant le reset
        delay(1000);

        // Reset de l'ESP
        ESP.restart();
    });

    // Démarrer le serveur
    server.begin();

    // Clignotement de la LED pour indiquer la réception
    blinkLED();

}


void loop() {
    // La boucle reste vide car les LEDs sont maintenant contrôlées 
    // par les événements de drag and drop
}


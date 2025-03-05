#include "WiFi.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include "HW_SendRsV3.h"
#include <ArduinoJson.h> // Utilisé pour traiter les données JSON

const char* ssid = "Livebox-A0D0";
const char* password = "eaVsfDpztiRnnChzCx";

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

AsyncWebServer server(80);

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

void setup() {

  // Configurer GPIO2 comme sortie pour la LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);  // Éteindre la LED au départ

  Serial.begin(115200);
  // mySerial.begin(57600, SERIAL_8N1, 16, 17); // TX1 sur GPIO 16, RX1 sur GPIO 17
  // Appeler setupSerialConfig pour configurer la communication série et enregistrer la configuration

  setupSerialConfig();

  // Initialisation de SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }

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

    DynamicJsonDocument doc(1024);
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

    // Debug: Afficher les valeurs
    Serial.println("-------- Debug Values --------");
    Serial.print("muteConfigByte (decimal): ");
    Serial.println(muteConfigByte);
    Serial.print("muteConfigByte (hex): 0x");
    Serial.println(muteConfigByte, HEX);
    Serial.println("--------------------------");

    // Envoi de la commande NewPreset
    SendRsBuffer[0] = 1;  // Numéro de preset
    SendRsBuffer[1] = muteConfigByte;
    HW_SendRsV3(NewPreset, 0, 2);

    
    // Mise à jour des LEDs de configuration Matrix
    for(int i = 0; i < NUM_LEDS_MATRIXCONF; i++) {
 
        if(muteConfigByte & (1 << i)) {
            digitalWrite(LED_PINS_MATRIXCONF[i], HIGH);
        } else {
            digitalWrite(LED_PINS_MATRIXCONF[i], LOW);
        }
    }


    

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

    // Prépare le buffer pour la commande de reset
    SendRsBuffer[0] = 0;  // Optionnel, selon les spécifications du reset
    HW_SendRsV3(Reset, 0, 0);

    // Répondre à la requête pour indiquer que la commande a été envoyée
    request->send(200, "application/json", "{\"status\":\"success\", \"message\": \"Reset command sent\"}");
  });

  // Configuration des pins LED
  for(int i = 0; i < NUM_LEDS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  // Configuration des pins LED en sortie
  for(int i = 0; i < NUM_LEDS_MATRIXCONF; i++) {
    pinMode(LED_PINS_MATRIXCONF[i], OUTPUT);
  }

  // Démarrer le serveur
  server.begin();
}

void loop() {
  // La boucle reste vide car les LEDs sont maintenant contrôlées 
  // par les événements de drag and drop
}

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
    
    // Allumer les LEDs des effets utilisés
    for(JsonVariant v : order) {
        int effectNum = v.as<String>().toInt();
        if(effectNum > 0 && effectNum <= NUM_LEDS) {
            ledStates[effectNum - 1] = true;
            digitalWrite(LED_PINS[effectNum - 1], HIGH);
        }
    }
    
    // Obtenir et retourner la configuration de routage
    return getMuteConfig(order);
}

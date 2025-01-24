#include "WiFi.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include "HW_SendRsV3.h"
#include <ArduinoJson.h> // Utilisé pour traiter les données JSON

const char* ssid = "KNET_34db_2.4Ghz";
const char* password = "Coolight2016@Stpol@Ruche";

// LED sur GPIO2
const int ledPin = 2;

const int NUM_LEDS = 4;
const int LED_PINS[NUM_LEDS] = {15, 4, 22, 23};  // D2, D4, D22, D23
bool ledStates[NUM_LEDS] = {false, false, false, false};
unsigned long previousMillis = 0;
const long interval = 100;  // Intervalle de 100ms entre chaque LED
int currentLed = 0;

AsyncWebServer server(80);

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
    
    // Initialisation du muteConfig par défaut : chaque out_x sur in_x
    String muteConfig = "11100100"; // "11 10 01 00" - Chaque canal bouclé sur lui-même
                                   // out4->in4 (11)
                                   // out3->in3 (10)
                                   // out2->in2 (01)
                                   // out1->in1 (00)

    // Réinitialiser l'état des LEDs
    for(int i = 0; i < NUM_LEDS; i++) {
        ledStates[i] = false;
        digitalWrite(LED_PINS[i], LOW);
    }

    // Si le rack de droite n'est pas vide, appliquer le routage spécifique
    if (order.size() > 0) {
        // Allumer les LEDs des effets utilisés
        for(size_t i = 0; i < order.size(); i++) {
            String effect = order[i].as<String>();
            int effectNum = effect.toInt();
            
            if(effectNum > 0 && effectNum <= NUM_LEDS) {
                ledStates[effectNum - 1] = true;
                digitalWrite(LED_PINS[effectNum - 1], HIGH);
            }
        }

        // Configurer le routage
        String effect = order[0].as<String>();
        int effectNum = effect.toInt();
        
        // Configurer out4_pc vers l'entrée du premier effet
        switch(effectNum) {
            case 1: // SPX (in1) : 00 pour out4
                muteConfig.setCharAt(7, '0');
                muteConfig.setCharAt(6, '0');
                // Configurer la sortie de SPX vers in4_pc
                muteConfig.setCharAt(1, '1');
                muteConfig.setCharAt(0, '1');
                break;
            case 2: // DIGI (in2) : 01 pour out4
                muteConfig.setCharAt(7, '0');
                muteConfig.setCharAt(6, '1');
                // Configurer la sortie de DIGI vers in4_pc
                muteConfig.setCharAt(3, '1');
                muteConfig.setCharAt(2, '1');
                break;
            case 3: // BBE (in3) : 10 pour out4
                muteConfig.setCharAt(7, '1');
                muteConfig.setCharAt(6, '0');
                // Configurer la sortie de BBE vers in4_pc
                muteConfig.setCharAt(5, '1');
                muteConfig.setCharAt(4, '1');
                break;
        }
    }

    // Convertir la configuration binaire en octet
    String reversedMuteConfig = "";
    for(int i = 7; i >= 0; i--) {
        reversedMuteConfig += muteConfig[i];
    }
    uint8_t muteConfigByte = strtol(reversedMuteConfig.c_str(), nullptr, 2);

    // Debug: Afficher les valeurs
    Serial.println("-------- Debug Values --------");
    Serial.print("muteConfig (binary): ");
    Serial.println(muteConfig);
    Serial.print("reversedMuteConfig (binary): ");
    Serial.println(reversedMuteConfig);
    Serial.print("muteConfigByte (decimal): ");
    Serial.println(muteConfigByte);
    Serial.print("muteConfigByte (hex): 0x");
    Serial.println(muteConfigByte, HEX);
    Serial.println("--------------------------");

    // Envoi de la commande NewPreset
    SendRsBuffer[0] = 1;  // Numéro de preset
    SendRsBuffer[1] = muteConfigByte;  // Configuration binaire inversée
 

 

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


// Fonction de configuration de la matrice en fonction de l'ordre des effets
void configureMatrix(String order) {
  // Exemple de traitement de l'ordre pour appliquer la table de vérité
  Serial.println("Configuring matrix with order: " + order);
  String config = ""; // Configuration binaire en fonction de la table de vérité

  // Ajout du calcul binaire selon l'ordre
  // Par exemple, ajouter les valeurs en fonction de la table de vérité
  if (order.indexOf("1") >= 0) config += "10"; // Effet 1
  if (order.indexOf("2") >= 0) config += "01"; // Effet 2
  if (order.indexOf("3") >= 0) config += "11"; // Effet 3
  config += "11"; // Insert (toujours actif)

  // Convertir et envoyer la configuration
  uint8_t byteConfig = strtol(config.c_str(), nullptr, 2);
  mySerial.write(byteConfig);
}

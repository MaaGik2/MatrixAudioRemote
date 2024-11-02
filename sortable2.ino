#include "WiFi.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include "HW_SendRsV3.h"
#include <ArduinoJson.h> // Utilisé pour traiter les données JSON

const char* ssid = "KNET_34db_2.4Ghz";
const char* password = "Coolight2016@Stpol@Ruche";

// LED sur GPIO2
const int ledPin = 2;

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


  // Route pour recevoir l'ordre de la liste 2 via une requête POST JSON
  server.on("/update-order", HTTP_POST, [](AsyncWebServerRequest * request) {}, NULL,
  [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Lire le corps de la requête
    String body = "";
    for (size_t i = 0; i < len; i++) {
      body += (char)data[i];
    }

    Serial.println("Received body:");
    Serial.println(body);

    // Analyse du JSON
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
    Serial.println("Nouvel ordre de la liste 2:");
    for (String item : order) {
      Serial.println(item);
    }

    // Clignotement de la LED lorsque les données sont envoyées
    blinkLED();

    // Réponse
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

  // Démarrer le serveur
  server.begin();
}

void loop() {
  // Pas de code nécessaire dans la boucle principale
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

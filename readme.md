| Action                                 | Commande                          | Explication                                           |
|----------------------------------------|---------------------------------|-----------------------------------------------------|
| Compiler le projet                     | `pio run`                       | Compile le firmware sans téléverser                  |
| Téléverser le firmware                 | `pio run --target upload`       | Compile et téléverse le firmware sur l’ESP32        |
| Ouvrir le moniteur série à 115200 bauds | `pio device monitor --baud 115200` | Affiche les messages du port série (debug, logs)    |
| Arrêter le moniteur série              | `Ctrl+C`                       | Ferme le moniteur dans le terminal                    |
| Uploader les fichiers SPIFFS/LittleFS | `pio run --target uploadfs`     | Téléverse le contenu du dossier `data/` dans l’ESP32|
| Changer de port série (ex : COM9)     | Ajouter dans `platformio.ini` :<br>`upload_port = COM9`<br>`monitor_port = COM9` | Définit le port pour l’upload et le moniteur série  |
| Nettoyer le projet                     | `pio run --target clean`        | Supprime les fichiers de compilation                  |



1. **ArduinoJson**
   - Version : 7.4.1
   - Auteur : Benoit Blanchon
   - URL : https://github.com/bblanchon/ArduinoJson

2. **ESP Async WebServer**
   - Version : 1.2.4
   - Auteur : Hristo Gochkov
   - URL : https://github.com/me-no-dev/ESPAsyncWebServer

3. **ESPAsyncTCP** (dépendance pour ESP8266)
   - Version : 1.2.2
   - Auteur : Hristo Gochkov
   - URL : https://github.com/me-no-dev/ESPAsyncTCP

4. **AsyncTCP** (pour ESP32)
   - Version : 1.1.1
   - Auteur : Hristo Gochkov
   - URL : https://github.com/me-no-dev/AsyncTCP

Ces bibliothèques sont automatiquement installées par PlatformIO lors de la première compilation du projet. Si vous souhaitez installer manuellement ces versions spécifiques, vous pouvez utiliser les commandes suivantes dans PlatformIO :

```bash
pio lib install "bblanchon/ArduinoJson@7.4.1"
pio lib install "me-no-dev/ESP Async WebServer@1.2.4"
```

Les dépendances AsyncTCP et ESPAsyncTCP seront automatiquement installées car elles sont des dépendances requises par ESP Async WebServer.

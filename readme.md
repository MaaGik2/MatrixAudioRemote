| Action                                 | Commande                          | Explication                                           |
|----------------------------------------|---------------------------------|-----------------------------------------------------|
| Compiler le projet                     | `pio run`                       | Compile le firmware sans téléverser                  |
| Téléverser le firmware                 | `pio run --target upload`       | Compile et téléverse le firmware sur l’ESP32        |
| Ouvrir le moniteur série à 115200 bauds | `pio device monitor --baud 115200` | Affiche les messages du port série (debug, logs)    |
| Arrêter le moniteur série              | `Ctrl+C`                       | Ferme le moniteur dans le terminal                    |
| Uploader les fichiers SPIFFS/LittleFS | `pio run --target uploadfs`     | Téléverse le contenu du dossier `data/` dans l’ESP32|
| Changer de port série (ex : COM9)     | Ajouter dans `platformio.ini` :<br>`upload_port = COM9`<br>`monitor_port = COM9` | Définit le port pour l’upload et le moniteur série  |
| Nettoyer le projet                     | `pio run --target clean`        | Supprime les fichiers de compilation                  |

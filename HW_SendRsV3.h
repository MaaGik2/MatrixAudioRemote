#ifndef HW_SENDRSV3_H
#define HW_SENDRSV3_H

#include <Arduino.h>

// Définitions des messages pour le protocole
//  -New Protocol V3 � Universal Protocol !
// Command Standart

#define AboutVersion                  6
#define PresetChange                  18
#define NewPreset                     19
#define GpiINChange                   20
#define GpiOUTChange                  21

// Déclaration de mySerial comme extern
extern HardwareSerial mySerial;

// Déclaration de la fonction pour la configuration série
void setupSerialConfig();

// Déclaration de la fonction
void HW_SendRsV3(uint8_t Command, uint8_t ExCommand, uint16_t NbData);



// Définir la taille maximale de données si nécessaire
const int MAX_DATA_LENGTH = 256;
extern uint8_t SendRsData[MAX_DATA_LENGTH];
extern uint8_t SendRsBuffer[MAX_DATA_LENGTH]; // Buffer pour les données à envoyer

void ClearSendRSBuff();

#endif // HW_SENDRSV3_H

#include "HW_SendRsV3.h"

uint8_t SendRsData[MAX_DATA_LENGTH];
uint8_t SendRsBuffer[MAX_DATA_LENGTH]; // Buffer pour les données à envoyer


HardwareSerial mySerial(1);


void setupSerialConfig() {
    mySerial.begin(9600, SERIAL_8N1, 16, 17); // TX1 sur GPIO 16, RX1 sur GPIO 17
}


void ClearSendRSBuff() {
    memset(SendRsBuffer, 0, sizeof(SendRsBuffer));
}

void HW_SendRsV3(uint8_t Command, uint8_t ExCommand, uint16_t NbData) {
  // la desactivation d'une commande se fait par un 0, 
  
  // Excommand : 256 command et 128 + 256 data max
  // Command   : 32 command et 8 data MAx !
  // la data sont dans le SendRsBuffer

  
    uint8_t CheckSum;
    uint8_t ComNbCompact;
    uint16_t i;
    uint16_t Pcs;

  //  TX485_DE_ENABLE; // Active la transmission

    Pcs = 0;
    CheckSum = 0;

    // Calcul du nombre compact de commandes
    if (ExCommand == 0) {
        ComNbCompact = (Command << 3) + ((uint8_t)NbData);
    } else {
        ComNbCompact = Command;
    }

    // Alignement des données
    for (i = 0; i < NbData; i++) {
        if (ExCommand == 0) {
            SendRsData[2 + i] = SendRsBuffer[i];
        } else {
            SendRsData[4 + i] = SendRsBuffer[i];
        }
    }

    // Entête
    SendRsData[0] = 13;

    if (ExCommand == 0) { // Commande normale
        SendRsData[1] = ComNbCompact;
        NbData = NbData + 2; // On ajoute l'entête pour le checksum
    } else { // Commande étendue
        SendRsData[1] = 0xFF;
        SendRsData[2] = ComNbCompact;
        SendRsData[3] = ((uint8_t)NbData);
        NbData = NbData + 4;
    }

    // Calcul du checksum
    for (i = 0; i < NbData; i++) {
        Pcs += SendRsData[i];
    }

    CheckSum = Pcs;

    SendRsData[NbData] = CheckSum; // On finit par le checksum

    // Envoyer les données via Serial
    mySerial.write(SendRsData, NbData + 1); // Envoi des données par UART

    ClearSendRSBuff(); // Effacer le buffer après envoi
}

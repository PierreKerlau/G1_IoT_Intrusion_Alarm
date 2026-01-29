#include <Arduino.h>
#include "security_code.h"
#include "motion_detector.h"

bool isAlarmActive = false;

void setup() {
  Serial.begin(9600);
  setupMotion();
  setupSecurity();
  Serial.println("SYSTEME ARME : En attente de mouvement...");
}

void loop() {
  // CAS 1 : SURVEILLANCE
  if (!isAlarmActive) {
    if (checkMotion() == true) {
      
      // >>> C'EST ICI LA CORRECTION <<<
      // Avant de lancer l'alarme, on remet le digicode à zéro (et on l'allume)
      resetAlarmState(); 
      
      isAlarmActive = true;
      Serial.println("MOUVEMENT DETECTE ! Entrez le code !");
    }
    delay(100); 
  } 
  
  // CAS 2 : ALARME EN COURS
  else {
    // On fait tourner la logique du code
    bool disarmed = runSecurityLogic();
    
    // Si le code est bon
    if (disarmed == true) {
      isAlarmActive = false; // On coupe l'alarme
      Serial.println("Systeme desactive. Retour en surveillance dans 3 sec...");
      delay(3000); 
      Serial.println("SYSTEME ARME : En attente de mouvement...");
    }
  }
}
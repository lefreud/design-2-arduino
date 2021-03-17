#include <Wire.h>

void initPID() {
  Wire.begin();
  
  // Creation du timer interrupt
  noInterrupts();

  // Le 1 signifie le timer 1, avec un compteur de 16 bits
  TCCR1A = 0; // OC1A et OC1B sont déconnectés

  TCCR1B = 0;

  OCR1A = 16000; // Maximum counter value before clear, set for 1 kHz
  TCCR1B |= (1 << WGM12); // Clear timer on compare match (CTC)
  //TCCR1B |= (1 << CS10); // No prescaler
  //TCCR1B |= (1 << CS11); // 8x prescaler
  TCCR1B |= (1 << CS11) | (1 << CS10); // 64x prescaler
  
  TIMSK1 |= (1 << OCIE1A); // enable comparator 1 interrupt

  // start counter at 0
  TCNT1 = 0;

  // output compare register set to maximum

  interrupts();
}

float sommeErreurs = 0.0;
float erreur = 0;
float getTensionCommandePI(float tensionActuelle) {
  erreur = TENSION_CONSIGNE - tensionActuelle;

  float termeProportionnel = CONSTANTE_PROPORTIONNELLE * erreur;
  float termeIntegrale = CONSTANTE_INTEGRALE * (sommeErreurs + erreur) * DELTA_TEMPS;

  float commande = termeProportionnel + termeIntegrale;

  // verification de securite et anti-windup
  if (commande > TENSION_COMMANDE_MAX) {
    commande = TENSION_COMMANDE_MAX;
  } else if (commande < 0) {
    commande = 0; 
  } else {
    // pas de saturation, donc pas besoin d'anti-windup on peut donc additionner les erreurs
    sommeErreurs += erreur;
  }
  
  return commande;
}

float getTensionPosition() {
  return (5.0 / 1024) * analogRead(CAPTEUR_POSITION_PIN);
}

float envoyerCommande(float commandeTension) {
  int commandeTensionDiscret = (int) (4096 / 5.0) * commandeTension;
  
  // SORTIE DE LA COMMANDE DAC
  // https://ww1.microchip.com/downloads/en/DeviceDoc/22039d.pdf
  Wire.beginTransmission(DAC_I2C_ADDRESS);

  // mise a jour DAC
  Wire.write(64);
  
  // 8 MSB bits
  Wire.write(commandeTensionDiscret >> 4);
  
  // 4 LSB bits
  Wire.write((commandeTensionDiscret & 0b1111) << 4);
  
  Wire.endTransmission();  
}

ISR(TIMER1_COMPA_vect) {
    // TODO: Code qui s'execute a chaque interruption du timer
}

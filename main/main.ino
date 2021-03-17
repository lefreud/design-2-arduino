const int DAC_I2C_ADDRESS = 0x60;
const int CAPTEUR_POSITION_PIN = A8;
const int CAPTEUR_COURANT_PIN = A10;

// PID
// const float TENSION_CONSIGNE = 2.05;
const float TENSION_CONSIGNE = 1.8;
const float DELTA_TEMPS = 0.064; // en secondes
const float CONSTANTE_PROPORTIONNELLE = 0.001;
const float CONSTANTE_INTEGRALE = 0.1;
const float TENSION_COMMANDE_MAX = 1.6;
const float TENSION_COMMANDE_MIN = 0.7;

// TODO: Enlever cette variable globale
float tensionCapteurCourant = 0;

void setup() {
  Serial.begin(9600);
  initPID();
  initLcd();
  pinMode(50, OUTPUT);
}

bool out = HIGH;
void loop() {
  if (out == HIGH) {
    out = LOW;
  } else {
    out = HIGH;
  }
  digitalWrite(50, out);
  
  // Lecture masse
  tensionCapteurCourant = (5.0/1024) * analogRead(CAPTEUR_COURANT_PIN);
  float masseMesuree = getMasseAPartirDeCourant(tensionCapteurCourant);

  // LCD
  lireEntrees();
  setMasse(masseMesuree);
  ecrireSorties();

  // PID
  float tensionPosition = getTensionPosition();
  float tensionCommande = getTensionCommandePI(tensionPosition);
  envoyerCommande(tensionCommande);

  setTension(tensionPosition);
  // Creation du serial plot
  Serial.print("consigne:"); Serial.print(TENSION_CONSIGNE); Serial.print(" ");
  Serial.print("capteur:"); Serial.print(tensionPosition); Serial.print(" ");
  Serial.print("commande:"); Serial.print(tensionCommande); Serial.print("\n");
}

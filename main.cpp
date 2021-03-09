int capteurPositionPin = A0;
int commandePin = 2;

// PID
const float TENSION_CONSIGNE = 3.5;
const float DELTA_TEMPS = 0.001; // en secondes
const float CONSTANTE_PROPORTIONNELLE = 1;
const float CONSTANTE_INTEGRALE = 1;
const float CONSTANTE_DERIVEE = 1;
const float TENSION_COMMANDE_MAX = 1.3;
const float TENSION_COMMANDE_MIN = 0.7;

float derniereTension = 0.0;
float sommeErreurs = 0.0;

void afficherTensionPosition(float tensionPosition) {
  Serial.print("Tension de position: ");
  Serial.print(tensionPosition);
  Serial.println(" V");
}

void afficherCommande(float commande) {
  Serial.print("Commande: ");
  Serial.print(commande);
  Serial.println(" V");
}

float getTensionCommandePID(float tensionActuelle) {
  float erreur = TENSION_CONSIGNE - tensionActuelle;

  float termeProportionnel = CONSTANTE_PROPORTIONNELLE * erreur;
  float termeIntegrale = CONSTANTE_INTEGRALE * (sommeErreurs + erreur) * DELTA_TEMPS;
  float termeDerivee = CONSTANTE_DERIVEE * (tensionActuelle - derniereTension) / DELTA_TEMPS;

  float commande = termeProportionnel + termeIntegrale + termeDerivee;

  // mise a jour des variables
  derniereTension = tensionActuelle;
  sommeErreurs += erreur; // a valider

  // verification de securite
  if (commande > TENSION_COMMANDE_MAX) {
    // Serial.println("Attention! tension de commande maximale.");
    commande = TENSION_COMMANDE_MAX;
  } else if (commande < TENSION_COMMANDE_MIN) {
    // Serial.println("Attention! tension de commande minimale.");
    commande = TENSION_COMMANDE_MIN;
  }
  
  return commande;
}

ISR(TIMER1_COMPA_vect) {
    // Add code here that gets executed each time the timer interrupt is triggered
    int capteurPositionValue = analogRead(capteurPositionPin);
    float tensionPosition = (5.0 / 1024) * capteurPositionValue;
    float commandeTension = getTensionCommandePID(tensionPosition);
    int commandeTensionDiscret = (int) (255 / 5.0) * commandeTension;
    analogWrite(commandePin, commandeTensionDiscret);
    Serial.print("consigne:"); Serial.print(TENSION_CONSIGNE); Serial.print(" ");
    Serial.print("capteur:"); Serial.print(tensionPosition); Serial.print(" ");
    Serial.print("commande:"); Serial.print(commandeTension); Serial.print("\n");
}

void setup() {
  pinMode(commandePin, OUTPUT);

  Serial.begin(9600);

  // setup timer interrupts
  noInterrupts();

  // Le 1 signifie le timer 1, avec un compteur de 16 bits
  TCCR1A = 0; // OC1A et OC1B sont déconnectés

  TCCR1B = 0;

  OCR1A = 16000; // Maximum counter value before clear, set for 1 kHz
  TCCR1B |= (1 << WGM12); // Clear timer on compare match (CTC)
  TCCR1B |= (1 << CS10); // No prescaler

  //TIMSK1 = 0;
  TIMSK1 |= (1 << OCIE1A); // enable comparator 1 interrupt

  // start counter at 0
  TCNT1 = 0;

  // output compare register set to maximum

  interrupts();
}

void loop() {}

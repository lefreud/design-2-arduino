#include <LiquidCrystal.h>
#include <Wire.h>

const int DAC_I2C_ADDRESS = 0x60;
const int CAPTEUR_POSITION_PIN = A8;

// PID
const float TENSION_CONSIGNE = 3.5;
const float DELTA_TEMPS = 0.001; // en secondes
const float CONSTANTE_PROPORTIONNELLE = 1;
const float CONSTANTE_INTEGRALE = 10;
const float CONSTANTE_DERIVEE = 0;
const float TENSION_COMMANDE_MAX = 1.3;
const float TENSION_COMMANDE_MIN = 0.7;

float derniereTension = 0.0;
float sommeErreurs = 0.0;

// Fonctions des boutons
const int TAILLEARRAYMASSESMOYENNES = 55;
const int TYPESDEPIECE = 10;
const int NOMBREDEPIECESTOTALPOSSIBLE = 5;
int indiceUniteDeLaMasse = 0; // 0 si c'est en gramme et 1 si c'est en oz
int buttonsState = 0; // État des boutons live
int lastButtonState = 0; // État précédent des boutons
String messageLigneDuHaut = "Bienvenue!";
String messageLigneDuBas;
float masseDeQualibrage = 0.00;
float massesMoyennes[TAILLEARRAYMASSESMOYENNES] = {0.0};
float masseMoyenne = 0.0;
int indiceArrayDeMoyenne = 0;

// On intialise la librairie avec les pins utilisées
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// On définit la valeur des boutons et des clés utilisés
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

// Fonction pour l'étalonnage
float etalonnageBalance(){
  ecrireSorties();
  while(analogRead(0) > 1000){
    
  }
}

// Fonction pour obtenir la moyenne de la masse
float getMasseMoyenne(){
  float averageMasse = 0;
  int i = 0;
  do {
    averageMasse += massesMoyennes[i];
    i++;
  } while (i < TAILLEARRAYMASSESMOYENNES);
  averageMasse = averageMasse/TAILLEARRAYMASSESMOYENNES;
  return averageMasse;
}

// Fonction pour changer de grammes à ounces
String uniteDeLaMasse(float masse) {
  String masseAvecUnites;
  if (indiceUniteDeLaMasse == 0){
    masseAvecUnites = String(masse) + " g";
    indiceUniteDeLaMasse = 1;
  }
  else{
    masseAvecUnites = String(masse/28.35) + " oz";
    indiceUniteDeLaMasse = 0;
  }
  return masseAvecUnites;
}

// Fonction pour identifier le type de pièces
String typeDePiece(float massePesee) {
  String typeDePiece;
  float mesMasses[] = {3.95, 4.6, 1.75, 2.07, 4.40, 5.05, 6.27, 7.00, 6.92, 7.3};
  String identificationMasses[] = {" x 0.05$"," x 0.05$"," x 0.10$"," x 0.10$"," x 0.25$"," x 0.25$"," x 1.00$"," x 1.00$"," x 2.00$"," x 2.00$"};
  float dist = INFINITY;
  for(int i = 0; i < TYPESDEPIECE; i++){
    for(int x = 1; x <= NOMBREDEPIECESTOTALPOSSIBLE; x++){
      if(abs(mesMasses[i] - massePesee/x) < dist and (abs(mesMasses[i] - massePesee/x)/(mesMasses[i]))<0.03){
        dist = abs(mesMasses[i] - massePesee/x);
        typeDePiece = String(x) + identificationMasses[i];
      }
    }
  }
  if (dist == INFINITY){
    typeDePiece = "Mauvaise piece";
  }
  return typeDePiece;
}

// Fonction pour changer la masse tare de la balance
float masseTare (float masse) {
  masseDeQualibrage = masse;
  return masseDeQualibrage;
}

void lireEntrees(){ // Fonction pour lire les entrées
  buttonsState = analogRead(0);
}

void ecrireSorties(){ // Fonction pour écrire les sorties
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(messageLigneDuHaut);
  lcd.setCursor(0,1);
  lcd.print(messageLigneDuBas);
}

void setMasse(float masse){
  massesMoyennes[indiceArrayDeMoyenne] = masse;
  indiceArrayDeMoyenne++;
  indiceArrayDeMoyenne %= TAILLEARRAYMASSESMOYENNES;
  if (buttonsState != lastButtonState and buttonsState != lastButtonState + 1 and buttonsState != lastButtonState - 1) {
    if (buttonsState < 60) { // Quand on clique sur le bouton right
      messageLigneDuHaut = "Nouvelle masse tare:";
      messageLigneDuBas = masseTare(masse);
    }
    else if (buttonsState < 200) {
      lcd.print ("Up    ");
    }
    else if (buttonsState < 400){
      lcd.print ("Down  ");
    }
    else if (buttonsState < 600){ // Quand on clique sur le bouton left
      masseMoyenne = getMasseMoyenne();
      messageLigneDuHaut = "Authentification";
      messageLigneDuBas = typeDePiece(masseMoyenne);
    }
    else if (buttonsState < 800){ // Quand on clique sur le bouton select
      masseMoyenne = getMasseMoyenne();
      messageLigneDuHaut = "Masse totale:";
      messageLigneDuBas = uniteDeLaMasse(masseMoyenne);
    }
  }
  lastButtonState = buttonsState; // On sauvegarde l'état actuel comme étant l'état précédent
}


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

  // verification de securite et anti-windup
  if (commande > TENSION_COMMANDE_MAX) {
    // Serial.println("Attention! tension de commande maximale.");
    commande = TENSION_COMMANDE_MAX;
  } else if (commande < TENSION_COMMANDE_MIN) {
    // Serial.println("Attention! tension de commande minimale.");
    commande = TENSION_COMMANDE_MIN;
  } else {
    // pas de saturation, donc pas besoin d'anti-windup on peut donc additionner les erreurs
    sommeErreurs += erreur;
  }
  
  return commande;
}

int capteurPositionValue = 0;
float tensionPosition = 0;
float commandeTension = 0;

int commandeTensionDiscret = 0;

ISR(TIMER1_COMPA_vect) {
    // Code qui s'execute a chaque interruption du timer
    capteurPositionValue = analogRead(CAPTEUR_POSITION_PIN);
    tensionPosition = (5.0 / 1024) * capteurPositionValue;
    commandeTension = getTensionCommandePID(tensionPosition);
    commandeTensionDiscret = (int) (4096 / 5.0) * commandeTension;
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  Wire.begin();
  // setup timer interrupts
  noInterrupts();

  // Le 1 signifie le timer 1, avec un compteur de 16 bits
  TCCR1A = 0; // OC1A et OC1B sont déconnectés

  TCCR1B = 0;

  OCR1A = 16000; // Maximum counter value before clear, set for 1 kHz
  TCCR1B |= (1 << WGM12); // Clear timer on compare match (CTC)
  //TCCR1B |= (1 << CS10); // No prescaler
  TCCR1B |= (1 << CS11); // 8x prescaler

  TIMSK1 |= (1 << OCIE1A); // enable comparator 1 interrupt

  // start counter at 0
  TCNT1 = 0;

  // output compare register set to maximum

  interrupts();
}

void loop() {

  Serial.print("consigne:"); Serial.print(TENSION_CONSIGNE); Serial.print(" ");
  Serial.print("capteur:"); Serial.print(tensionPosition); Serial.print(" ");
  Serial.print("commande:"); Serial.print(commandeTension); Serial.print("\n");
  float masse = 7.3; // TODO: change this
  lireEntrees();
  setMasse(masse - masseDeQualibrage);
  ecrireSorties();

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

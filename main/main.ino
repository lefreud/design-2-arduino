#include <LiquidCrystal.h>
#include <Wire.h>

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

float sommeErreurs = 0.0;

// Fonctions des boutons
const int TAILLEARRAYMASSESMOYENNES = 10;
const int TYPESDEPIECE = 10;
const int NOMBREDEPIECESTOTALPOSSIBLE = 3;
int buttonsState = 0; // État des boutons live
int lastButtonState = 0; // État précédent des boutons
String messageLigneDuHaut = "Bienvenue!";
String messageLigneDuBas;
float masseDeQualibrage = 0.00;
float massesMoyennes[TAILLEARRAYMASSESMOYENNES] = {0.0};
float masseMoyenne = 0.0;
int indiceArrayDeMoyenne = 0;
float tensionCapteurCourant = 0;

// On intialise la librairie avec les pins utilisées
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Boutons keypad
const int BTN_VALUE_MARGIN = 5;

const int BTN_SELECT = 720;
const int BTN_LEFT = 476;
const int BTN_UP = 130;
const int BTN_DOWN = 304;
const int NO_BTN = 1023;

const int MODE_MASSE_TOTALE = 0;
const int MODE_COMPTAGE = 1;
const int MODE_TARE = 2;
const int MODE_ETALONNAGE = 3;
const int UNITE_GRAMME = 0;
const int UNITE_ONCE = 1;

int mode = MODE_ETALONNAGE;
int indiceUniteDeLaMasse = UNITE_GRAMME;
int indexDeEtalonnage = 0;

// variable pour le calcul ax+b de l'étalonnage
float tension0g = 0.0;
float tension100g = 0.0;
float masse0g = 0.0;
float masse100g = 100.0;
float penteDroiteEtalonnage = 0.0;
float bDroiteEtalonnage = 0.0;

float getMasseAPartirDeCourant(float tensionCapteurCourant) {
  return penteDroiteEtalonnage * tensionCapteurCourant + bDroiteEtalonnage;
}

float getMasseInstantanee() {
  return massesMoyennes[indiceArrayDeMoyenne] - masseDeQualibrage;
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
  return averageMasse - masseDeQualibrage;
}

// Fonction pour changer de grammes à ounces
String uniteDeLaMasse(float masse) {
  String masseAvecUnites;
  if (indiceUniteDeLaMasse == UNITE_GRAMME){
    masseAvecUnites = String((float)((round(masse*10))/10.0),1) + " g";
  }
  else{
    masseAvecUnites = String(masse/28.35) + " oz";
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
      if(abs(x*mesMasses[i] - massePesee) < dist and (abs(x*mesMasses[i] - massePesee)/(x*mesMasses[i]))<0.03){
        dist = abs(x*mesMasses[i] - massePesee);
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
float masseTare () {
  masseDeQualibrage = massesMoyennes[indiceArrayDeMoyenne];
}

bool isBoutonSelectionne(int bouton) {
  return abs(buttonsState - bouton) < BTN_VALUE_MARGIN;
}

void lireEntrees(){ // Fonction pour lire les entrées
  buttonsState = analogRead(0);
  //Serial.println(buttonsState);
  if (buttonsState >= lastButtonState + 2 or buttonsState <= lastButtonState - 2) {
    if (mode == MODE_ETALONNAGE) {
      if (isBoutonSelectionne(BTN_UP)) {
        if (indexDeEtalonnage == 0) {
          indexDeEtalonnage++;
        } else if (indexDeEtalonnage == 1) {
          tension0g = tensionCapteurCourant;
          indexDeEtalonnage++;
        } else if (indexDeEtalonnage == 2) {
          tension100g = tensionCapteurCourant;
          penteDroiteEtalonnage = (masse100g - masse0g)/(tension100g - tension0g);
          bDroiteEtalonnage = masse100g - penteDroiteEtalonnage * tension100g;
          indexDeEtalonnage++;
        } else if (indexDeEtalonnage == 3) {
          mode = MODE_MASSE_TOTALE;
        }
      }
    }
    else {
    if (isBoutonSelectionne(BTN_DOWN)) { // Quand on clique sur le bouton down
      mode = MODE_TARE;
      masseTare();
    }
    else if (isBoutonSelectionne(BTN_LEFT)){ // Quand on clique sur le bouton left
      mode = MODE_COMPTAGE;
    }
    else if (isBoutonSelectionne(BTN_SELECT)){ // Quand on clique sur le bouton select
      mode = MODE_MASSE_TOTALE;
      if (indiceUniteDeLaMasse == UNITE_GRAMME) {
        indiceUniteDeLaMasse = UNITE_ONCE;
      } else {
        indiceUniteDeLaMasse = UNITE_GRAMME;
      }
    }
    }
  }
  lastButtonState = buttonsState;
}

void ecrireSorties(){ // Fonction pour écrire les sorties
  masseMoyenne = getMasseMoyenne();
  String derniereLigneHaut = messageLigneDuHaut;
  String derniereLigneBas = messageLigneDuBas;
  if (mode == MODE_COMPTAGE) {
    messageLigneDuHaut = "Authentification";
    messageLigneDuBas = typeDePiece(masseMoyenne);
  } else if (mode == MODE_MASSE_TOTALE) {
    messageLigneDuHaut = "Masse totale";
    messageLigneDuBas = uniteDeLaMasse(masseMoyenne);
  } else if (mode == MODE_TARE) {
    messageLigneDuHaut = "Tare";
    messageLigneDuBas = uniteDeLaMasse(getMasseInstantanee());
  } else if (mode == MODE_ETALONNAGE) {
    if (indexDeEtalonnage == 0) {
      messageLigneDuHaut = "Etalonnage Pt. 1";
      messageLigneDuBas = "Voir Manuel";
    } else if (indexDeEtalonnage == 1) {
      messageLigneDuHaut = "Ne rien mettre";
      messageLigneDuBas = "sur la balance.";
    } else if (indexDeEtalonnage == 2) {
      messageLigneDuHaut = "Mettre 100 grammes";
      messageLigneDuBas = "sur la balance.";
    } else if (indexDeEtalonnage == 3) {
      messageLigneDuHaut = "Etalonnage";
      messageLigneDuBas = "complete!";
    }
  }
  if (derniereLigneHaut != messageLigneDuHaut || derniereLigneBas != messageLigneDuBas) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(messageLigneDuHaut);
    lcd.setCursor(0,1);
    lcd.print(messageLigneDuBas);
  }
  
}

void setMasse(float masse){
  massesMoyennes[indiceArrayDeMoyenne] = masse;
  indiceArrayDeMoyenne++;
  indiceArrayDeMoyenne %= TAILLEARRAYMASSESMOYENNES;
}

float erreur = 0;
float getTensionCommandePID(float tensionActuelle) {
  erreur = TENSION_CONSIGNE - tensionActuelle;

  float termeProportionnel = CONSTANTE_PROPORTIONNELLE * erreur;
  float termeIntegrale = CONSTANTE_INTEGRALE * (sommeErreurs + erreur) * DELTA_TEMPS;

  float commande = termeProportionnel + termeIntegrale;

  // verification de securite et anti-windup
  if (commande > TENSION_COMMANDE_MAX) {
    // Serial.println("Attention! tension de commande maximale.");
    commande = TENSION_COMMANDE_MAX;
  } else if (commande < 0) {
    commande = 0; 
  }
  else {
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
    /*
    capteurPositionValue = analogRead(CAPTEUR_POSITION_PIN);
    tensionPosition = (5.0 / 1024) * capteurPositionValue;
    commandeTension = getTensionCommandePID(tensionPosition);
    commandeTensionDiscret = (int) (4096 / 5.0) * commandeTension;
    */
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
  //TCCR1B |= (1 << CS11); // 8x prescaler
  TCCR1B |= (1 << CS11) | (1 << CS10); // 64x prescaler
  
  TIMSK1 |= (1 << OCIE1A); // enable comparator 1 interrupt

  // start counter at 0
  TCNT1 = 0;

  // output compare register set to maximum

  interrupts();
}

void loop() {

  capteurPositionValue = analogRead(CAPTEUR_POSITION_PIN);
  tensionPosition = (5.0 / 1024) * capteurPositionValue;
  commandeTension = getTensionCommandePID(tensionPosition);
  commandeTensionDiscret = (int) (4096 / 5.0) * commandeTension;
  Serial.print("consigne:"); Serial.print(TENSION_CONSIGNE); Serial.print(" ");
  Serial.print("capteur:"); Serial.print(tensionPosition); Serial.print(" ");
  Serial.print("commande:"); Serial.print(commandeTension); Serial.print("\n");
  tensionCapteurCourant = (5.0/1024) * analogRead(CAPTEUR_COURANT_PIN);
  float masseMesuree = getMasseAPartirDeCourant(tensionCapteurCourant);
  // float masseMesuree = tensionCapteurCourant * (100.0 / 5); // TODO: change this
  lireEntrees();
  setMasse(masseMesuree);
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

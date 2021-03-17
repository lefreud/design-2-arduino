#include <LiquidCrystal.h>

const int TAILLEARRAYMASSESMOYENNES = 10;
const int TAILLEARRAYTENSIONSMOYENNES = TAILLEARRAYMASSESMOYENNES;
const int TYPESDEPIECE = 10;
const int NOMBREDEPIECESTOTALPOSSIBLE = 10;
const int NOMBRE_COURANTS_MOYENNE = 5000;
const float MARGE_STABILITE_POSITION = 0.08;

int buttonsState = 0; // État des boutons live
int lastButtonState = 0; // État précédent des boutons
String messageLigneDuHaut;
String messageLigneDuBas;
float masseDeQualibrage = 0.00;
float massesMoyennes[TAILLEARRAYMASSESMOYENNES] = {0.0};
float masseMoyenne = 0.0;
int indiceArrayMoyenneDeMasse = 0;
float tensionsMoyennes[TAILLEARRAYTENSIONSMOYENNES] = {0.0};
int indiceArrayMoyenneDeTension = 0;

// Comptage des pièces
int indiceComptageDesPieces = 0;
int nombreDeTypeDePieces = 5;
String identificationPieces[] = {"0.05$","0.10$","0.25$","1.00$","2.00$"};
float identificationMasses[] = {3.95,1.75,4.40,6.27,6.92};
String pieceChoisie;
String piece = "";

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
const int MODE_SELECTION = 1;
const int MODE_ETALONNAGE = 2;
const int MODE_COMPTAGE = 3;
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

void initLcd() {
  lcd.begin(16, 2);
}

float getMasseAPartirDeCourant(float tensionCapteurCourant) {
  return penteDroiteEtalonnage * tensionCapteurCourant + bDroiteEtalonnage;
}

float getMasseInstantanee() {
  return massesMoyennes[indiceArrayMoyenneDeMasse] - masseDeQualibrage;
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

// Fonction pour obtenir la moyenne de la tension
float getTensionMoyenne(){
  float averageTension = 0;
  int j = 0;
  do {
    averageTension += tensionsMoyennes[j];
    j++;
  } while (j < TAILLEARRAYTENSIONSMOYENNES);
  averageTension = averageTension/TAILLEARRAYTENSIONSMOYENNES;
  return averageTension;
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

// Fonction pour compter le nombre de pièces
String compterPieces(float massePesee) {
  String nombreDePieces;
  int wantedpos;
  for(int iter=0;iter<nombreDeTypeDePieces;iter++){
    if(piece == identificationPieces[iter]) {
     wantedpos = iter;
     break;
   }
  }
  float weigth = identificationMasses[wantedpos];
  float dist = INFINITY;
  for(int x = 1; x <= NOMBREDEPIECESTOTALPOSSIBLE; x++){
      if(abs(x*weigth - massePesee) < dist and (abs(x*weigth - massePesee)/(x*weigth))<0.03){
        dist = abs(x*weigth - massePesee);
        nombreDePieces = String(x) + " x " + identificationPieces[wantedpos];
      }
  }
  return nombreDePieces;
}

// Fonction pour changer la masse tare de la balance
float masseTare () {
  masseDeQualibrage = massesMoyennes[indiceArrayMoyenneDeMasse];
}

bool isBoutonSelectionne(int bouton) {
  return abs(buttonsState - bouton) < BTN_VALUE_MARGIN;
}

bool isStable(){
  int x = 0;
  do {
    if(!(tensionsMoyennes[x] - MARGE_STABILITE_POSITION <= TENSION_CONSIGNE && TENSION_CONSIGNE <= tensionsMoyennes[x] + MARGE_STABILITE_POSITION)){
      return false;
    }
    x++;
  } while (x < TAILLEARRAYTENSIONSMOYENNES);
  return true;
}

float getTensionCourantMoyen() {
  float courant = 0;
  for (int i = 0; i < NOMBRE_COURANTS_MOYENNE; i++) {
    courant += analogRead(CAPTEUR_COURANT_PIN);
  }
  courant /= NOMBRE_COURANTS_MOYENNE;

  // conversion en 0-5V
  courant *= (5.0/1024);
  return courant;
}

void lireEntrees(){ // Fonction pour lire les entrées
  buttonsState = analogRead(0);
  //Serial.println(buttonsState);
  if (buttonsState >= lastButtonState + 2 or buttonsState <= lastButtonState - 2) {
    if (mode == MODE_ETALONNAGE) {
      if (isBoutonSelectionne(BTN_LEFT)) {
        if (indexDeEtalonnage == 0) {
          indexDeEtalonnage++;
        } else if (indexDeEtalonnage == 1) {
          tension0g = getTensionCourantMoyen();
          indexDeEtalonnage++;
        } else if (indexDeEtalonnage == 2) {
          tension100g = getTensionCourantMoyen();
          penteDroiteEtalonnage = (masse100g - masse0g)/(tension100g - tension0g);
          bDroiteEtalonnage = masse100g - penteDroiteEtalonnage * tension100g;
          indexDeEtalonnage++;
        } else if (indexDeEtalonnage == 3) {
          mode = MODE_MASSE_TOTALE;
          indexDeEtalonnage++;
        }
      }
    }
    else {
    if (isBoutonSelectionne(BTN_DOWN)) { // Quand on clique sur le bouton down
      masseTare();
      mode = MODE_MASSE_TOTALE;
    }
    else if (isBoutonSelectionne(BTN_UP)){ // Quand on clique sur le bouton up
      mode = MODE_SELECTION;
      piece = choixTypeDePiece();
    }
    else if (isBoutonSelectionne(BTN_LEFT) and 3 < indexDeEtalonnage){ // Quand on clique sur le bouton up
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
  if (mode == MODE_SELECTION) {
    messageLigneDuHaut = "Authentification";
    messageLigneDuBas = piece;
  } else if (mode == MODE_MASSE_TOTALE) {
    messageLigneDuHaut = "Masse totale";
    messageLigneDuBas = uniteDeLaMasse(masseMoyenne);
  } else if (mode == MODE_COMPTAGE and piece != "") {
    messageLigneDuHaut = "Il y a";
    messageLigneDuBas = compterPieces(masseMoyenne);
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
  if(!isStable()){
    messageLigneDuBas = "Calcul en cours...";
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
  massesMoyennes[indiceArrayMoyenneDeMasse] = masse;
  indiceArrayMoyenneDeMasse++;
  indiceArrayMoyenneDeMasse %= TAILLEARRAYMASSESMOYENNES;
}

void setTension(float tension){
  tensionsMoyennes[indiceArrayMoyenneDeTension] = tension;
  indiceArrayMoyenneDeTension++;
  indiceArrayMoyenneDeTension %= TAILLEARRAYTENSIONSMOYENNES;
}

String choixTypeDePiece(){
  pieceChoisie = identificationPieces[indiceComptageDesPieces];
  indiceComptageDesPieces++;
  indiceComptageDesPieces %= nombreDeTypeDePieces;
  return pieceChoisie;
}

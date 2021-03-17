#include <LiquidCrystal.h>

// Fonctions des boutons
const int TAILLEARRAYMASSESMOYENNES = 10;
const int TAILLEARRAYTENSIONSMOYENNES = TAILLEARRAYMASSESMOYENNES;
const int TYPESDEPIECE = 10;
const int NOMBREDEPIECESTOTALPOSSIBLE = 3;
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
const int MODE_ETALONNAGE = 2;
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
  masseDeQualibrage = massesMoyennes[indiceArrayMoyenneDeMasse];
}

bool isBoutonSelectionne(int bouton) {
  return abs(buttonsState - bouton) < BTN_VALUE_MARGIN;
}

bool isStable(){
  float tensionMoyenne = getTensionMoyenne();
  float tensionActuelle = getTensionPosition();
  return (tensionMoyenne - 0.1 <= tensionActuelle && tensionActuelle <= tensionMoyenne + 0.1);
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
      masseTare();
      mode = MODE_MASSE_TOTALE;
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
    messageLigneDuBas = "Calcul...";
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

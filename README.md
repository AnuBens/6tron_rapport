# 6TRON - Station Météo Connectée
**Projet réalisé par : Benjamin LEJEUNE, Ossama ER RAKI, Tanguy GALLIFFET**

Ce projet implémente une station météo connectée basée sur un capteur **BME280**. Elle mesure la température, l'humidité et la pression atmosphérique, et publie ces données sur une interface **Adafruit IO** en utilisant le protocole **MQTT**. En cas de température élevée, une alerte est déclenchée et une LED clignote.

---

## Table des matières
1. [Fonctionnalités](#fonctionnalités)
2. [Matériel requis](#matériel-requis)
3. [Logiciels requis](#logiciels-requis)
4. [Installation](#installation)
    - [Configuration d'Adafruit IO](#configuration-dadafruit-io)
    - [Configuration du code](#configuration-du-code)
5. [Utilisation](#utilisation)
6. [Développement du code](#développement-du-code)
7. [Dépannage](#dépannage)
8. [Auteurs](#auteurs)

---

## Fonctionnalités

- **Mesure en temps réel** des paramètres météorologiques :
  - Température (°C)
  - Humidité (%)
  - Pression atmosphérique (hPa)
- Publication des données sur **Adafruit IO** pour une visualisation via des graphiques.
- **Détection de température élevée** avec :
  - Clignotement d'une LED.
  - Notification publiée sur le feed `alerte` d'Adafruit IO.
- Configuration facile via des constantes dans le code.

---

## Matériel requis

1. **Carte compatible Mbed OS** :
   - Exemple : STM32L4A6RG.
2. **Capteur BME280** pour mesurer les données environnementales.
3. **LED** connectée à la carte pour indiquer les alertes.
4. **Connexion Internet** pour publier les données sur Adafruit IO.

---

## Logiciels requis

- **Mbed OS** : Version 6.0 ou supérieure.
- **Compilateur GCC ARM**.
- **Adafruit IO** : Compte (gratuit ou premium).
- Interface de terminal série : PuTTY, Tera Term ou équivalent.

---

## Installation

### 1. Cloner le projet

```bash
git clone https://github.com/votre-repo/6tron-station-meteo.git
cd 6tron-station-meteo
```
### 2. Configuration d'Adafruit IO

1. **Créer un compte Adafruit IO**  
   Rendez-vous sur [Adafruit IO](https://io.adafruit.com) et créez un compte gratuit ou premium si ce n’est pas déjà fait.

2. **Créer les feeds nécessaires**  
   - Connectez-vous à votre compte Adafruit IO.  
   - Dans le menu principal, cliquez sur **Feeds** > **Actions** > **Create a New Feed**.  
   - Créez les feeds suivants :  
     - `temperature`  
     - `humidity`  
     - `pressure`  
     - `alerte` (utilisé pour les messages d'alerte).  

3. **Obtenir votre clé API**  
   - Rendez-vous dans les paramètres de votre compte en cliquant sur votre avatar en haut à droite, puis sur **My Account**.  
   - Copiez votre **clé API** dans la section **AIO Key**.  

---

### 3. Configuration du code

1. **Modifier les constantes MQTT**  
   - Dans le fichier `main.cpp`, remplacez les valeurs suivantes :
     - **Nom d'utilisateur Adafruit IO** :  
       ```cpp
       data.username.cstring = (char *)"VotreNomUtilisateur";
       ```
     - **Clé API Adafruit IO** :  
       ```cpp
       data.password.cstring = (char *)"VotreCleAPI";
       ```

2. **Définir le seuil de température pour les alertes**  
   Vous pouvez ajuster le seuil de température dans le fichier `main.cpp` :
   ```cpp
   constexpr float TEMP_THRESHOLD = 30.0; // Exemple : 30°C
   ```
   ---

### 4. Utilisation

1. **Démarrage**
   - Branchez votre carte compatible Mbed OS à l'alimentation ou à votre PC via USB.
   - Ouvrez un terminal série (ex. PuTTY ou Tera Term) avec les paramètres suivants :
     - **Baudrate** : 9600.
     - **Port COM** : Vérifiez le port correspondant à votre carte.

2. **Publication des données**
   - Les mesures de température, d'humidité, et de pression atmosphérique sont automatiquement publiées toutes les **10 secondes** sur Adafruit IO.
   - Vous pouvez visualiser les données en temps réel en ouvrant les **dashboards** Adafruit IO associés aux feeds.

3. **Détection d'alertes**
   - Si la température dépasse le seuil défini dans le code (`TEMP_THRESHOLD`), une alerte est déclenchée :
     - Une LED connectée à la carte clignote toutes les 500 ms.
     - Un message d'alerte est envoyé sur le feed `alerte` d'Adafruit IO.
   - Lorsque la température revient à la normale, la LED cesse de clignoter et un message est publié sur le terminal série.

---

### 5. Développement du code

1. **Structure principale**
   - **Initialisation des périphériques** :
     - Capteur BME280 pour les mesures météo.
     - LED utilisée pour les alertes.
   - **Configuration réseau** :
     - Connexion au réseau Wi-Fi.
     - Configuration MQTT avec Adafruit IO.
   - **Boucle principale** :
     - Mesure des données environnementales (température, humidité, pression).
     - Publication des données sur Adafruit IO.
     - Vérification des conditions d'alerte pour activer ou désactiver la LED.

2. **Personnalisation**
   - Modifiez les constantes suivantes dans `main.cpp` selon vos besoins :
     - **Fréquence de publication** :
       ```cpp
       publish_ticker.attach(main_queue.event(publish), 10); // Intervalle en secondes
       ```
     - **Seuil de température pour les alertes** :
       ```cpp
       constexpr float TEMP_THRESHOLD = 30.0; // Exemple : 30°C
       ```

3. **Évolution**
   - Ajouter d'autres capteurs pour des mesures complémentaires.
   - Envoyer des notifications par e-mail ou SMS en utilisant les actions Adafruit IO.
   - Intégrer une gestion d'énergie pour optimiser l'autonomie en cas d'alimentation sur batterie.

---

### 6. Dépannage

1. **Problèmes courants**
   - **Pas de connexion réseau** :
     - Vérifiez que votre routeur est fonctionnel.
     - Assurez-vous que la configuration réseau dans le code est correcte.
   - **Données non publiées sur Adafruit IO** :
     - Vérifiez les feeds Adafruit IO (`temperature`, `humidity`, `pressure`, `alerte`).
     - Vérifiez le nom d'utilisateur et la clé API dans le code.
   - **Pas de clignotement de la LED lors d'une alerte** :
     - Vérifiez que la température dépasse bien le seuil défini.
     - Assurez-vous que la LED est connectée correctement.

2. **Messages d'erreur sur le terminal série**
   - **Erreur de connexion MQTT** :
     - Vérifiez que le serveur Adafruit IO est accessible.
     - Vérifiez votre clé API et votre nom d'utilisateur.

3. **Problèmes avec le capteur BME280**
   - Assurez-vous que le capteur est bien connecté aux broches I2C.
   - Vérifiez la configuration du capteur dans le code.

---

### 7. Auteurs
- **Benjamin LEJEUNE**
- **Ossama ER RAKI**
- **Tanguy GALLIFFET**

---

### 8. Licence
Ce projet est sous licence MIT. Vous êtes libre de l'utiliser, de le modifier et de le distribuer tant que vous mentionnez les auteurs originaux.




# NMEAMultiplatformConverter

This project is an NMEA data converter designed to run on an ESP32 platform. The repository also contains a first working version of the system, suitable for testing and further development.

Ce projet est un convertisseur de donnees NMEA concu pour fonctionner sur une plateforme ESP32. Le depot contient egalement une premiere version operationnelle du systeme, adaptee aux tests et aux evolutions futures.

## Présentation / Presentation

### Français

Ce projet est un convertisseur de données NMEA destiné à être exécuté sur plateforme ESP32. Le dépôt contient une version initiale fonctionnelle du système, prête à être utilisée et à évoluer.

Le projet permet de traiter des données NMEA, de les transformer et de les transmettre selon un comportement adapté à une utilisation embarquée sur ESP32.

### English

This project is an NMEA data converter designed to run on an ESP32 platform. The repository contains an initial working version of the system, ready to be used and further developed.

The project can process NMEA data, transform it, and transmit it according to behavior suited to embedded use on ESP32.

## Fonctionnement principal / Main features

### Émulateur pour tests hors ligne / Offline emulator for testing

Le projet intègre un émulateur permettant de simuler les données de réception et de tester le comportement du système sans dépendre d’un environnement matériel ou réseau réel. Cet outil est particulièrement utile pour valider les traitements, les conversions et les règles d’envoi en mode hors ligne.

The project includes an emulator that simulates incoming data and allows testing of the system behavior without relying on a real hardware or network environment. This is especially useful for validating processing, conversions, and send rules in offline mode.

### Envoi après moyenne glissante / Sending after moving average

Une partie importante du fonctionnement consiste à agréger plusieurs mesures avant d’envoyer les données. Cette logique de moyenne glissante permet de lisser les variations, de réduire le bruit et de limiter les transmissions inutiles, ce qui est intéressant pour les systèmes embarqués et les communications réseau.

An important part of the behavior is to aggregate several measurements before sending the data. This moving-average logic smooths variations, reduces noise, and limits unnecessary transmissions, which is useful for embedded systems and network communications.

## Prérequis / Prerequisites

- ESP-IDF installé et configuré / ESP-IDF installed and configured
- Outils de compilation pour ESP32 / Build tools for ESP32
- Visual Studio Code avec l’extension ESP-IDF si vous utilisez cet environnement / Visual Studio Code with the ESP-IDF extension if you use this environment

## Structure du projet / Project structure

- `main/` : code source principal du projet / main application source code
- `CMakeLists.txt` : configuration CMake du projet / CMake project configuration
- `partitions.csv` : configuration des partitions flash / flash partition configuration
- `sdkconfig` : configuration ESP-IDF / ESP-IDF configuration

## Compilation / Build

Depuis la racine du projet :

```bash
idf.py build
```

From the project root:

```bash
idf.py build
```

## Flashage / Flashing

### Francais

Pour flasher le firmware sur la carte ESP32, utilisez la commande suivante en remplacant `<PORT>` par le port serie de votre carte.

### English

To flash the firmware to the ESP32 board, use the following command and replace `<PORT>` with your board serial port.

```bash
idf.py -p <PORT> flash
```

## Surveillance de la console série / Serial console monitoring

### Francais

Pour afficher les logs en temps reel sur le port serie, lancez la commande suivante apres le flash.

### English

To display real-time logs from the serial port, run the following command after flashing.

```bash
idf.py -p <PORT> monitor
```

## Développement / Development

Pour enregistrer une modification locale :

```bash
git add .
git commit -m "Votre message"
```

To record a local change:

```bash
git add .
git commit -m "Your message"
```

Pour envoyer les changements sur GitHub :

```bash
git push
```

To send changes to GitHub:

```bash
git push
```

## Notes / Notes

Ce README peut être complété plus tard avec des détails techniques, des instructions d’utilisation et des captures d’écran.

This README can be expanded later with technical details, usage instructions, and screenshots.

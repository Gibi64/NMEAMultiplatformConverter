# NMEAMultiplatformConverter

Ce projet est un convertisseur de données NMEA destiné à être exécuté sur plateforme ESP32.

## Présentation

Ce dépôt contient la version initiale du projet, prête à être utilisée et à évoluer.

## Prérequis

- ESP-IDF installé et configuré
- Outils de compilation pour ESP32
- Visual Studio Code avec l’extension ESP-IDF si vous utilisez cet environnement

## Structure du projet

- `main/` : code source principal du projet
- `CMakeLists.txt` : configuration CMake du projet
- `partitions.csv` : configuration des partitions flash
- `sdkconfig` : configuration ESP-IDF

## Compilation

Depuis la racine du projet :

```bash
idf.py build
```

## Flashage

```bash
idf.py -p <PORT> flash
```

## Surveillance de la console série

```bash
idf.py -p <PORT> monitor
```

## Développement

Pour enregistrer une modification locale :

```bash
git add .
git commit -m "Votre message"
```

Pour envoyer les changements sur GitHub :

```bash
git push
```

## Notes

Ce README peut être complété plus tard avec des détails techniques, des instructions d’utilisation et des captures d’écran.

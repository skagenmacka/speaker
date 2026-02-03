# Högtalarsystem

Detta projekt är omfattar ett högtalarsystem som består av följande delar:
- __apps/controlller-ui:__ En webbapp som är byggd på React, Vite & Tailwind. Med hjälp av appen kan man kontrollera olika parametrar som påverkar ljudet som kommer ifrån ljudsystemet.
- __apps/speaker:__ Det centrala högtalarsystemet kodat i C++. Använder delar som ligger i /libs. Systemet använder `librespot` för att kontrollera musik via Spotify.

## Bygga projektet:
För att bygga högtalarsystemet så körs följande kommandon från projektets rot:
```bash 
make configure
make build
```
För att bygga kontroll UI:t så körs följande kommandon från projektets rot:

```bash
cd /apps/controller-ui
npm i
npm run build
```

## Körning
För att aktivera Spotify-spot:en (`librespot`) körs kommandot:
```bash
librespot --name "Speaker Dev (Mac)" --backend pipe --bitrate 160
```

Köra båda kommandon samtidigt för att mata in ljud-strömmen till programmet:
```bash
librespot --name "Speaker Dev (Mac)" --backend pipe --bitrate 160 \
  | ./build/apps/speaker/speaker 0
```
eller 
```bash
make run
``` 

För att köra kontroll UI:t körs förljande kommando:
```bash
npm run start
```
eller för utveckling
```bash
npm run start
```
obs kan även kräva följande kommando innan (installerar node dependencies)
```bash
npm i
```
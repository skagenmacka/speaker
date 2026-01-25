# Högtalarsystem
Bygga projektet:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```
eller 
```bash
make configure
make build
```

För att aktivera Spotify-spot:en körs kommandot:
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
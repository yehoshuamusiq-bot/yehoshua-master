# YEHOSHU'A MUSIQ — MASTER

Plugin de mastering VST3 (Windows / FL Studio) aux couleurs de Yehoshu'a Musiq.

**Chaîne de traitement :** Input → EQ 5 bandes + Low Cut → Compresseur multibande 3 bandes → Exciter (Sub + Air) → Image stéréo (Width + Mono Bass) → *[suréchantillonnage 4x]* Saturation analogique (Tape/Tube) → Soft Clipper → Limiteur look-ahead brickwall → *[retour 1x]* Output → Dither.

Mesures : crêtes entrée/sortie, réduction de gain (comp / clip / limit), LUFS Momentary / Short-term / Integrated (norme EBU R128), corrélation de phase, analyseur de spectre avec courbe d'EQ. 12 presets d'usine, comparaison A/B, interface redimensionnable.

Le guide complet est dans **Guide_YEHOSHUA_MASTER.pdf**.

## Obtenir le fichier .vst3

### Option A — Sans rien installer (GitHub)
1. Crée un compte sur github.com et un nouveau dépôt (repository).
2. Téléverse tout le contenu de ce dossier (y compris le dossier caché `.github`).
3. Onglet **Actions** → le build « Build Windows VST3 » démarre tout seul (~10 min).
4. Ouvre le build terminé → section **Artifacts** → télécharge **YEHOSHUA-MASTER-Windows**.

### Option B — Sur ton PC Windows
1. Installe **Visual Studio 2022 Community** (coche « Développement Desktop en C++ »), **CMake** et **Git**.
2. Double-clique sur **build_windows.bat**.
3. Le plugin apparaît dans le dossier `dist`.

## Installer dans FL Studio
1. Copie `YEHOSHUA MASTER.vst3` dans `C:\Program Files\Common Files\VST3`.
2. FL Studio → *Options* → *Manage plugins* → *Find installed plugins*.
3. Mixer → piste **Master** → dernier slot → *YEHOSHUA MASTER*.

## Développeurs
- `Source/dsp/MasterDSP.h` : tout le DSP (C++ pur, testable).
- `tests/dsp_test.cpp` : tests unitaires (`g++ -O2 -std=c++17 -ISource tests/dsp_test.cpp`).
- `cmake -B build -DYM_BUILD_TESTS=ON` : banc d'essai qui traite un beat avec chaque preset et capture l'interface.

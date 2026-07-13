#!/bin/bash
# Empaqueta LaDistorneta (VST3 + AU) en un instalador .pkg para macOS.
# Uso: ./make_pkg.sh          (recompila y empaqueta)
#      ./make_pkg.sh --no-build  (empaqueta lo ya compilado)
set -euo pipefail
cd "$(dirname "$0")"

VERSION="0.0.1"
ARTEFACTS="build/LaDistorneta_artefacts"
STAGE="build/pkg_stage"
OUT="LaDistorneta-${VERSION}.pkg"

# 1. Compilar
if [[ "${1:-}" != "--no-build" ]]; then
    cmake --build build
fi

# 2. Staging: replica la estructura de /Library/Audio/Plug-Ins
rm -rf "$STAGE"
mkdir -p "$STAGE/Library/Audio/Plug-Ins/VST3" "$STAGE/Library/Audio/Plug-Ins/Components"
cp -R "$ARTEFACTS/VST3/LaDistorneta.vst3"    "$STAGE/Library/Audio/Plug-Ins/VST3/"
cp -R "$ARTEFACTS/AU/LaDistorneta.component" "$STAGE/Library/Audio/Plug-Ins/Components/"

# 3. Firma ad-hoc (necesaria en Apple Silicon; sin esto el DAW no carga el binario)
codesign --force --deep -s - "$STAGE/Library/Audio/Plug-Ins/VST3/LaDistorneta.vst3"
codesign --force --deep -s - "$STAGE/Library/Audio/Plug-Ins/Components/LaDistorneta.component"

# 4. Construir el pkg (instala en /Library/Audio/Plug-Ins, para todos los usuarios)
pkgbuild --root "$STAGE" \
         --identifier com.ladistorneta.plugins \
         --version "$VERSION" \
         --install-location / \
         "$OUT"

echo ""
echo "Listo: $(pwd)/$OUT"

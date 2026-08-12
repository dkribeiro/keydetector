#!/usr/bin/env bash
set -euo pipefail

# Builds a macOS installer that drops the AU + VST3 into the standard
# system plugin folders. Run from the repo root after a Release build.
#
# Optional signing/notarization env vars (leave unset for an unsigned pkg):
#   INSTALLER_SIGN_ID  e.g. "Developer ID Installer: Your Name (TEAMID)"

ART="build/KeyDetector_artefacts/Release"
STAGE="$(mktemp -d)"
VERSION="${1:-0.1.0}"

mkdir -p "$STAGE/Library/Audio/Plug-Ins/Components"
mkdir -p "$STAGE/Library/Audio/Plug-Ins/VST3"

cp -R "$ART/AU/KeyDetector.component"  "$STAGE/Library/Audio/Plug-Ins/Components/"
cp -R "$ART/VST3/KeyDetector.vst3"     "$STAGE/Library/Audio/Plug-Ins/VST3/"

pkgbuild \
  --root "$STAGE" \
  --identifier "com.keydetector.keydetector" \
  --version "$VERSION" \
  --install-location "/" \
  "KeyDetector-macOS.pkg"

if [[ -n "${INSTALLER_SIGN_ID:-}" ]]; then
  productsign --sign "$INSTALLER_SIGN_ID" \
    "KeyDetector-macOS.pkg" "KeyDetector-macOS-signed.pkg"
  mv "KeyDetector-macOS-signed.pkg" "KeyDetector-macOS.pkg"
  echo "Signed. Notarize with: xcrun notarytool submit KeyDetector-macOS.pkg --keychain-profile <profile> --wait"
fi

rm -rf "$STAGE"
echo "Built KeyDetector-macOS.pkg (version $VERSION)"

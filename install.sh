#!/usr/bin/env bash
# QApp Framework — Developer Preview Installer (alpha)
# Builds and installs QApp from source. Pure C++/Qt6 — no Node.js required.
#
# Usage:
#   curl -fsSL https://raw.githubusercontent.com/QAppFramework/QApp/main/install.sh | bash
#   — or —
#   git clone https://github.com/QAppFramework/QApp.git && cd QApp && bash install.sh
#
# Licensed under EUPL v1.2

set -euo pipefail

REPO_URL="https://github.com/QAppFramework/QApp.git"
INSTALL_DIR="$HOME/.local/share/qapp-framework"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()  { echo -e "${GREEN}[QApp]${NC} $1"; }
warn()  { echo -e "${YELLOW}[QApp]${NC} $1"; }
error() { echo -e "${RED}[QApp]${NC} $1" >&2; }
die()   { error "$1"; exit 1; }

# ── Check prerequisites ──────────────────────────────────────

check_command() {
    command -v "$1" >/dev/null 2>&1 || die "Required: $1 — $2"
}

info "Checking prerequisites..."

check_command cmake   "Install: sudo apt install cmake"
check_command g++     "Install: sudo apt install g++"
check_command git     "Install: sudo apt install git"

# Check Qt6 dev packages via pkg-config
pkg-config --exists Qt6Core Qt6Quick Qt6WebEngineQuick 2>/dev/null || {
    error "Qt6 development packages not found."
    echo ""
    echo "  Ubuntu/Debian:"
    echo "    sudo apt install qt6-base-dev qt6-declarative-dev qt6-webengine-dev"
    echo "    sudo apt install qml6-module-qtquick-controls qml6-module-qtwebengine"
    echo "    sudo apt install qml6-module-qtquick-layouts qml6-module-qtcore"
    echo ""
    echo "  Fedora:"
    echo "    sudo dnf install qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtwebengine-devel"
    echo ""
    die "Install Qt6 packages and try again."
}

info "All prerequisites met."

# ── Clone or use local repo ───────────────────────────────────

SOURCE_DIR=""

if [ -f "CMakeLists.txt" ] && grep -q "qapp-framework" CMakeLists.txt 2>/dev/null; then
    info "Running from QApp source directory."
    SOURCE_DIR="$(pwd)"
else
    info "Cloning QApp from GitHub..."
    TMPDIR=$(mktemp -d)
    trap 'rm -rf "$TMPDIR"' EXIT
    git clone --depth 1 "$REPO_URL" "$TMPDIR/qapp"
    SOURCE_DIR="$TMPDIR/qapp"
fi

cd "$SOURCE_DIR"

# ── Build with CMake ──────────────────────────────────────────

info "Building QApp (cmake)..."
cmake -B build -DCMAKE_BUILD_TYPE=Release -Wno-dev 2>/dev/null
cmake --build build --parallel

[ -f "build/qapp-installer" ]   || die "Build failed: qapp-installer binary not found"
[ -f "build/qapp-ws-wrapper" ]  || die "Build failed: qapp-ws-wrapper binary not found"
[ -f "build/qapp-pwa-app" ]     || die "Build failed: qapp-pwa-app binary not found"

info "Build successful — 3 binaries."

# ── Install to ~/.local/share/qapp-framework/ ────────────────

info "Installing to $INSTALL_DIR ..."

mkdir -p "$INSTALL_DIR/app"

# Binaries
cp build/qapp-installer  "$INSTALL_DIR/app/"
cp build/qapp-ws-wrapper "$INSTALL_DIR/app/"
cp build/qapp-pwa-app    "$INSTALL_DIR/app/"

# Self-updater
cp bin/update.sh "$INSTALL_DIR/app/" 2>/dev/null || true

# ── Migrate legacy installs (~/.local/share/qapp/ → qapp-framework/) ──

OLD_BASE="$HOME/.local/share/qapp"
NEW_WRAPPER="$INSTALL_DIR/app/qapp-ws-wrapper"
APPS_DIR="$HOME/.local/share/applications"

# Remove old "qapp" desktop entry (pre-rename launcher)
if [ -f "$APPS_DIR/qapp.desktop" ]; then
    rm -f "$APPS_DIR/qapp.desktop"
    info "Removed legacy qapp.desktop entry."
fi

# Migrate app metadata and icons from old dir
if [ -d "$OLD_BASE/apps" ]; then
    mkdir -p "$INSTALL_DIR/apps" "$INSTALL_DIR/icons"

    # Copy metadata + icons (don't overwrite if already migrated)
    cp -rn "$OLD_BASE/apps/"*.json "$INSTALL_DIR/apps/" 2>/dev/null || true
    cp -rn "$OLD_BASE/icons/"* "$INSTALL_DIR/icons/" 2>/dev/null || true

    # Fix paths in metadata JSON (old paths → new paths, old binary → new binary)
    for jsonfile in "$INSTALL_DIR/apps/"*.json; do
        [ -f "$jsonfile" ] || continue
        sed -i "s|$OLD_BASE/icons/|$INSTALL_DIR/icons/|g" "$jsonfile"
        sed -i "s|$OLD_BASE/app/qapp-wrapper|$NEW_WRAPPER|g" "$jsonfile"
    done

    # Fix .desktop entries for installed apps
    for desktop in "$APPS_DIR"/qapp-*.desktop; do
        [ -f "$desktop" ] || continue
        [ "$(basename "$desktop")" = "qapp-installer.desktop" ] && continue
        if grep -q "$OLD_BASE/app/qapp-wrapper" "$desktop" 2>/dev/null; then
            sed -i "s|$OLD_BASE/app/qapp-wrapper|$NEW_WRAPPER|g" "$desktop"
            sed -i "s|$OLD_BASE/icons/|$INSTALL_DIR/icons/|g" "$desktop"
        fi
    done

    info "Migrated apps from legacy ~/.local/share/qapp/ directory."
fi

# Remove old binaries (not app data — keep icons/apps in case of rollback)
if [ -d "$OLD_BASE/app" ]; then
    rm -rf "$OLD_BASE/app"
fi

# ── Create .desktop entry ─────────────────────────────────────

info "Creating desktop launcher..."

mkdir -p "$HOME/.local/share/applications"

cat > "$HOME/.local/share/applications/qapp-installer.desktop" << DESKTOP
[Desktop Entry]
Type=Application
Name=QApp
Comment=Install websites as standalone desktop apps
Exec=$INSTALL_DIR/app/qapp-installer
Icon=applications-internet
Terminal=false
StartupNotify=true
Categories=Network;WebBrowser;Utility;
DESKTOP

update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true

# ── Summary ───────────────────────────────────────────────────

echo ""
info "============================================"
info "  QApp installed successfully! (alpha)"
info "============================================"
info ""
info "  Launch from KDE/GNOME menu: search 'QApp'"
info ""
info "  Or from terminal:"
info "    $INSTALL_DIR/app/qapp-installer"
info ""
info "  CLI commands:"
info "    qapp-installer --classify <url>"
info "    qapp-installer --install <url>"
info "    qapp-installer --list"
info "    qapp-installer --uninstall <app-id>"
info ""

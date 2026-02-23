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

# ── Clean up legacy installs ──────────────────────────────────

# Remove old "qapp" desktop entry (pre-rename, pointed to ~/.local/share/qapp/)
if [ -f "$HOME/.local/share/applications/qapp.desktop" ]; then
    rm -f "$HOME/.local/share/applications/qapp.desktop"
    info "Removed legacy qapp.desktop entry."
fi

# Remove old install dir (pre-rename)
if [ -d "$HOME/.local/share/qapp/app" ]; then
    rm -rf "$HOME/.local/share/qapp/app"
    info "Removed legacy ~/.local/share/qapp/app/ directory."
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

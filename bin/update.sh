#!/usr/bin/env bash
# QApp self-updater — downloads latest DEB from GitHub Release and installs.
# Designed to be run by the QApp installer app (via QProcess).
#
# Usage: bash update.sh [current-version]
#   current-version: e.g. "0.1.0" (passed by app-manager.cpp QAPP_VERSION)
#
# Exit codes:
#   0 = success (updated or already up to date)
#   1 = prerequisite missing
#   2 = download failed
#   3 = install failed

set -euo pipefail

REPO="QAppFramework/QApp"
API_URL="https://api.github.com/repos/$REPO/releases/latest"
CURRENT_VERSION="${1:-}"

info()  { echo "[QApp] $1"; }
error() { echo "[QApp] ERROR: $1" >&2; }

# ── Check prerequisites ──────────────────────────────────────
command -v curl >/dev/null 2>&1 || { error "Required: curl"; exit 1; }
command -v pkexec >/dev/null 2>&1 || { error "Required: pkexec (polkit)"; exit 1; }

# ── Get latest release info ──────────────────────────────────
info "Checking for latest version..."
RELEASE_JSON=$(curl -sL "$API_URL") || { error "Cannot reach GitHub"; exit 2; }

# Extract version from tag (strip leading "v")
TAG=$(echo "$RELEASE_JSON" | grep -o '"tag_name": "[^"]*"' | head -1 | cut -d'"' -f4)
LATEST_VERSION="${TAG#v}"

[ -n "$LATEST_VERSION" ] || { error "Cannot determine latest version"; exit 2; }

# ── Version comparison ───────────────────────────────────────
if [ -n "$CURRENT_VERSION" ]; then
    # sort -V sorts version strings correctly (0.1.0 < 0.2.0 < 1.0.0)
    OLDEST=$(printf '%s\n' "$CURRENT_VERSION" "$LATEST_VERSION" | sort -V | head -1)
    if [ "$CURRENT_VERSION" = "$LATEST_VERSION" ]; then
        info "Already up to date ($CURRENT_VERSION)."
        exit 0
    elif [ "$OLDEST" != "$CURRENT_VERSION" ]; then
        info "Current version ($CURRENT_VERSION) is newer than release ($LATEST_VERSION). Skipping."
        exit 0
    fi
    info "Update available: $CURRENT_VERSION → $LATEST_VERSION"
else
    info "Found version $TAG"
fi

# ── Download .deb ────────────────────────────────────────────
DEB_URL=$(echo "$RELEASE_JSON" | grep -o '"browser_download_url": "[^"]*\.deb"' | head -1 | cut -d'"' -f4)
[ -n "$DEB_URL" ] || { error "No .deb package found in latest release"; exit 2; }

TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

info "Downloading $TAG..."
curl -sL "$DEB_URL" -o "$TMPDIR/qapp-update.deb" || { error "Download failed"; exit 2; }

# ── Install with elevated privileges ─────────────────────────
info "Installing update (password required)..."
if ! pkexec dpkg -i "$TMPDIR/qapp-update.deb"; then
    error "Installation cancelled or failed"
    exit 3
fi

info "Updated to $TAG — restart QApp to use the new version."

# QApp — Turn Any Website Into a Desktop App

**Developer Preview (Alpha)**

QApp wraps any website into a standalone desktop application on Linux (KDE/GNOME).
Each installed site gets its own launcher entry, persistent login sessions, and
remembered window size/position — no browser chrome, no tabs, just the site.

## Features

- **One-click install** — Enter a URL, click Install, done
- **System launcher integration** — Apps appear in KDE/GNOME application menu
- **Persistent sessions** — Login state survives app restarts
- **Per-app window memory** — Each app remembers its size and position
- **Clean uninstall** — Remove apps cleanly via CLI or UI

## Install from Source

### Prerequisites

- Node.js 20+
- CMake + g++
- Qt6 dev packages (Core, Quick, WebEngine)

### Ubuntu/Debian

```bash
# Qt6 dependencies
sudo apt install qt6-base-dev qt6-declarative-dev qt6-webengine-dev
sudo apt install qml6-module-qtquick-controls qml6-module-qtwebengine
sudo apt install qml6-module-qtquick-layouts qml6-module-qtcore

# Build and install
git clone https://github.com/QAppFramework/QApp.git
cd QApp
./install.sh
```

### One-liner

```bash
curl -fsSL https://raw.githubusercontent.com/QAppFramework/QApp/main/install.sh | bash
```

## Usage

### GUI

Launch QApp from your system menu, enter a URL, classify it, and click Install.

### CLI

```bash
# Install a website as an app
node ~/.local/share/qapp/bin/install.js https://github.com

# List installed apps
node ~/.local/share/qapp/bin/list.js

# Uninstall an app
node ~/.local/share/qapp/bin/uninstall.js github-com
```

## How It Works

1. User provides a URL
2. QApp classifies the site (Website / Web App / PWA)
3. Downloads the site's favicon/icon
4. Generates a `.desktop` launcher entry
5. Creates a standalone Qt WebEngine window with:
   - Persistent cookies (login state)
   - Isolated storage per app
   - Window geometry persistence

## Tech Stack

- **Qt6/QML** — Native UI and WebEngine rendering
- **Node.js** — URL classification and install pipeline
- **C++** — QProcess bridge between QML and Node.js CLI
- **XDG** — Freedesktop integration (.desktop files, standard paths)

## License

[European Union Public Licence v1.2](LICENSE) (EUPL-1.2)

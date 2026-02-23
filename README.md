# QApp

Turn any website into a standalone desktop app.

## Quick Install (one command)

```bash
curl -fsSL https://raw.githubusercontent.com/QAppFramework/QApp/main/install.sh | bash
```

Or clone and install:

```bash
git clone https://github.com/QAppFramework/QApp.git && cd QApp && bash install.sh
```

## Requirements

- Linux (KDE/GNOME)
- Qt 6.5+ development packages
- cmake 3.22+, g++ (C++20)

**Ubuntu/Debian:**
```bash
sudo apt install cmake g++ qt6-base-dev qt6-declarative-dev qt6-webengine-dev \
  qml6-module-qtquick-controls qml6-module-qtwebengine \
  qml6-module-qtquick-layouts qml6-module-qtcore
```

## Usage

Launch from your app menu (search "QApp") or from terminal:

```bash
# GUI
~/.local/share/qapp-framework/app/qapp-installer

# CLI
qapp-installer --classify <url>
qapp-installer --install <url>
qapp-installer --list
qapp-installer --uninstall <app-id>
```

## License

EUPL v1.2 — see [LICENSE](LICENSE)

This application uses the [Qt framework](https://www.qt.io) under [LGPL v3.0](https://www.gnu.org/licenses/lgpl-3.0.html).
See [THIRD-PARTY-LICENSES](THIRD-PARTY-LICENSES) for details.

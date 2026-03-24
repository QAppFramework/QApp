# QApp

Turn any website into a standalone desktop app. Replace Chrome's fragile desktop
shortcuts with truly standalone apps that survive browser removal.

## Install

Download the latest `.deb` from [GitHub Releases](https://github.com/QAppFramework/QApp/releases) and install:

```bash
sudo dpkg -i qapp-framework-*.deb
```

### Build from source (developers)

```bash
git clone https://github.com/QAppFramework/QApp.git
cd QApp
cmake -B build && cmake --build build
sudo cmake --install build
```

Requires: cmake, Rust (stable), Qt 6.5+ (qt6-base-dev, qt6-declarative-dev, qt6-webengine-dev)

## Three Products

- **QApp** — Manager. Installs and administers web apps. Can be removed without breaking installed apps.
- **QAppWS** — Website wrapper. Any website in its own window with tabs and bookmarks.
- **QAppPWA** — Web App shell. Manifest-driven apps with offline, push, badges, and permissions.

## License

EUPL v1.2

<!-- MIB-NOTICE -->

> **Note:** This project is as-is — it is an artefact of a MIB process. See [mib.lpmwfx.com](https://mib.lpmwfx.com/) for details. It is only an MVP, not a full release. Feel free to use it for your own projects as you like.

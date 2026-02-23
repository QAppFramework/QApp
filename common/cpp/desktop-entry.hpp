#ifndef QAPP_DESKTOP_ENTRY_HPP
#define QAPP_DESKTOP_ENTRY_HPP

#include "result.hpp"
#include <QString>

namespace qapp {

/// Input for .desktop file generation.
struct DesktopEntryInput {
    QString name;
    QString exec;
    QString icon;
    QString comment;     // optional
    QString categories;  // optional, defaults to "Network;WebBrowser;"
};

/// Freedesktop .desktop entry file generation.
/// Ports js/src/installer/system/desktop-entry.js
class DesktopEntry {
public:
    static Result<QString> generate(const DesktopEntryInput &input);
};

} // namespace qapp

#endif // QAPP_DESKTOP_ENTRY_HPP

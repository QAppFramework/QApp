#include "desktop-entry.hpp"
#include <QStringList>

namespace qapp {

Result<QString> DesktopEntry::generate(const DesktopEntryInput &input)
{
    if (input.name.isEmpty()) {
        return Result<QString>::fail(QStringLiteral("Name is required"));
    }
    if (input.exec.isEmpty()) {
        return Result<QString>::fail(QStringLiteral("Exec is required"));
    }
    if (input.icon.isEmpty()) {
        return Result<QString>::fail(QStringLiteral("Icon is required"));
    }

    QStringList lines;
    lines << QStringLiteral("[Desktop Entry]");
    lines << QStringLiteral("Type=Application");
    lines << QStringLiteral("Name=") + input.name;
    lines << QStringLiteral("Exec=") + input.exec;
    lines << QStringLiteral("Icon=") + input.icon;
    lines << QStringLiteral("Terminal=false");
    lines << QStringLiteral("StartupNotify=true");

    if (!input.comment.isEmpty()) {
        lines << QStringLiteral("Comment=") + input.comment;
    }

    QString categories = input.categories.isEmpty()
        ? QStringLiteral("Network;WebBrowser;")
        : input.categories;
    lines << QStringLiteral("Categories=") + categories;

    // Trailing newline per freedesktop spec
    return Result<QString>::ok(lines.join(QLatin1Char('\n')) + QLatin1Char('\n'));
}

} // namespace qapp

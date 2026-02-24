#include <QTest>
#include "desktop-entry.hpp"

class TestDesktopEntry : public QObject
{
    Q_OBJECT

private slots:
    void generateBasicEntry()
    {
        qapp::DesktopEntryInput input;
        input.name = QStringLiteral("GitHub");
        input.exec = QStringLiteral("/usr/bin/qapp-ws-wrapper github-com https://github.com");
        input.icon = QStringLiteral("/home/user/.local/share/qapp-framework/icons/github-com.png");

        auto result = qapp::DesktopEntry::generate(input);
        QVERIFY(result.success);

        QVERIFY(result.data.startsWith(QStringLiteral("[Desktop Entry]\n")));
        QVERIFY(result.data.contains(QStringLiteral("Type=Application\n")));
        QVERIFY(result.data.contains(QStringLiteral("Name=GitHub\n")));
        QVERIFY(result.data.contains(QStringLiteral("Exec=/usr/bin/qapp-ws-wrapper github-com https://github.com\n")));
        QVERIFY(result.data.contains(QStringLiteral("Terminal=false\n")));
        QVERIFY(result.data.contains(QStringLiteral("StartupNotify=true\n")));
        // Default categories
        QVERIFY(result.data.contains(QStringLiteral("Categories=Network;WebBrowser;\n")));
        // Trailing newline
        QVERIFY(result.data.endsWith(QLatin1Char('\n')));
    }

    void generateWithComment()
    {
        qapp::DesktopEntryInput input;
        input.name = QStringLiteral("GitHub");
        input.exec = QStringLiteral("/usr/bin/qapp-ws-wrapper github-com");
        input.icon = QStringLiteral("/path/to/icon.png");
        input.comment = QStringLiteral("GitHub - Where the world builds software");

        auto result = qapp::DesktopEntry::generate(input);
        QVERIFY(result.success);
        QVERIFY(result.data.contains(QStringLiteral("Comment=GitHub - Where the world builds software\n")));
    }

    void generateWithCustomCategories()
    {
        qapp::DesktopEntryInput input;
        input.name = QStringLiteral("Slack");
        input.exec = QStringLiteral("/usr/bin/qapp-ws-wrapper slack-com");
        input.icon = QStringLiteral("/path/to/icon.png");
        input.categories = QStringLiteral("Network;InstantMessaging;");

        auto result = qapp::DesktopEntry::generate(input);
        QVERIFY(result.success);
        QVERIFY(result.data.contains(QStringLiteral("Categories=Network;InstantMessaging;\n")));
        QVERIFY(!result.data.contains(QStringLiteral("Categories=Network;WebBrowser;")));
    }

    void generateWithSpecialCharsInName()
    {
        qapp::DesktopEntryInput input;
        input.name = QStringLiteral("Ben & Jerry's");
        input.exec = QStringLiteral("/usr/bin/qapp-ws-wrapper test");
        input.icon = QStringLiteral("/path/to/icon.png");

        auto result = qapp::DesktopEntry::generate(input);
        QVERIFY(result.success);
        QVERIFY(result.data.contains(QStringLiteral("Name=Ben & Jerry's\n")));
    }

    void missingNameFails()
    {
        qapp::DesktopEntryInput input;
        input.exec = QStringLiteral("/usr/bin/test");
        input.icon = QStringLiteral("/path/to/icon.png");

        auto result = qapp::DesktopEntry::generate(input);
        QVERIFY(!result.success);
        QCOMPARE(result.error, QStringLiteral("Name is required"));
    }

    void missingExecFails()
    {
        qapp::DesktopEntryInput input;
        input.name = QStringLiteral("Test");
        input.icon = QStringLiteral("/path/to/icon.png");

        auto result = qapp::DesktopEntry::generate(input);
        QVERIFY(!result.success);
        QCOMPARE(result.error, QStringLiteral("Exec is required"));
    }

    void missingIconFails()
    {
        qapp::DesktopEntryInput input;
        input.name = QStringLiteral("Test");
        input.exec = QStringLiteral("/usr/bin/test");

        auto result = qapp::DesktopEntry::generate(input);
        QVERIFY(!result.success);
        QCOMPARE(result.error, QStringLiteral("Icon is required"));
    }

    void generateWithActions()
    {
        qapp::DesktopEntryInput input;
        input.name = QStringLiteral("Social App");
        input.exec = QStringLiteral("/usr/bin/qapp-pwa-app social-com https://social.com");
        input.icon = QStringLiteral("/path/to/icon.png");
        input.actions = {
            {"NewPost", "New Post", "/usr/bin/qapp-pwa-app social-com https://social.com/compose", ""},
            {"Settings", "Settings", "/usr/bin/qapp-pwa-app social-com https://social.com/settings", "/path/icon2.png"},
        };

        auto result = qapp::DesktopEntry::generate(input);
        QVERIFY(result.success);
        QVERIFY(result.data.contains(QStringLiteral("Actions=NewPost;Settings;\n")));
        QVERIFY(result.data.contains(QStringLiteral("[Desktop Action NewPost]\n")));
        QVERIFY(result.data.contains(QStringLiteral("Name=New Post\n")));
        QVERIFY(result.data.contains(QStringLiteral("[Desktop Action Settings]\n")));
        QVERIFY(result.data.contains(QStringLiteral("Icon=/path/icon2.png\n")));
    }

    void generateWithMimeType()
    {
        qapp::DesktopEntryInput input;
        input.name = QStringLiteral("Editor");
        input.exec = QStringLiteral("/usr/bin/qapp-pwa-app editor");
        input.icon = QStringLiteral("/path/to/icon.png");
        input.mimeTypes = QStringLiteral("text/plain;text/html;");

        auto result = qapp::DesktopEntry::generate(input);
        QVERIFY(result.success);
        QVERIFY(result.data.contains(QStringLiteral("MimeType=text/plain;text/html;\n")));
    }

    void fieldOrderIsCorrect()
    {
        qapp::DesktopEntryInput input;
        input.name = QStringLiteral("Test");
        input.exec = QStringLiteral("/usr/bin/test");
        input.icon = QStringLiteral("/path/to/icon.png");

        auto result = qapp::DesktopEntry::generate(input);
        QVERIFY(result.success);

        // Verify field order: header, Type, Name, Exec, Icon, Terminal, StartupNotify, Categories
        int headerPos = result.data.indexOf(QStringLiteral("[Desktop Entry]"));
        int typePos = result.data.indexOf(QStringLiteral("Type="));
        int namePos = result.data.indexOf(QStringLiteral("Name="));
        int execPos = result.data.indexOf(QStringLiteral("Exec="));
        int iconPos = result.data.indexOf(QStringLiteral("Icon="));
        int termPos = result.data.indexOf(QStringLiteral("Terminal="));
        int notifyPos = result.data.indexOf(QStringLiteral("StartupNotify="));
        int catPos = result.data.indexOf(QStringLiteral("Categories="));

        QVERIFY(headerPos < typePos);
        QVERIFY(typePos < namePos);
        QVERIFY(namePos < execPos);
        QVERIFY(execPos < iconPos);
        QVERIFY(iconPos < termPos);
        QVERIFY(termPos < notifyPos);
        QVERIFY(notifyPos < catPos);
    }
};

QTEST_MAIN(TestDesktopEntry)
#include "test-desktop-entry.moc"

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

namespace qapp::pwa {

/// Config-driven helper for PWA apps.
/// Reads per-app metadata from config.json via common lib.
class PwaHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool metadataLoaded READ metadataLoaded NOTIFY metadataChanged)
    Q_PROPERTY(QString displayMode READ displayMode NOTIFY metadataChanged)
    Q_PROPERTY(QString themeColor READ themeColor NOTIFY metadataChanged)
    Q_PROPERTY(QString backgroundColor READ backgroundColor NOTIFY metadataChanged)
    Q_PROPERTY(QString iconPath READ iconPath NOTIFY metadataChanged)
    Q_PROPERTY(QString startUrl READ startUrl NOTIFY metadataChanged)
    Q_PROPERTY(QString scope READ scope NOTIFY metadataChanged)
    Q_PROPERTY(QString appName READ appName NOTIFY metadataChanged)
    Q_PROPERTY(bool isOnline READ isOnline NOTIFY onlineChanged)

public:
    explicit PwaHelper(QObject *parent = nullptr);

    bool metadataLoaded() const { return m_metadataLoaded; }
    QString displayMode() const { return m_displayMode; }
    QString themeColor() const { return m_themeColor; }
    QString backgroundColor() const { return m_backgroundColor; }
    QString iconPath() const { return m_iconPath; }
    QString startUrl() const { return m_startUrl; }
    QString scope() const { return m_scope; }
    QString appName() const { return m_appName; }
    bool isOnline() const { return m_isOnline; }

    Q_INVOKABLE void loadMetadata(const QString &appId);

signals:
    void metadataChanged();
    void onlineChanged();

private:
    bool m_metadataLoaded = false;
    QString m_displayMode;
    QString m_themeColor;
    QString m_backgroundColor;
    QString m_iconPath;
    QString m_startUrl;
    QString m_scope;
    QString m_appName;
    bool m_isOnline = true;
};

} // namespace qapp::pwa

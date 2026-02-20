#ifndef SITE_CLASSIFIER_BRIDGE_HPP
#define SITE_CLASSIFIER_BRIDGE_HPP

#include <QObject>
#include <QProcess>
#include <QString>
#include <QtQml/qqmlregistration.h>

class SiteClassifier : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString level READ level NOTIFY resultChanged)
    Q_PROPERTY(QString name READ name NOTIFY resultChanged)
    Q_PROPERTY(QString iconUrl READ iconUrl NOTIFY resultChanged)
    Q_PROPERTY(QString displayMode READ displayMode NOTIFY resultChanged)
    Q_PROPERTY(QString themeColor READ themeColor NOTIFY resultChanged)
    Q_PROPERTY(QString startUrl READ startUrl NOTIFY resultChanged)
    Q_PROPERTY(bool hasManifest READ hasManifest NOTIFY resultChanged)
    Q_PROPERTY(bool hasServiceWorker READ hasServiceWorker NOTIFY resultChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY resultChanged)
    Q_PROPERTY(bool hasResult READ hasResult NOTIFY resultChanged)

public:
    explicit SiteClassifier(QObject *parent = nullptr);

    bool busy() const { return m_busy; }
    QString level() const { return m_level; }
    QString name() const { return m_name; }
    QString iconUrl() const { return m_iconUrl; }
    QString displayMode() const { return m_displayMode; }
    QString themeColor() const { return m_themeColor; }
    QString startUrl() const { return m_startUrl; }
    bool hasManifest() const { return m_hasManifest; }
    bool hasServiceWorker() const { return m_hasServiceWorker; }
    QString errorMessage() const { return m_errorMessage; }
    bool hasResult() const { return m_hasResult; }

    Q_INVOKABLE void classify(const QString &url);

signals:
    void busyChanged();
    void resultChanged();

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void clearResult();
    void parseResult(const QByteArray &output);

    QProcess *m_process = nullptr;
    bool m_busy = false;
    bool m_hasResult = false;
    QString m_level;
    QString m_name;
    QString m_iconUrl;
    QString m_displayMode;
    QString m_themeColor;
    QString m_startUrl;
    bool m_hasManifest = false;
    bool m_hasServiceWorker = false;
    QString m_errorMessage;
};

#endif // SITE_CLASSIFIER_BRIDGE_HPP

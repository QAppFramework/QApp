#ifndef WRAPPER_HELPER_HPP
#define WRAPPER_HELPER_HPP

#include <QObject>
#include <QProcess>
#include <QString>
#include <QtQml/qqmlregistration.h>

class WrapperHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)

public:
    explicit WrapperHelper(QObject *parent = nullptr);

    bool busy() const { return m_busy; }
    QString statusMessage() const { return m_statusMessage; }

    Q_INVOKABLE void saveAsApp(const QString &url, const QString &name);

signals:
    void busyChanged();
    void statusChanged();

private slots:
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QProcess *m_process = nullptr;
    bool m_busy = false;
    QString m_statusMessage;
};

#endif // WRAPPER_HELPER_HPP

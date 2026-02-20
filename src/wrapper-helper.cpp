#include "wrapper-helper.hpp"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>

WrapperHelper::WrapperHelper(QObject *parent)
    : QObject(parent)
{
}

void WrapperHelper::saveAsApp(const QString &url, const QString &name)
{
    if (m_busy) return;

    m_busy = true;
    m_statusMessage = "Installing...";
    emit busyChanged();
    emit statusChanged();

    m_process = new QProcess(this);
    connect(m_process, &QProcess::finished, this, &WrapperHelper::onFinished);

    QString scriptPath = QCoreApplication::applicationDirPath() + "/../bin/install.js";
    QString wrapperPath = QCoreApplication::applicationDirPath() + "/qapp-wrapper";

    QStringList processArgs = {scriptPath, url, "--wrapper-path", wrapperPath};
    if (!name.isEmpty()) {
        processArgs << "--name" << name;
    }

    m_process->start("node", processArgs);

    if (!m_process->waitForStarted(5000)) {
        m_statusMessage = "Failed to start install process";
        m_busy = false;
        emit busyChanged();
        emit statusChanged();
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void WrapperHelper::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_busy = false;
    emit busyChanged();

    if (!m_process) return;

    QByteArray output = m_process->readAllStandardOutput();

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QJsonDocument doc = QJsonDocument::fromJson(output);
        if (doc.isObject()) {
            QString name = doc.object()["name"].toString();
            m_statusMessage = "Saved as app: " + name;
        } else {
            m_statusMessage = "Saved as app";
        }
    } else {
        QJsonDocument doc = QJsonDocument::fromJson(output);
        if (doc.isObject() && doc.object().contains("error")) {
            m_statusMessage = doc.object()["error"].toString();
        } else {
            m_statusMessage = "Install failed";
        }
    }

    emit statusChanged();
    m_process->deleteLater();
    m_process = nullptr;
}

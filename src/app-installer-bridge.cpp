#include "app-installer-bridge.hpp"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

AppInstaller::AppInstaller(QObject *parent)
    : QObject(parent)
{
}

void AppInstaller::install(const QString &url)
{
    if (m_busy) return;

    clearResult();
    m_busy = true;
    emit busyChanged();

    m_process = new QProcess(this);

    connect(m_process, &QProcess::finished, this, &AppInstaller::onInstallFinished);

    QString scriptPath = QCoreApplication::applicationDirPath() + "/../bin/install.js";
    QString wrapperPath = QCoreApplication::applicationDirPath() + "/qapp-wrapper";

    m_process->start("node", {scriptPath, url, "--wrapper-path", wrapperPath});

    if (!m_process->waitForStarted(5000)) {
        m_errorMessage = "Failed to start Node.js process";
        m_busy = false;
        emit busyChanged();
        emit resultChanged();
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void AppInstaller::uninstall(const QString &appId)
{
    if (m_busy) return;

    clearResult();
    m_busy = true;
    emit busyChanged();

    m_process = new QProcess(this);

    connect(m_process, &QProcess::finished, this, &AppInstaller::onUninstallFinished);

    QString scriptPath = QCoreApplication::applicationDirPath() + "/../bin/uninstall.js";

    m_process->start("node", {scriptPath, appId});

    if (!m_process->waitForStarted(5000)) {
        m_errorMessage = "Failed to start Node.js process";
        m_busy = false;
        emit busyChanged();
        emit resultChanged();
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void AppInstaller::onInstallFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_busy = false;
    emit busyChanged();

    if (!m_process) return;

    QByteArray output = m_process->readAllStandardOutput();
    QByteArray errOutput = m_process->readAllStandardError();

    if (exitStatus != QProcess::NormalExit) {
        m_errorMessage = "Install process crashed";
        emit resultChanged();
    } else if (exitCode != 0) {
        QJsonDocument doc = QJsonDocument::fromJson(output);
        if (doc.isObject() && doc.object().contains("error")) {
            m_errorMessage = doc.object()["error"].toString();
        } else {
            m_errorMessage = QString::fromUtf8(errOutput).trimmed();
            if (m_errorMessage.isEmpty()) {
                m_errorMessage = "Install failed (exit code " + QString::number(exitCode) + ")";
            }
        }
        emit resultChanged();
    } else {
        QJsonDocument doc = QJsonDocument::fromJson(output);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            m_appId = obj["appId"].toString();
            m_appName = obj["name"].toString();
            m_desktopPath = obj["desktopPath"].toString();
            m_installed = true;
        } else {
            m_errorMessage = "Invalid JSON output from installer";
        }
        emit resultChanged();
    }

    m_process->deleteLater();
    m_process = nullptr;
}

void AppInstaller::onUninstallFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_busy = false;
    emit busyChanged();

    if (!m_process) return;

    QByteArray output = m_process->readAllStandardOutput();
    QByteArray errOutput = m_process->readAllStandardError();

    if (exitStatus != QProcess::NormalExit) {
        m_errorMessage = "Uninstall process crashed";
    } else if (exitCode != 0) {
        QJsonDocument doc = QJsonDocument::fromJson(output);
        if (doc.isObject() && doc.object().contains("error")) {
            m_errorMessage = doc.object()["error"].toString();
        } else {
            m_errorMessage = QString::fromUtf8(errOutput).trimmed();
            if (m_errorMessage.isEmpty()) {
                m_errorMessage = "Uninstall failed (exit code " + QString::number(exitCode) + ")";
            }
        }
    } else {
        m_installed = false;
        m_appId.clear();
        m_appName.clear();
        m_desktopPath.clear();
    }

    emit resultChanged();
    m_process->deleteLater();
    m_process = nullptr;
}

void AppInstaller::clearResult()
{
    m_installed = false;
    m_appId.clear();
    m_appName.clear();
    m_desktopPath.clear();
    m_errorMessage.clear();
}

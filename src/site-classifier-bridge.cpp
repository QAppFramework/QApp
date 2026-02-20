#include "site-classifier-bridge.hpp"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

SiteClassifier::SiteClassifier(QObject *parent)
    : QObject(parent)
{
}

void SiteClassifier::classify(const QString &url)
{
    if (m_busy) return;

    clearResult();
    m_busy = true;
    emit busyChanged();

    m_process = new QProcess(this);

    connect(m_process, &QProcess::finished, this, &SiteClassifier::onProcessFinished);

    // Find the classify.js script relative to the application directory
    QString scriptPath = QCoreApplication::applicationDirPath() + "/../bin/classify.js";

    m_process->start("node", {scriptPath, url});

    if (!m_process->waitForStarted(5000)) {
        m_errorMessage = "Failed to start Node.js process";
        m_hasResult = true;
        m_busy = false;
        emit busyChanged();
        emit resultChanged();
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void SiteClassifier::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_busy = false;
    emit busyChanged();

    if (!m_process) return;

    QByteArray output = m_process->readAllStandardOutput();
    QByteArray errOutput = m_process->readAllStandardError();

    if (exitStatus != QProcess::NormalExit) {
        m_errorMessage = "Classification process crashed";
        m_hasResult = true;
        emit resultChanged();
    } else if (exitCode != 0) {
        // Parse error JSON from stdout
        QJsonDocument doc = QJsonDocument::fromJson(output);
        if (doc.isObject() && doc.object().contains("error")) {
            m_errorMessage = doc.object()["error"].toString();
        } else {
            m_errorMessage = QString::fromUtf8(errOutput).trimmed();
            if (m_errorMessage.isEmpty()) {
                m_errorMessage = "Classification failed (exit code " + QString::number(exitCode) + ")";
            }
        }
        m_hasResult = true;
        emit resultChanged();
    } else {
        parseResult(output);
    }

    m_process->deleteLater();
    m_process = nullptr;
}

void SiteClassifier::clearResult()
{
    m_hasResult = false;
    m_level.clear();
    m_name.clear();
    m_iconUrl.clear();
    m_displayMode.clear();
    m_themeColor.clear();
    m_startUrl.clear();
    m_hasManifest = false;
    m_hasServiceWorker = false;
    m_errorMessage.clear();
}

void SiteClassifier::parseResult(const QByteArray &output)
{
    QJsonDocument doc = QJsonDocument::fromJson(output);
    if (!doc.isObject()) {
        m_errorMessage = "Invalid JSON output from classifier";
        m_hasResult = true;
        emit resultChanged();
        return;
    }

    QJsonObject root = doc.object();
    QJsonObject classification = root["classification"].toObject();
    QJsonObject metadata = root["metadata"].toObject();

    m_level = classification["level"].toString();
    m_hasManifest = classification["hasManifest"].toBool();
    m_hasServiceWorker = classification["hasServiceWorker"].toBool();

    m_name = metadata["name"].toString();
    m_iconUrl = metadata["iconUrl"].toString();
    m_displayMode = metadata["displayMode"].toString();
    m_themeColor = metadata["themeColor"].toString();
    m_startUrl = metadata["startUrl"].toString();

    m_hasResult = true;
    emit resultChanged();
}

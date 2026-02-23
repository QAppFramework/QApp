#pragma once
#include "result.hpp"
#include "html-parser.hpp"
#include "manifest-parser.hpp"
#include <QObject>
#include <QUrl>
#include <QJsonObject>
#include <QJsonArray>

class QNetworkAccessManager;
class QNetworkReply;

namespace qapp::installer {

struct ClassifyResult {
    // Classification
    QString level;       // "WS", "WAPP", "PWAPP"
    bool hasManifest = false;
    bool hasServiceWorker = false;

    // Metadata
    QString name;
    QString iconUrl;
    QString displayMode;
    QString themeColor;
    QString backgroundColor;
    QString startUrl;
    QString scope;
    QString finalUrl;    // URL after redirects
    QString manifestId;
    QStringList displayOverride;
    QJsonArray shortcuts;
    QStringList mimeTypes;         // from file_handlers
    QJsonArray protocolHandlers;   // [{protocol, url}]

    QJsonObject toJson() const;
};

class ClassifyPipeline : public QObject {
    Q_OBJECT
public:
    explicit ClassifyPipeline(QObject *parent = nullptr);

    void start(const QUrl &url);
    void startFromData(const QString &html, const QUrl &finalUrl,
                       bool isHttps, const QJsonObject &manifestJson,
                       bool swDetected);

signals:
    void statusChanged(const QString &msg);
    void finished(const qapp::installer::ClassifyResult &result);
    void failed(const QString &error);

private:
    void fetchPage();
    void onPageFetched(QNetworkReply *reply);
    void parseHtmlAndContinue();
    void fetchManifest();
    void onManifestFetched(QNetworkReply *reply);
    void detectServiceWorker();
    void probeSwPath(int index);
    void onSwProbeDone(QNetworkReply *reply, int index);
    void classify();
    void resolveIcon();
    void probeFavicon(int index);
    void onFaviconProbeDone(QNetworkReply *reply, int index);
    void emitResult();

    QNetworkAccessManager *m_nam = nullptr;
    QUrl m_url;
    QString m_html;
    qapp::HtmlMeta m_htmlMeta;
    qapp::Manifest m_manifest;
    bool m_hasManifest = false;
    bool m_swDetected = false;
    ClassifyResult m_result;
};

} // namespace qapp::installer

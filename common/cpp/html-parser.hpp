#pragma once
#include <QString>
#include <QUrl>
#include <QVector>
#include <QPair>

namespace qapp {

struct HtmlMeta {
    QString manifestUrl;
    QString title;
    QVector<QPair<QString, QString>> icons; // href, sizes
    QString themeColor;
    QString ogImage;
    bool swHint = false;
};

class HtmlParser {
public:
    static HtmlMeta parse(const QString &html, const QUrl &baseUrl);

private:
    static QString resolveUrl(const QString &href, const QUrl &baseUrl);
};

} // namespace qapp

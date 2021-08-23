//
// Created by Reverier-Xu on 2021/7/29.
//

#pragma once

#include <QObject>
#include <QUuid>
#include "models/media.h"

class BaseParser : public QObject {
Q_OBJECT
private:
    QUuid mParserId = QUuid(nullptr);

public:
    explicit BaseParser(QObject *parent = nullptr) : QObject(parent) {}

    BaseParser(const BaseParser &parser) {
        this->setParent(parser.parent());
    }

    ~BaseParser() override = default;

    [[nodiscard]] virtual BaseParser *clone() = 0;

    [[nodiscard]] virtual const QStringList &acceptTypes() = 0;

    [[nodiscard]] virtual bool accepted(const Media &media) = 0;

    [[nodiscard]] virtual bool accepted(const QString &path) = 0;

    [[nodiscard]] virtual Media getMedia(const QString &path) = 0;

    [[nodiscard]] virtual Media parseMedia(const Media &media) = 0;

    [[nodiscard]] virtual QString getMediaCover(const Media &media) = 0;

    [[nodiscard]] QUuid parserId() { return this->mParserId; }

    void setParserId(const QUuid &n) {
        this->mParserId = n;
        emit this->parserIdChanged(n);
    }

signals:

    void parserIdChanged(const QUuid &n);
};

//
// Created by Reverier-Xu on 2021/6/25.
//

#include "player_manager.h"

#include <QSettings>
#include <QPixmap>

#include "parser_manager.h"
#include "queue_manager.h"

PlayerManager *PlayerManager::mInstance = nullptr;

PlayerManager::PlayerManager(QObject *parent) : BaseManager(parent) {
    try {
        this->mEngine = MpvEngine::instance(this);
        this->setIsReady(true);
    } catch (...) {
        this->setIsReady(false);
    }

    this->mScreensaver = Screensaver::GetScreensaver();

    this->connectSignals();
    this->loadSettings();
}

PlayerManager::~PlayerManager() {
    this->saveSettings();
}

void PlayerManager::loadSettings() {
    QSettings settings;
    settings.beginGroup("Player");
    this->setVolume(settings.value("Volume", this->volume()).toDouble());
    this->setIsMuted(settings.value("Muted", this->isMuted()).toBool());
    settings.endGroup();
}

void PlayerManager::saveSettings() {
    QSettings settings;
    settings.beginGroup("Player");
    settings.setValue("Volume", this->volume());
    settings.setValue("Muted", this->isMuted());
    settings.endGroup();

    settings.sync();
}

PlayerManager *PlayerManager::instance(QObject *parent) {
    if (mInstance == nullptr) mInstance = new PlayerManager(parent);
    return mInstance;
}

void PlayerManager::connectSignals() {
    connect(this->mEngine, &MpvEngine::durationChanged,
            [=](double secs) {
                this->setTotalTime(secs);
            });
    connect(this->mEngine, &MpvEngine::newMusicOpened,
            [=]() { this->setCurrentMediaIsVideo(false); });
    connect(this->mEngine, &MpvEngine::newVideoOpened,
            [=]() { this->setCurrentMediaIsVideo(true); });
    connect(this->mEngine, &MpvEngine::positionChanged, [=](double msecs) {
        this->setCurrentTime(msecs);
        this->setCurrentLyricIndex(0);
    });
    connect(this->mEngine, &MpvEngine::resumed, [=]() {
        this->setPlaying(true);
        // qDebug() << "resumed";
        emit this->stateChanged();
    });
    connect(this->mEngine, &MpvEngine::paused, [=]() {
        this->setPlaying(false);
        // qDebug() << "paused";
        emit this->stateChanged();
    });
    connect(this->mEngine, &MpvEngine::started, [=]() {
        this->setPlaying(true);
        this->setIsMediaLoaded(true);
        // qDebug() << "started";
        emit this->stateChanged();
    });
    connect(this->mEngine, &MpvEngine::ended, [=]() {
        this->setIsMediaLoaded(false);
        // qDebug() << "ended";
        emit this->stateChanged();
        if (!QueueManager::instance(this->parent())->queueEnded())
            if (this->totalTime() - this->currentTime() < 0.5)
                QueueManager::instance(this->parent())->next();
    });
    connect(QueueManager::instance(this->parent()), &QueueManager::currentMediaChanged, this,
            &PlayerManager::play);

    connect(this, &PlayerManager::mediaParseRequired, ParserManager::instance(),
            &ParserManager::handleParseMediaRequest, Qt::QueuedConnection);
    connect(this, &PlayerManager::mediaCoverRequired, ParserManager::instance(),
            &ParserManager::handleGetMediaCoverRequest, Qt::QueuedConnection);
    connect(this, &PlayerManager::coverColorRequired, ParserManager::instance(),
            &ParserManager::handleGetMediaCoverColorRequest, Qt::QueuedConnection);
    connect(this, &PlayerManager::mediaLyricsRequired,
            ParserManager::instance(),
            &ParserManager::handleGetMediaLyricsRequest, Qt::QueuedConnection);

    connect(ParserManager::instance(), &ParserManager::mediaCoverIsReady, this,
            &PlayerManager::handleMediaCoverIsReady, Qt::QueuedConnection);
    connect(ParserManager::instance(), &ParserManager::mediaIsReady, this,
            &PlayerManager::handleMediaIsReady, Qt::QueuedConnection);
    connect(ParserManager::instance(), &ParserManager::mediaLyricsIsReady, this,
            &PlayerManager::handleMediaLyricsIsReady, Qt::QueuedConnection);
    connect(ParserManager::instance(), &ParserManager::mediaCoverColorIsReady, this,
            &PlayerManager::handleCoverColorIsReady, Qt::QueuedConnection);
    connect(QueueManager::instance(), &QueueManager::playQueueEnded, this,
            &PlayerManager::handlePlayQueueEnded, Qt::QueuedConnection);

    if (this->mScreensaver != nullptr) {
        connect(this->mEngine, &MpvEngine::started, [=]() {
            // qDebug() << "Started";
            if (this->currentMediaIsVideo()) {
                // qDebug() << "Inhibit";
                this->mScreensaver->Inhibit();
            }
        });
        connect(this->mEngine, &MpvEngine::ended, [=]() {
            this->mScreensaver->UnInhibit();
            // qDebug() << "UnInhibit";
        });
        connect(this, &PlayerManager::isPlayingChanged, [=](bool n) {
            if (this->currentMediaIsVideo() and n) {
                // qDebug() << "Inhibit";
                this->mScreensaver->Inhibit();
            } else {
                // qDebug() << "UnInhibit";
                this->mScreensaver->UnInhibit();
            }
        });
    }
}

void PlayerManager::handlePlayQueueEnded() {
    this->setPlaying(false);
    this->setCurrentMediaIsVideo(false);
    this->setCurrentTime(0);
    this->setTotalTime(0);
    this->setCurrentLyricIndex(0);
    this->setCurrentMediaCover("qrc:/assets/music-big.svg");
    this->setCurrentMediaTitle("No Media");
    this->setCoverColor(QColor(0x00, 0x78, 0xd6));
    this->setCurrentMediaAlbum("No Album");
    this->setCurrentMediaArtist("No Artist");
    this->stop();
}

void PlayerManager::userDragHandler(double t) { this->mEngine->setTimePos(t); }

void PlayerManager::play(const Media &m) {
    // qDebug() << "playing: " << m.title();

    this->setCurrentMediaIsVideo(m.type() == VIDEO);
    // qDebug() << media.collection();

    this->setCurrentMediaAlbum(m.collection());
    this->setCurrentMediaArtist(m.artist());
    this->setCurrentMediaTitle(m.title());
    this->setIsMediaLoading(true);
    this->pause();

    emit this->mediaCoverRequired(m);
    emit this->mediaParseRequired(m);
    if (m.type() == AUDIO) emit this->mediaLyricsRequired(m);
    // qDebug() << "mediaParseRequired is emitted.";
}

void PlayerManager::pause() {
    this->mEngine->pause();
    // qDebug() << "paused";
}

void PlayerManager::resume() {
    this->mEngine->resume();
    // qDebug() << "resumed";
}

void PlayerManager::stop() {
    this->mEngine->stop();
    // qDebug() << "stopped";
}

void PlayerManager::playUrl(const QString &m) {
    this->mEngine->playMedia(m);
}

void PlayerManager::setLyrics(const QString &raw, const QString &tr) {
    this->mLyricsModel.parseLyrics(raw, tr);
    this->setIsLyricLoaded(2);
}

void PlayerManager::handleMediaIsReady(bool ok, const Media &m) {
    if (ok) {
        this->playUrl(m.rawUrl());
        this->setIsMediaLoading(false);
        this->resume();
    } else {
        QueueManager::instance(this->parent())->next();
    }
}

void PlayerManager::handleMediaCoverIsReady(bool ok, const QString &m) {
    if (ok) {
        this->setCurrentMediaCover(m);
        emit this->coverColorRequired(m);
    } else {
        this->setCurrentMediaCover("qrc:/assets/archive-big.svg");
        this->setCoverColor(QColor(0xd7, 0x75, 0x87));
    }
}

void PlayerManager::handleMediaLyricsIsReady(bool ok, const QString &raw,
                                             const QString &trans) {
    if (ok)
        this->setLyrics(raw, trans);
    else
        this->setLyrics(tr("[00:00.00] Lyrics Not Found."));
}

void PlayerManager::handleCoverColorIsReady(bool ok, const QColor &color) {
    if (ok)
        this->setCoverColor(color);
    else
        this->setCoverColor(QColor(0x00, 0x78, 0xd6));
}

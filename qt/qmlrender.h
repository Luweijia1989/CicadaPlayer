#pragma once
#include <MediaPlayer.h>
#include <QQuickFramebufferObject>
#include <SimpleEffectPlayer.h>

using namespace Cicada;

class QMLPlayer : public QQuickFramebufferObject {
    Q_OBJECT
public:
    QMLPlayer(QQuickItem *parent = nullptr);
    ~QMLPlayer();

    QQuickFramebufferObject::Renderer *createRenderer() const;

    Q_INVOKABLE void setMixInfo(QString info);
    Q_INVOKABLE void setSource(QString path);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void testplay();

    static void onVideoSize(int64_t width, int64_t height, void *userData);
    static void onFirstFrame(void *userData);

    Q_INVOKABLE void seektobegin();

signals:
    void videoRatioChanged(qreal ratio);
    void ended();
    void firstFrame();

private:
    std::shared_ptr<MediaPlayer> internal_player;
};

class SimpleQMLPlayer : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(QString sourceTag READ sourceTag WRITE setSourceTag NOTIFY sourceTagChanged)
    Q_PROPERTY(QString sourceUrl READ sourceUrl WRITE setSourceUrl NOTIFY sourceUrlChanged)
public:
    SimpleQMLPlayer(QQuickItem *parent = nullptr);
    ~SimpleQMLPlayer();

    QQuickFramebufferObject::Renderer *createRenderer() const;

    Q_INVOKABLE void play1();
    Q_INVOKABLE void play2();
    Q_INVOKABLE void play3(QString path);

public:
    QString sourceTag();
    void setSourceTag(const QString &tag);

    QString sourceUrl();
    void setSourceUrl(const QString &url);

signals:
    void sourceTagChanged();
    void sourceUrlChanged();

protected:
    static void onVideoSize(int64_t width, int64_t height, void *userData);
    static void onEOS(void *userData);
    static void onFirstFrame(void *userData);

signals:
    void videoRatioChanged(qreal ratio);
    void ended();
    void firstFrame();

private:
    std::shared_ptr<SimpleEffectPlayer> internal_player;

    QString m_sourceUrl = "";
    QString m_sourceTag = "unknown";
};
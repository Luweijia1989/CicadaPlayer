#pragma once
#include <QQuickFramebufferObject>
#include <MediaPlayer.h>

using namespace Cicada;

class QMLPlayer : public QQuickFramebufferObject
{
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
    static void onEOS(void *userData);
	static void onFirstFrame(void *userData);

signals:
	void videoRatioChanged(qreal ratio);
	void ended();
	void firstFrame();

private:
    std::shared_ptr< MediaPlayer > internal_player;
};
#include "qmlrender.h"
#include <qquickwindow.h>

class VideoRendererInternal : public QQuickFramebufferObject::Renderer {
public:
    VideoRendererInternal(const std::shared_ptr<MediaPlayer> &p, void *vo) : player(p), m_vo(vo)
    {}

    ~VideoRendererInternal()
    {
        auto p = player.lock();
        if (p)
            p->clearGLResource(m_vo);
        else
            MediaPlayer::foreignGLContextDestroyed(m_vo);
    }

    void render() override
    {
        auto p = player.lock();
        if (p) p->renderVideo(m_vo);
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        auto p = player.lock();
        if (p) p->setVideoSurfaceSize(size.width(), size.height(), m_vo);
		QOpenGLFramebufferObjectFormat format;
		format.setInternalTextureFormat(GL_RGBA);
        return new QOpenGLFramebufferObject(size, format);
    }

    std::weak_ptr<MediaPlayer> player;
	void *m_vo;
};


QMLPlayer::QMLPlayer(QQuickItem *parent) : QQuickFramebufferObject(parent), internal_player(std::make_shared<MediaPlayer>())
{
    setMirrorVertically(true);
    internal_player->setRenderCallback([this](void *) { QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection); }, this);

    internal_player->SetDataSource("https://tvideo.bxapp.cn/ad1ba4b647cc47a59518e53598cb4db4.mp4");
	//internal_player->setMaskMode(
 //           IVideoRender::Mask_Right,
 //           u8"{\"[imgUser]\":\"C:/Users/posat/Desktop/big.jpeg\", \"[textUser]\":\"luweijia\", \"[textAnchor]\":\"rurongrong\"}");
    internal_player->SetAutoPlay(true);
    internal_player->SetLoop(false);
    internal_player->Prepare();
}

QMLPlayer::~QMLPlayer()
{
    internal_player->setRenderCallback(nullptr, this);
}

QQuickFramebufferObject::Renderer *QMLPlayer::createRenderer() const
{
    return new VideoRendererInternal(internal_player, (void *)this);
}

#include <QThread>
void QMLPlayer::test()
{
	internal_player->Stop();
	internal_player->SetDataSource("https://video.hellobixin.com/video/dc37c4361114437ab84f1ebef1bbb7e5.mp4");
	/*internal_player->setMaskMode(
            IVideoRender::Mask_Right,
            u8"{\"[imgUser]\":\"C:/Users/posat/Desktop/big.jpeg\", \"[textUser]\":\"luweijia\", \"[textAnchor]\":\"rurongrong\"}");*/
    internal_player->SetAutoPlay(true);
    internal_player->SetLoop(false);
    internal_player->Prepare();
}
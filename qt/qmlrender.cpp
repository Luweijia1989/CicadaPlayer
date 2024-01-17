#include "qmlrender.h"
#include <QOpenGLFramebufferObjectFormat>
#include <qdebug.h>
#include <qelapsedtimer.h>
#include <qtimer.h>

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
        if (p) {
            QElapsedTimer t;
            t.start();
            p->renderVideo(m_vo);
            qDebug() << t.elapsed();
        }
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        auto p = player.lock();
        if (p) p->setVideoSurfaceSize(size.width(), size.height(), m_vo);
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        return new QOpenGLFramebufferObject(size, format);
    }

    std::weak_ptr<MediaPlayer> player;
    void *m_vo;
};

void QMLPlayer::onVideoSize(int64_t width, int64_t height, void *userData)
{
    QMLPlayer *p = (QMLPlayer *) userData;
    if (p) emit p->videoRatioChanged((qreal) width / (qreal) height);
}

void QMLPlayer::onFirstFrame(void *userData)
{
    QMLPlayer *p = (QMLPlayer *) userData;
    if (p) emit p->firstFrame();
}

QMLPlayer::QMLPlayer(QQuickItem *parent) : QQuickFramebufferObject(parent), internal_player(std::make_shared<MediaPlayer>())
{
    playerListener pListener{nullptr};
    pListener.userData = this;
    pListener.VideoSizeChanged = onVideoSize;
    pListener.FirstFrameShow = onFirstFrame;
    internal_player->SetListener(pListener);

    setMirrorVertically(true);
    internal_player->EnableHardwareDecoder(true);
    internal_player->setRenderCallback([this](void *) { QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection); }, this);
    //internal_player->setMaskMode(IVideoRender::Mask_Right);
    //internal_player->setSmoothLoop(true);
    internal_player->SetLoop(true);

    //setSource("C:\\Users\\luweijia\\Desktop\\aework\\output-2.mp4");
    internal_player->setMaskMode(
            IVideoRender::Mask_Right,
            "{\"[textAnchor]\":\"aaaa\",\"[textUser]\":\"bbbb\",\"[imgAnchor]\":\"D:/test.jpg\",\"[imgUser]\":\"D:/ccc.jpg\"}");
    setSource(u8R"(D:\crf_compose.mp4)");
}

QMLPlayer::~QMLPlayer()
{
    internal_player->setRenderCallback(nullptr, this);
}

QQuickFramebufferObject::Renderer *QMLPlayer::createRenderer() const
{
    return new VideoRendererInternal(internal_player, (void *) this);
}

void QMLPlayer::setMixInfo(QString info)
{
    //internal_player->setMaskMode(IVideoRender::Mask_Right, info.toStdString());
}

void QMLPlayer::setSource(QString path)
{
    std::string p = path.toStdString();
    internal_player->SetDataSource(p.c_str());
    internal_player->SetAutoPlay(true);
    //internal_player->SetLoop(false);
    internal_player->Prepare();
}

void QMLPlayer::stop()
{
    internal_player->Stop();
}

void QMLPlayer::testplay()
{
    stop();
    setMixInfo("");
    setSource("D:\\stage_level_1.mp4");
}

void QMLPlayer::seektobegin()
{
    stop();
    setMixInfo("");
    setSource("C:\\Users\\luweijia\\AppData\\Local\\yuerlive\\cache\\image\\stage\\stage_level_interval.mp4");
}


class SimpleVideoRendererInternal : public QQuickFramebufferObject::Renderer {
public:
    SimpleVideoRendererInternal(const std::shared_ptr<SimpleEffectPlayer> &p, void *vo, QString videoTag)
        : player(p), m_vo(vo), m_videoTag(videoTag)
    {
        qInfo() << __FUNCTION__;
    }

    ~SimpleVideoRendererInternal()
    {
        auto p = player.lock();
        if (p)
            p->clearGLResource(m_vo);
        else
            SimpleEffectPlayer::foreignGLContextDestroyed(m_vo,m_videoTag.toStdString());
    }

    void render() override
    {
        auto p = player.lock();
        if (p) {
            p->renderVideo(m_vo, fbo_id);
        }
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        qInfo() << __FUNCTION__;
        auto p = player.lock();
        if (p) p->setSurfaceSize(m_vo, size.width(), size.height());
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        auto fbo = new QOpenGLFramebufferObject(size, format);
        fbo_id = fbo->handle();
        return fbo;
    }

    uint fbo_id = -1;
    std::weak_ptr<SimpleEffectPlayer> player;
    void *m_vo;
    QString m_videoTag;
};

SimpleQMLPlayer::SimpleQMLPlayer(QQuickItem *parent)
    : QQuickFramebufferObject(parent), internal_player(std::make_shared<SimpleEffectPlayer>())
{
    setMirrorVertically(true);

    connect(this, &SimpleQMLPlayer::sourceTagChanged, this, [=]() {
        //qInfo() << "sourceTagChanged" << m_sourceTag;
        if (internal_player) {
            internal_player->setSourceTag(m_sourceTag.toStdString());

            playerListener pListener{nullptr};
            pListener.userData = this;
            pListener.VideoSizeChanged = onVideoSize;
            pListener.Completion = onEOS;
            pListener.FirstFrameShow = onFirstFrame;
            internal_player->SetListener(pListener);

            internal_player->setSmoothLoop(false);
            internal_player->EnableHardwareDecoder(true);
            internal_player->setRenderCallback([this](void *) { QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection); }, this);
        }
    });

    connect(this, &SimpleQMLPlayer::sourceUrlChanged, this, [=]() {
        qInfo() << "sourceUrlChanged" << m_sourceUrl;
        return;
        QTimer::singleShot(100, this, [=]() {
            qInfo() << "start render special videofile" << m_sourceUrl;
            if (internal_player && m_sourceUrl.length()) {
                internal_player->stop();
                internal_player->start(m_sourceUrl.toStdString());
            }
        });
    });
}

SimpleQMLPlayer::~SimpleQMLPlayer()
{
    internal_player->setRenderCallback(nullptr, this);
}

QQuickFramebufferObject::Renderer *SimpleQMLPlayer::createRenderer() const
{
    return new SimpleVideoRendererInternal(internal_player, (void *) this,m_sourceTag);
}

void SimpleQMLPlayer::play1()
{
    internal_player->stop();
    internal_player->setMaskMode(
            IVideoRender::Mask_Right,
            "{\"[textAnchor]\":\"aaaa\",\"[textUser]\":\"bbbb\",\"[imgAnchor]\":\"D:/test.jpg\",\"[imgUser]\":\"D:/ccc.jpg\"}");
    internal_player->start(u8R"(C:\Users\luweijia\AppData\Local\yuerlive\cache\image\stage\stage_level_3.mp4)");
}

void SimpleQMLPlayer::play2()
{
    internal_player->stop();
    internal_player->start("F:\\workspace\\avsolution\\screen-recorder\\x64\\Debug_DLL\\hlqk\\output\\crf_compose.mp4");
}

void SimpleQMLPlayer::play3(QString path)
{
    qInfo() << __FUNCTION__ << path;
    QFile file(path);
    if (file.exists()) {
        internal_player->stop();
        internal_player->start(path.toStdString());
    } else {
        qInfo() << "file not exist";
    }
}

void SimpleQMLPlayer::stop()
{
    qInfo() << __FUNCTION__;
    internal_player->stop();
}

QString SimpleQMLPlayer::sourceTag()
{
    return m_sourceTag;
}

void SimpleQMLPlayer::setSourceTag(const QString &tag)
{
    if (m_sourceTag != tag) {
        m_sourceTag = tag;
        sourceTagChanged();
    }
}

QString SimpleQMLPlayer::sourceUrl()
{
    return m_sourceUrl;
}
void SimpleQMLPlayer::setSourceUrl(const QString &url)
{
    qInfo() << __FUNCTION__ << url;
    if (m_sourceUrl != url) {
        m_sourceUrl = url;
        sourceUrlChanged();
    }
}

void SimpleQMLPlayer::onVideoSize(int64_t width, int64_t height, void *userData)
{
    qInfo() << __FUNCTION__ << width << height << userData;
}
void SimpleQMLPlayer::onEOS(void *userData)
{
    qInfo() << __FUNCTION__ << userData;
    if (userData) {
        SimpleQMLPlayer *p = (SimpleQMLPlayer *) userData;
        emit p->ended();
    }
}
void SimpleQMLPlayer::onFirstFrame(void *userData)
{
    qInfo() << __FUNCTION__ << userData;
}
#include "MediaPlayer.h"
#include "glwidget.h"
#include <QApplication>
#include <QPushButton>
#include <QTimer>

using namespace Cicada;
using namespace std;

class A {
public:
    A(int a) : m(a)
    {}

    int m;
};
//#define DEMO_QUICKWIDGET
#ifdef DEMO_QUICKWIDGET
OpenGLWidget *gl = nullptr;

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication a(argc, argv);

    auto player = std::shared_ptr<MediaPlayer>(new MediaPlayer());

    //player->setMaskMode(
    //        IVideoRender::Mask_Right,
    //        u8"{\"[imgUser]\":\"C:/Users/posat/Desktop/big.jpeg\", \"[textUser]\":\"luweijia\", \"[textAnchor]\":\"rurongrong\"}");
    player->SetRotateMode(ROTATE_MODE_0);
    player->SetScaleMode(SM_FIT);

    //player->SetSpeed(0.5);
    player->SetDefaultBandWidth(1000 * 1000);
    //player->SetDataSource("http://player.alicdn.com/video/aliyunmedia.mp4");
    //player->SetDataSource("E:\\vap1.mp4");
    player->SetDataSource("C:\\Users\\posat\\Desktop\\7p.mp4");
    player->SetAutoPlay(true);
    player->SetLoop(true);
    player->SetIPResolveType(IpResolveWhatEver);
    player->SetFastStart(true);
    MediaPlayerConfig config = *(player->GetConfig());
    config.mMaxBackwardBufferDuration = 20000;
    config.liveStartIndex = -3;
    player->SetConfig(&config);
    player->Prepare();
    player->SelectTrack(-1);

    QPushButton btn("new one");
    QObject::connect(&btn, &QPushButton::clicked, [=]() {
        OpenGLWidget *w = new OpenGLWidget(player);
        w->resize(640, 480);
        w->show();
    });
    btn.show();

    //QTimer t;
    //QObject::connect(&t, &QTimer::timeout, [=]() {
    //    {
    //        OpenGLWidget *w = new OpenGLWidget(player);
    //        w->resize(640, 480);
    //        w->show();
    //    }
    //{
    //    OpenGLWidget *w = new OpenGLWidget(player);
    //    w->resize(640, 480);
    //    w->show();
    //}
    //{
    //    OpenGLWidget *w = new OpenGLWidget(player);
    //    w->resize(640, 480);
    //    w->show();
    //}
    //{
    //    OpenGLWidget *w = new OpenGLWidget(player);
    //    w->resize(640, 480);
    //    w->show();
    //}
    //{
    //    OpenGLWidget *w = new OpenGLWidget(player);
    //    w->resize(640, 480);
    //    w->show();
    //}
    //});
    //t.start(300);

    //QTimer::singleShot(5000, [=]() {
    //	qApp->quit();
    //});

    return a.exec();
}

#else

#include "qmlrender.h"
#include <QQmlApplicationEngine>
#include <qquickview.h>
QQuickView *view = nullptr;
int main(int argc, char *argv[])
{
    qputenv("QSG_RENDER_LOOP", "basic");
    QGuiApplication app(argc, argv);

    qmlRegisterType<QMLPlayer>("MDKPlayer", 1, 0, "MDKPlayer");

    //QTimer t;
    //QObject::connect(&t, &QTimer::timeout, [=]() {
    //    if (view) view->deleteLater();

    //    view = new QQuickView;
    //    view->setResizeMode(QQuickView::SizeRootObjectToView);
    //    view->setSource(QUrl("qrc:/qmdkqmlplay.qml"));
    //    view->show();
    //});
    //t.start(500);

    QQuickView v;
    v.setResizeMode(QQuickView::SizeRootObjectToView);
    v.setSource(QUrl("qrc:/qmdkqmlplay.qml"));
    v.show();

    return app.exec();
}

#endif
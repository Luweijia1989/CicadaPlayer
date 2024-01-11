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

    player->setMaskMode(
            IVideoRender::Mask_Right,
            u8"{\"[imgUser]\":\"C:/Users/posat/Desktop/big.jpeg\", \"[textUser]\":\"luweijia\", \"[textAnchor]\":\"rurongrong\"}");
    player->SetRotateMode(ROTATE_MODE_0);
    player->SetScaleMode(SM_FIT);

    //player->SetSpeed(0.5);
    player->SetDefaultBandWidth(1000 * 1000);
    player->SetDataSource("http://player.alicdn.com/video/aliyunmedia.mp4");
    //player->SetDataSource("E:\\vap1.mp4");
    //player->SetDataSource("E:\\test.mp4");
    player->EnableHardwareDecoder(true);
    player->SetAutoPlay(true);
    player->SetLoop(true);
    player->SetIPResolveType(IpResolveWhatEver);
    player->SetFastStart(true);
    MediaPlayerConfig config = *(player->GetConfig());
    config.mMaxBackwardBufferDuration = 20000;
    config.liveStartIndex = -3;
    player->SetConfig(&config);
    //player->Prepare();
    player->SelectTrack(-1);

    QPushButton btn("new one");
    QObject::connect(&btn, &QPushButton::clicked, [=]() {
        OpenGLWidget *w = new OpenGLWidget(player);
        w->resize(640, 480);
        w->show();
    });
    btn.show();

    QPushButton btn1("play");
    QObject::connect(&btn1, &QPushButton::clicked, [=]() { player->Prepare(); });
    btn1.show();


    auto player1 = std::shared_ptr<MediaPlayer>(new MediaPlayer());
    player1->SetDataSource("C:\\Users\\posat\\Desktop\\vap1.mp4");
    player1->EnableHardwareDecoder(true);
    player1->SetAutoPlay(true);
    player1->SetLoop(true);

    QPushButton btn2("play2");
    QObject::connect(&btn2, &QPushButton::clicked, [=]() { player1->Prepare(); });
    btn2.show();

    QPushButton btn3("new one2");
    QObject::connect(&btn3, &QPushButton::clicked, [=]() {
        OpenGLWidget *w = new OpenGLWidget(player1);
        w->resize(640, 480);
        w->show();
    });
    btn3.show();

    QPushButton btn4("quit");
    QObject::connect(&btn4, &QPushButton::clicked, [=]() { qApp->quit(); });
    btn4.show();

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
#include "DiyQmlRender.h"
#include <QQmlApplicationEngine>
#include <QQuickWidget>
#include <qquickview.h>
QQuickView *view = nullptr;
int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    //qputenv("QSG_RENDER_LOOP", "basic");
    QApplication app(argc, argv);

    qputenv("QT_DEBUG_PLUGINS", "1");
    qputenv("QML_IMPORT_TRACE", "1");

    qmlRegisterType<QMLPlayer>("MDKPlayer", 1, 0, "MDKPlayer");
    qmlRegisterType<SimpleQMLPlayer>("SimplePlayer", 1, 0, "SimplePlayer");
    qmlRegisterType<DiyQMLPlayer>("DiyPlayer", 1, 0, "DiyPlayer");

    //QTimer t;
    //QObject::connect(&t, &QTimer::timeout, [=]() {
    //    if (view) view->deleteLater();

    //    view = new QQuickView;
    //    view->setResizeMode(QQuickView::SizeRootObjectToView);
    //    view->setSource(QUrl("qrc:/qmdkqmlplay.qml"));
    //    view->show();
    //});
    //t.start(300);

    QQuickWidget m_renderWidget;
    //m_renderWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::WindowStaysOnTopHint | Qt::ToolTip);
    m_renderWidget.setAttribute(Qt::WA_TranslucentBackground);//…Ë÷√±≥æ∞Õ∏√˜
    m_renderWidget.setAttribute(Qt::WA_ShowWithoutActivating);
    m_renderWidget.setClearColor(Qt::transparent);
    m_renderWidget.setResizeMode(QQuickWidget::SizeRootObjectToView);

    m_renderWidget.setSource(QUrl("qrc:/qmdkqmlplay.qml"));
    m_renderWidget.show();
    /*QQuickView v;
    v.setResizeMode(QQuickView::SizeRootObjectToView);
    v.setSource(QUrl("qrc:/qmdkqmlplay.qml"));
    v.show();*/

    return app.exec();
}

#endif
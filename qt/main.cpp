#include "glwidget.h"
#include <QApplication>
#include <QTimer>

using namespace Cicada;
using namespace std;

OpenGLWidget *gl = nullptr;
int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);
 //   QTimer t;
 //   QObject::connect(&t, &QTimer::timeout, []() {
 //       if (gl) {
	//		gl->deleteLater();
	//		//Sleep(1000);
 //       }

 //       gl = new OpenGLWidget;
 //       gl->resize(640, 480);
 //       gl->show();
 //   });
	//t.start(200);
	//t.setSingleShot(true);

	OpenGLWidget gl;
	gl.resize(640, 480);
	gl.show();

    return a.exec();
}

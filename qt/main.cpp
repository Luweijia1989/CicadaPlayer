#include <QApplication>
#include "glwidget.h"

using namespace Cicada;
using namespace std;

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);

	OpenGLWidget gl;
	gl.resize(640, 480);
	gl.show();

	return a.exec();
}

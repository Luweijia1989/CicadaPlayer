#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

class QOpenGLVertexArrayObject;
class QOpenGLBuffer;
class QOpenGLShader;
class QOpenGLShaderProgram;
class QOpenGLFramebufferObject;
class QOpenGLTexture;

namespace Cicada {
    class MediaPlayer;
}
class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
public:
    OpenGLWidget(const std::shared_ptr<Cicada::MediaPlayer> &p);
    ~OpenGLWidget();

protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();

private:
    std::shared_ptr<Cicada::MediaPlayer> player;
};

#endif// OPENGLWIDGET_H

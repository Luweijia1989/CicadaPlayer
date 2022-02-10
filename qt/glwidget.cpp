#include "glwidget.h"
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

OpenGLWidget::OpenGLWidget()
{}

void OpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    
}

void OpenGLWidget::resizeGL(int w, int h)
{
    
}

void OpenGLWidget::paintGL()
{
   
}

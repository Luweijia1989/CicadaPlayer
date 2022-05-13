#pragma once

#include <QOpenGLFramebufferObject>
#include <QQuickFramebufferObject>
#include <memory>

#include "MediaPlayer.h"

using namespace Cicada;

class QMLPlayer : public QQuickFramebufferObject {
    Q_OBJECT
public:
    QMLPlayer(QQuickItem *parent = nullptr);
    ~QMLPlayer();

    QQuickFramebufferObject::Renderer *createRenderer() const;

	Q_INVOKABLE void test();

private:
    QString m_source;
    std::shared_ptr<MediaPlayer> internal_player;
};
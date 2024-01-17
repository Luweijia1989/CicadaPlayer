#pragma once
#include <QQuickFramebufferObject>
//#include <SimpleEffectPlayer.h>
#include <DiyEffectPlayer.h>

class DiyQMLPlayer : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(QString sourceTag READ sourceTag WRITE setSourceTag NOTIFY sourceTagChanged)
    Q_PROPERTY(QString sourceUrl READ sourceUrl WRITE setSourceUrl NOTIFY sourceUrlChanged)
    Q_PROPERTY(QVariantList sourceList READ sourceList WRITE setSourceList NOTIFY sourceListChanged)
public:
    DiyQMLPlayer(QQuickItem *parent = nullptr);
    ~DiyQMLPlayer();

    QQuickFramebufferObject::Renderer *createRenderer() const;

    Q_INVOKABLE void play1();
    Q_INVOKABLE void play2();
    Q_INVOKABLE void play3(QString path);
    Q_INVOKABLE void play4(QVariantList urlList);
    Q_INVOKABLE void stop();

public:
    QString sourceTag();
    void setSourceTag(const QString &tag);

    QString sourceUrl();
    void setSourceUrl(const QString &url);

	QVariantList sourceList();
    void setSourceList(const QVariantList &urlList);

signals:
    void sourceTagChanged();
    void sourceUrlChanged();
    void sourceListChanged();

protected:
    static void onVideoSize(int64_t width, int64_t height, void *userData);
    static void onEOS(void *userData);
    static void onFirstFrame(void *userData);

signals:
    void videoRatioChanged(qreal ratio);
    void ended();
    void firstFrame();

private:
    std::shared_ptr<DiyEffectPlayer> internal_player;

    QString m_sourceUrl = "";
    QString m_sourceTag = "unknown";
    QVariantList m_sourceList;
};
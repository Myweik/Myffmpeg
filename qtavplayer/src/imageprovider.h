#ifndef IMAGEPROVIDER_H
#define IMAGEPROVIDER_H
#include <QQuickImageProvider>
#include<QImage>
#include<QTcpSocket>
#include <QTcpServer>
#include<QBuffer>

class ImageProvider : public QQuickImageProvider
{
public:
    ImageProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);
    QImage img;
};
class ShowImage : public QObject
{
    Q_OBJECT
public:
    explicit ShowImage(QObject *parent = 0);
    ImageProvider *m_pImgProvider;
public slots:
    void sendimage(QImage);
signals:
    void callQmlRefeshImg();
    void sendPic(QImage image);
};

#endif // IMAGEPROVIDER_H

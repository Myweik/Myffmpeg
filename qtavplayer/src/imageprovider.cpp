#include "imageprovider.h"


ImageProvider::ImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{

}

QImage ImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    return this->img;
}

QPixmap ImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    return QPixmap::fromImage(this->img);
}

ShowImage::ShowImage(QObject *parent) :
    QObject(parent)
{
    m_pImgProvider = new ImageProvider();
}

void  ShowImage::sendimage(QImage sendimage)
{
      m_pImgProvider->img =sendimage ;
      emit callQmlRefeshImg();
}

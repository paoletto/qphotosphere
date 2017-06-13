#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QDebug>
#include "qphotosphere.h"

QT_BEGIN_NAMESPACE


class PhotoSphere : public QQmlExtensionPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "PhotoSphere")

public:
    PhotoSphere(QObject *parent = 0)
    :   QQmlExtensionPlugin(parent)
    {
    }

    void registerTypes(const char *uri)
    {
        Q_ASSERT(uri == QLatin1String("PhotoSphere"));
        qmlRegisterType<QmlPhotoSphere>(uri, 1, 0, "PhotoSphere");
    }
};

QT_END_NAMESPACE

#include "plugin.moc"

/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtPositioning module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qphotosphere.h"
#include <QtQuick/qsgrendernode.h>
#include <QtGui/qopenglshaderprogram.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qopenglextrafunctions.h>
#include <QtGui/QOpenGLVertexArrayObject>
#include <QtGui/QOpenGLTexture>
#include <QDebug>
#include <cmath>
#include <GL/glu.h>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QScopedPointer>
#include <QEventLoop>
#include "TinyEXIF/TinyEXIF.h"

static double sphereRadius = 50.0;

static QByteArray versionedShaderCode(const char *src)
{
    QByteArray versionedSrc;
    versionedSrc.append(QByteArrayLiteral("#version 330 compatibility\n"));
    versionedSrc.append(src);
    return versionedSrc;
}

class QmlPhotoSphereRenderNode : public QSGRenderNode
{
public:
    QmlPhotoSphereRenderNode(QmlPhotoSphere *sphere);
    ~QmlPhotoSphereRenderNode();

    void render(const RenderState *state) Q_DECL_OVERRIDE;
    void releaseResources() Q_DECL_OVERRIDE;
    StateFlags changedStates() const Q_DECL_OVERRIDE;
    RenderingFlags flags() const Q_DECL_OVERRIDE;
    QRectF rect() const Q_DECL_OVERRIDE;

private:
    QmlPhotoSphere *m_quickPhotoSphere;
    QOpenGLVertexArrayObject *m_vao;
    QOpenGLShaderProgram *m_shader;
    GLUquadric *m_quadric;
    QOpenGLTexture *m_texPhotoSphere;
};

QmlPhotoSphereRenderNode::QmlPhotoSphereRenderNode(QmlPhotoSphere *sphere)
: m_quickPhotoSphere(sphere), m_vao(0), m_shader(0), m_quadric(0), m_texPhotoSphere(0)
{

}

QmlPhotoSphereRenderNode::~QmlPhotoSphereRenderNode()
{

}

void QmlPhotoSphereRenderNode::releaseResources()
{
    if (m_shader)
        delete m_shader;
    m_shader = 0;

    if (m_vao)
        delete m_vao;
    m_vao = 0;

    if (m_quadric)
        delete m_quadric;
    m_quadric = 0;

    if (m_texPhotoSphere)
        delete m_texPhotoSphere;
    m_texPhotoSphere = 0;
}

void QmlPhotoSphereRenderNode::render(const QSGRenderNode::RenderState *state)
{
    QMatrix4x4 matRotateX;
    matRotateX.rotate(90, 1, 0 ,0);

    QMatrix4x4 matProjection;
    matProjection.perspective(m_quickPhotoSphere->fieldOfView(), m_quickPhotoSphere->width() / m_quickPhotoSphere->height(), 0.001, 200);

    QMatrix4x4 matQuickize;
    //matQuickize.scale(0.5);
    matQuickize.translate(sphereRadius,sphereRadius,0);

    QMatrix4x4 matViewFromOutside;
    matViewFromOutside.translate(0,0,-1.5 * sphereRadius);

    QMatrix4x4 matAzimuth;
    matAzimuth.rotate(m_quickPhotoSphere->azimuth(), 0, -1, 0);
//    QVector3D xaxis(-1, 0, 0);
//    xaxis = matScroll * xaxis;
    QMatrix4x4 matElevation;
    matElevation.rotate(m_quickPhotoSphere->elevation(), -1, 0, 0);



    //qDebug() << xaxis;

    QMatrix4x4 transformation =
            QMatrix4x4()
            //* *state->projectionMatrix()
            //* *matrix()

            * matProjection
            //* matQuickize
            * matElevation
            * matAzimuth
            * matRotateX
            //* matViewFromOutside

            ;

    if (!m_vao) {
        m_vao = new QOpenGLVertexArrayObject;
        m_vao->create();

        m_quadric = gluNewQuadric();
        gluQuadricOrientation(m_quadric,GLU_INSIDE);
        gluQuadricOrientation(m_quadric,GLU_OUTSIDE);
        gluQuadricNormals(m_quadric, GLU_SMOOTH);
        gluQuadricDrawStyle(m_quadric, GLU_FILL); // GLU_FILL, GLU_LINE, GLU_POINT, GLU_SILHOUETTE
        gluQuadricTexture(m_quadric, GL_TRUE);

        // Create shaders
        static const char *vertexShaderSource =
        "attribute highp vec4 vCoord;\n"
        "uniform highp mat4 matrix;\n"
        "out highp vec3 texCoord;\n"
        "void main()\n"
        "{\n"
        "    texCoord = gl_MultiTexCoord0.xyz;\n"
        "    gl_Position = matrix * gl_Vertex; //vCoord;\n"
        "}\n"
        "\n";

        static const char *fragmentShaderSource =
        "uniform lowp vec4 color;\n"
        "in highp vec3 texCoord;\n"
        "uniform sampler2D samImage; \n"
        "void main()\n"
        "{\n"
        "    lowp vec4 texColor = texture(samImage, texCoord.xy);\n"
        "    //gl_FragColor = vec4(texCoord.xy, 0 , 1); //color; \n"
        "    gl_FragColor = vec4(texColor.rgb, 1.0); //color; \n"
        "}\n"
        "\n";


        m_shader = new QOpenGLShaderProgram;
        m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex, versionedShaderCode(vertexShaderSource));
        m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment, versionedShaderCode(fragmentShaderSource));
        m_shader->bindAttributeLocation("vCoord", 0);
        m_shader->link();

        m_texPhotoSphere = new QOpenGLTexture(QOpenGLTexture::Target2D);
    }

    if (m_quickPhotoSphere->m_texDirty) {
        m_texPhotoSphere->setData(m_quickPhotoSphere->m_image);
        m_quickPhotoSphere->m_texDirty = false;
    }

    bool texturing = false;
    if (m_texPhotoSphere->width())
        texturing  = true;

    QOpenGLExtraFunctions *f = QOpenGLContext::currentContext()->extraFunctions();
    f->glEnable(GL_DEPTH_TEST);
    f->glDepthFunc(GL_LESS);
    f->glDepthMask(true);

    if (texturing)
        m_texPhotoSphere->bind(0);

    m_shader->bind();
    m_shader->setUniformValue("matrix", transformation);
    m_shader->setUniformValue("color", QColor(255,0,0));
    m_shader->setUniformValue("samImage", 0);
    m_vao->bind();

    gluSphere(m_quadric, sphereRadius /* radius */, 50 /* slices */, 50 /* stacks*/);

    m_vao->release();
    m_shader->release();
    m_texPhotoSphere->release(0);
}

QSGRenderNode::StateFlags QmlPhotoSphereRenderNode::changedStates() const
{
    return BlendState
          | DepthState
          | StencilState
          | ScissorState
          | ColorState
          | CullState
          | ViewportState
          | RenderTargetState;
}

QSGRenderNode::RenderingFlags QmlPhotoSphereRenderNode::flags() const
{
    return
//            BoundedRectRendering
            DepthAwareRendering
//            | OpaqueRendering
            ;
}

QRectF QmlPhotoSphereRenderNode::rect() const
{
    return QRectF(0, 0, m_quickPhotoSphere->width() , m_quickPhotoSphere->height());
}



/*
 * QmlPhotoSphere
 */

QmlPhotoSphere::QmlPhotoSphere(QQuickItem *parent)
:   QQuickItem(parent), m_azimuth(0), m_elevation(0), m_fieldOfView(90), m_texDirty(false)
{
    setFlag(ItemHasContents);
}

QmlPhotoSphere::~QmlPhotoSphere()
{

}

qreal QmlPhotoSphere::azimuth() const
{
    return m_azimuth;
}

void QmlPhotoSphere::setAzimuth(qreal azimuth)
{
    azimuth = std::fmod(azimuth, qreal(360.0));
    if (azimuth < 0.0)
        azimuth += 360.0;

    if (azimuth == m_azimuth)
        return;

    m_azimuth = azimuth;
    updateSphere();
    emit azimuthChanged(azimuth);
}

qreal QmlPhotoSphere::elevation() const
{
    return m_elevation;
}

void QmlPhotoSphere::setElevation(qreal elevation)
{
    if (elevation == m_elevation || elevation >= 90 || elevation <= -90)
        return;

    m_elevation = elevation;
    updateSphere();
    emit elevationChanged(elevation);
}

qreal QmlPhotoSphere::fieldOfView() const
{
    return m_fieldOfView;
}

void QmlPhotoSphere::setFieldOfView(qreal fov)
{
    if (fov == m_fieldOfView || fov < 1.0 || fov > 179.0)
        return;

    m_fieldOfView = fov;
    updateSphere();
    emit fieldOfViewChanged(fov);
}

QString QmlPhotoSphere::imageUrl() const
{
    return m_imageUrl;
}

static QByteArray fetchUrl(const QUrl &url)
{
    QNetworkRequest request;
    request.setUrl(url);
    QEventLoop syncLoop;
    QScopedPointer<QNetworkAccessManager> nam(new QNetworkAccessManager);

    QNetworkDiskCache *diskCache = new QNetworkDiskCache();
    QString providersCacheDir = QLatin1String("/tmp/photospherefetcher/");
    diskCache->setCacheDirectory(providersCacheDir);
    nam->setCache(diskCache);

    QNetworkReply *reply = nam->get(request);
    if (!reply->isFinished()) {
        QObject::connect(reply, SIGNAL(finished()), &syncLoop, SLOT(quit()));
        syncLoop.exec();

        if (!reply->isFinished()) {
            // This shouldn't happen
            qWarning() << "Unfinished reply";
            return QByteArray();
        }
    }
    reply->deleteLater();
    return reply->readAll();
}

void QmlPhotoSphere::setImageUrl(const QString &url)
{
    if (url == m_imageUrl || url.isEmpty())
        return;

    QUrl u(url);
    if (!u.isValid())
        return;

    emit imageUrlChanged(url);

    /*
        Load the image
    */

    QByteArray data = fetchUrl(u);
    QImage image;
    if (!image.loadFromData(data))
        return;

    if (image.isNull())
        return;

    m_image = image;
    m_texDirty = true;

    updateSphere();
}



bool QmlPhotoSphere::contains(const QPointF &point) const
{
    qDebug() << "QPhotoSphere contains called";
    return QQuickItem::contains(point);
}

QSGNode *QmlPhotoSphere::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *data)
{
    Q_UNUSED(data);

    if (width() <= 0 || height() <= 0) {
        delete oldNode;
        oldNode = 0;
        return 0;
    }

    QmlPhotoSphereRenderNode *photoSphereNode = static_cast<QmlPhotoSphereRenderNode *>(oldNode);
    if (!photoSphereNode)
        photoSphereNode = new QmlPhotoSphereRenderNode(this);

    return photoSphereNode;
}

void QmlPhotoSphere::updateSphere()
{
    qDebug() << "Updating frikkin Sphere";
    setFlag(ItemHasContents);
    polish();
    update();
}



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


#ifndef QPHOTOSPHERE_H
#define QPHOTOSPHERE_H

#include <QQuickItem>
#include <QImage>

class QmlPhotoSphere : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(qreal azimuth READ azimuth WRITE setAzimuth NOTIFY azimuthChanged)
    Q_PROPERTY(qreal elevation READ elevation WRITE setElevation NOTIFY elevationChanged)
    Q_PROPERTY(qreal fieldOfView READ fieldOfView WRITE setFieldOfView NOTIFY fieldOfViewChanged)
    Q_PROPERTY(QString imageUrl READ imageUrl WRITE setImageUrl NOTIFY imageUrlChanged)
public:
    QmlPhotoSphere(QQuickItem *parent = 0);
    ~QmlPhotoSphere();

    qreal azimuth() const;
    void setAzimuth(qreal azimuth);

    qreal elevation() const;
    void setElevation(qreal elevation);

    qreal fieldOfView() const;
    void setFieldOfView(qreal fov);

    QString imageUrl() const;
    void setImageUrl(const QString &url);

    bool contains(const QPointF &point) const Q_DECL_OVERRIDE;

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) Q_DECL_OVERRIDE;
    void updateSphere();

signals:
    void azimuthChanged(qreal azimuth);
    void elevationChanged(qreal elevation);
    void fieldOfViewChanged(qreal fov);
    void imageUrlChanged(const QString &url);

public slots:

private:
    qreal m_azimuth;
    qreal m_elevation;
    qreal m_fieldOfView;
    bool m_texDirty;
    QImage m_image;
    QString m_imageUrl;

    friend class QmlPhotoSphereRenderNode;
    Q_DISABLE_COPY(QmlPhotoSphere)
};

#endif // QPHOTOSPHERE_H

/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "QSGImageTexture.h"
#include <qmath.h>
#include <QOpenGLFunctions>

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

QSGImageTexture::QSGImageTexture()
	: QSGTexture()
	, m_texture_id(0)
	, m_has_alpha(false)
	, m_dirty_texture(false)
	, m_dirty_bind_options(false)
	, m_owns_texture(true)
{
}


QSGImageTexture::~QSGImageTexture()
{
	if (m_texture_id && m_owns_texture && QOpenGLContext::currentContext())
		QOpenGLContext::currentContext()->functions()->glDeleteTextures(1, &m_texture_id);
}

void QSGImageTexture::setImage(const QImage &image)
{
	m_image = image;
	m_texture_size = image.size();
	m_has_alpha = image.hasAlphaChannel();
	m_dirty_texture = true;
	m_dirty_bind_options = true;
 }

int QSGImageTexture::textureId() const
{
	if (m_dirty_texture) {
		if (m_image.isNull()) {
			// The actual texture and id will be updated/deleted in a later bind()
			// or ~QSGImageTexture so just keep it minimal here.
			return 0;
		} else if (m_texture_id == 0){
			// Generate a texture id for use later and return it.
			QOpenGLContext::currentContext()->functions()->glGenTextures(1, &const_cast<QSGImageTexture *>(this)->m_texture_id);
			return int(m_texture_id);
		}
	}
	return int(m_texture_id);
}

void QSGImageTexture::bind()
{
	QOpenGLContext *context = QOpenGLContext::currentContext();
	QOpenGLFunctions *funcs = context->functions();
	if (!m_dirty_texture) {
		funcs->glBindTexture(GL_TEXTURE_2D, m_texture_id);
		updateBindOptions(m_dirty_bind_options);
		m_dirty_bind_options = false;
		return;
	}

	m_dirty_texture = false;

	if (m_image.isNull()) {
		if (m_texture_id && m_owns_texture) {
			funcs->glDeleteTextures(1, &m_texture_id);
		}
		m_texture_id = 0;
		m_texture_size = QSize();
		m_has_alpha = false;

		return;
	}

	if (m_texture_id == 0)
		funcs->glGenTextures(1, &m_texture_id);
	funcs->glBindTexture(GL_TEXTURE_2D, m_texture_id);

	QImage tmp = m_image/*(m_image.format() == QImage::Format_RGB32 || m_image.format() == QImage::Format_ARGB32_Premultiplied)
				 ? m_image
				 : m_image.convertToFormat(QImage::Format_ARGB32_Premultiplied);*/;

	int max;
	funcs->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);
	if (tmp.width() > max || tmp.height() > max) {
		tmp = tmp.scaled(qMin(max, tmp.width()), qMin(max, tmp.height()), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		m_texture_size = tmp.size();
	}

	if (tmp.width() * 4 != tmp.bytesPerLine())
		tmp = tmp.copy();

	updateBindOptions(m_dirty_bind_options);

	GLenum externalFormat = GL_RGBA;
	GLenum internalFormat = GL_RGBA;

#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
	QString *deviceName =
			static_cast<QString *>(QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("AndroidDeviceName"));
	static bool wrongfullyReportsBgra8888Support = deviceName != 0
													&& (deviceName->compare(QLatin1String("samsung SM-T211"), Qt::CaseInsensitive) == 0
														|| deviceName->compare(QLatin1String("samsung SM-T210"), Qt::CaseInsensitive) == 0
														|| deviceName->compare(QLatin1String("samsung SM-T215"), Qt::CaseInsensitive) == 0);
#else
	static bool wrongfullyReportsBgra8888Support = false;
#endif

	if (context->hasExtension(QByteArrayLiteral("GL_EXT_bgra"))) {
		externalFormat = GL_BGRA;
#ifdef QT_OPENGL_ES
		internalFormat = GL_BGRA;
#else
		if (context->isOpenGLES())
			internalFormat = GL_BGRA;
#endif // QT_OPENGL_ES
	} else if (!wrongfullyReportsBgra8888Support
			   && (context->hasExtension(QByteArrayLiteral("GL_EXT_texture_format_BGRA8888"))
				   || context->hasExtension(QByteArrayLiteral("GL_IMG_texture_format_BGRA8888")))) {
		externalFormat = GL_BGRA;
		internalFormat = GL_BGRA;
#if defined(Q_OS_DARWIN) && !defined(Q_OS_OSX)
	} else if (context->hasExtension(QByteArrayLiteral("GL_APPLE_texture_format_BGRA8888"))) {
		externalFormat = GL_BGRA;
		internalFormat = GL_RGBA;
#endif
	} else {
		tmp = std::move(tmp).convertToFormat(QImage::Format_RGBA8888_Premultiplied);
	}

	funcs->glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_texture_size.width(), m_texture_size.height(), 0, externalFormat, GL_UNSIGNED_BYTE, tmp.constBits());

	m_dirty_bind_options = false;
	m_image = {};
}

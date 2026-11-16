// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QElapsedTimer>
#include <QtConcurrent>

#include "ComputerControlInterface.h"

class ComputerControlInterface;
class QTimer;

class WebApiConnection
{
public:
	WebApiConnection(const QString& hostAddress);
	~WebApiConnection();

	ComputerControlInterface::Pointer controlInterface() const
	{
		return m_controlInterface;
	}

	void lock()
	{
		m_controlInterface->lock();
	}

	void unlock()
	{
		m_controlInterface->unlock();
	}

	QTimer* idleTimer() const
	{
		return m_idleTimer;
	}

	QTimer* lifetimeTimer() const
	{
		return m_lifetimeTimer;
	}

	QSize scaledFramebufferSize(int width, int height) const;

	QByteArray encodedFramebufferData(QSize size, const QByteArray& format, int compression, int quality);

	const QString& framebufferEncodingError() const
	{
		return m_encodingError;
	}

private:
	void runFramebufferEncoder();

	ComputerControlInterface::Pointer m_controlInterface;
	QTimer* m_idleTimer{nullptr};
	QTimer* m_lifetimeTimer{nullptr};

	QByteArray m_imageFormat{};
	int m_imageQuality{0};
	int m_imageCompression{0};
	QSize m_imageSize{};

	struct EncodingResult
	{
		QByteArray imageData{};
		QString errorString{};
		qint64 timestamp{QDateTime::currentMSecsSinceEpoch()};

		bool expired() const
		{
			static constexpr auto MaxAge = 1000;

			return QDateTime::currentMSecsSinceEpoch() - timestamp > MaxAge;
		}
	};

	static constexpr auto MinimumPreencodeInterval = 10;

	QFuture<EncodingResult> m_framebufferEncoder;
	QString m_encodingError;

	QElapsedTimer m_lastFramebufferRequestTimer;
	qint64 m_lastFramebufferRequestInterval{0};
	QAtomicInteger<qint64> m_framebufferEncodingTime{0};
};

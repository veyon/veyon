// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDate>
#include <QImage>

#include "ComputerControlInterface.h"
#include "VeyonCore.h"

class ComputerControlInterface;

class VEYON_CORE_EXPORT Screenshot : public QObject
{
	Q_OBJECT
public:
	enum class MetaData {
		User,
		Host,
		Date,
		Time
	};
	Q_ENUM(MetaData)

	explicit Screenshot(const QString& fileName = {}, QObject* parent = nullptr);

	void take(const ComputerControlInterface::Pointer& computerControlInterface);

	bool isValid() const
	{
		return !fileName().isEmpty() && !image().isNull();
	}

	const QString& fileName() const
	{
		return m_fileName;
	}

	void setImage(const QImage& image)
	{
		m_image = image;
	}

	const QImage& image() const
	{
		return m_image;
	}

	static QString constructFileName(const QString& user, const QString& hostAddress,
									 QDate date = QDate::currentDate(),
									 QTime time = QTime::currentTime());

	QString user() const;
	QString host() const;
	QString date() const;
	QString time() const;

	static QString metaDataKey(MetaData key);

private:
	QString property(const QString& key, int section) const;
	QString fileNameSection(int n) const;

	QString m_fileName;
	QImage m_image;
};

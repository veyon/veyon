/*
 * FileCollection.h - declaration of FileCollection class
 *
 * Copyright (c) 2025-2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include <QFile>

struct FileCollection {
	enum class State {
		Invalid,
		Initializing,
		ReadyForNextFileTransfer,
		FileTransferRunning,
		Finished
	};

	using Id = QUuid;
	using TransferId = QUuid;

	Id id = QUuid::createUuid();

	State state = State::Invalid;

	QString name = {};
	QStringList files = {};
	int processedFilesCount = 0;

	TransferId currentTransferId = {};
	QFile* currentOutputFile = nullptr;
	qint64 currentFileSize = 0;

	bool operator!=(const FileCollection& other) const
	{
		return other.id != id;
	}

	int currentFileProgress() const
	{
		return currentOutputFile && currentFileSize > 0 ?
					(currentOutputFile->size() * 100 / currentFileSize)
				  :
					0;
	}

	int progress() const
	{
		if (files.isEmpty())
		{
			return 0;
		}
		if (processedFilesCount >= files.size())
		{
			return 100;
		}
		return processedFilesCount * 100 / files.size() +
				currentFileProgress() / files.size();
	}
};

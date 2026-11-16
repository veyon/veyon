// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDataStream>

#include "VeyonCore.h"

class VEYON_CORE_EXPORT VariantStream
{
public:
	static constexpr auto MaxByteArraySize = 16 * 1024 * 1024;
	static constexpr auto MaxStringSize = 64 * 1024;
	static constexpr auto MaxContainerSize = 1024;
	static constexpr auto MaxCheckRecursionDepth = 3;

	explicit VariantStream(QIODevice* ioDevice);

	QVariant read(); // Flawfinder: ignore

	void write(const QVariant& v);

private:
	bool checkBool();
	bool checkByteArray();
	bool checkInt();
	bool checkLong();
	bool checkRect();
	bool checkString();
	bool checkStringList();
	bool checkUuid();
	bool checkVariant(int depth);
	bool checkVariantList(int depth);
	bool checkVariantMap(int depth);

	QDataStream m_dataStream;
};

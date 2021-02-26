/*
 * QtCompat.cpp - functions and templates for compatibility with older Qt versions
 *
 * Copyright (c) 2018-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "QtCompat.h"

#if QT_VERSION < 0x050600

// taken from qtbase/src/corelib/tools/qversionnumber.cpp

QVector<int> QVersionNumber::segments() const
{
	if (m_segments.isUsingPointer())
		return *m_segments.pointer_segments;

	QVector<int> result;
	result.resize(segmentCount());
	for (int i = 0; i < segmentCount(); ++i)
		result[i] = segmentAt(i);
	return result;
}

QVersionNumber QVersionNumber::normalized() const
{
	int i;
	for (i = m_segments.size(); i; --i)
		if (m_segments.at(i - 1) != 0)
			break;

	QVersionNumber result(*this);
	result.m_segments.resize(i);
	return result;
}

bool QVersionNumber::isPrefixOf(const QVersionNumber &other) const Q_DECL_NOTHROW
{
	if (segmentCount() > other.segmentCount())
		return false;
	for (int i = 0; i < segmentCount(); ++i) {
		if (segmentAt(i) != other.segmentAt(i))
			return false;
	}
	return true;
}

int QVersionNumber::compare(const QVersionNumber &v1, const QVersionNumber &v2) Q_DECL_NOTHROW
{
	int commonlen;

	if (Q_LIKELY(!v1.m_segments.isUsingPointer() && !v2.m_segments.isUsingPointer())) {
		// we can't use memcmp because it interprets the data as unsigned bytes
		const qint8 *ptr1 = v1.m_segments.inline_segments + InlineSegmentStartIdx;
		const qint8 *ptr2 = v2.m_segments.inline_segments + InlineSegmentStartIdx;
		commonlen = qMin(v1.m_segments.size(),
						 v2.m_segments.size());
		for (int i = 0; i < commonlen; ++i)
			if (int x = ptr1[i] - ptr2[i])
				return x;
	} else {
		commonlen = qMin(v1.segmentCount(), v2.segmentCount());
		for (int i = 0; i < commonlen; ++i) {
			if (v1.segmentAt(i) != v2.segmentAt(i))
				return v1.segmentAt(i) - v2.segmentAt(i);
		}
	}

	// ran out of segments in v1 and/or v2 and need to check the first trailing
	// segment to finish the compare
	if (v1.segmentCount() > commonlen) {
		// v1 is longer
		if (v1.segmentAt(commonlen) != 0)
			return v1.segmentAt(commonlen);
		else
			return 1;
	} else if (v2.segmentCount() > commonlen) {
		// v2 is longer
		if (v2.segmentAt(commonlen) != 0)
			return -v2.segmentAt(commonlen);
		else
			return -1;
	}

	// the two version numbers are the same
	return 0;
}

QVersionNumber QVersionNumber::commonPrefix(const QVersionNumber &v1,
											const QVersionNumber &v2)
{
	int commonlen = qMin(v1.segmentCount(), v2.segmentCount());
	int i;
	for (i = 0; i < commonlen; ++i) {
		if (v1.segmentAt(i) != v2.segmentAt(i))
			break;
	}

	if (i == 0)
		return QVersionNumber();

	// try to use the one with inline segments, if there's one
	QVersionNumber result(!v1.m_segments.isUsingPointer() ? v1 : v2);
	result.m_segments.resize(i);
	return result;
}

QString QVersionNumber::toString() const
{
	QString version;
	version.reserve(qMax(segmentCount() * 2 - 1, 0));
	bool first = true;
	for (int i = 0; i < segmentCount(); ++i) {
		if (!first)
			version += QLatin1Char('.');
		version += QString::number(segmentAt(i));
		first = false;
	}
	return version;
}

QVersionNumber QVersionNumber::fromString(const QString &string, int *suffixIndex)
{
	QVector<int> seg;

	const QByteArray cString(string.toLatin1());

	const char *start = cString.constData();
	const char *end = start;
	const char *lastGoodEnd = start;
	const char *endOfString = cString.constData() + cString.size();

	do {
		const qulonglong value = strtoull(start, (char **) &end, 10);
		if (value > qulonglong(std::numeric_limits<int>::max()))
			break;
		seg.append(int(value));
		start = end + 1;
		lastGoodEnd = end;
	} while (start < endOfString && (end < endOfString && *end == '.'));

	if (suffixIndex)
		*suffixIndex = int(lastGoodEnd - cString.constData());

	return QVersionNumber(qMove(seg));
}

#endif

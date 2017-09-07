/*
 * VncKeyMapper.h - VNC key mapper
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef VNC_KEY_MAPPER_H
#define VNC_KEY_MAPPER_H

#include "VeyonCore.h"

class VEYON_CORE_EXPORT VncKeyMapper
{
public:
	typedef uint32_t XKey;
	typedef uint16_t UnicodeKey;

	VncKeyMapper();

	bool isModifier( Qt::Key qtKey ) const;
	XKey qtToXKey( Qt::Key qtKey ) const;
	XKey unicodeToXKey( UnicodeKey unicodeKey ) const;

private:
	void initMaps();

	const QVector<Qt::Key> qtModifierKeys;
	QMap<UnicodeKey, XKey> ucsLatinExtABMap;
	QMap<UnicodeKey, XKey> ucsGreek_CopticMap;
	QMap<UnicodeKey, XKey> ucsCyrillicMap;
	QMap<UnicodeKey, XKey> ucsArmenianMap;
	QMap<UnicodeKey, XKey> ucsHebrewMap;
	QMap<UnicodeKey, XKey> ucsArabicMap;
	QMap<UnicodeKey, XKey> ucsThaiMap;
	QMap<UnicodeKey, XKey> ucsGeorgianMap;
	QMap<UnicodeKey, XKey> ucsHangulJamoMap;
	QMap<UnicodeKey, XKey> ucsLatinExtAddMap;
	QMap<UnicodeKey, XKey> ucsGenPuncMap;
	QMap<UnicodeKey, XKey> ucsCurrSymMap;
	QMap<UnicodeKey, XKey> ucsCJKSym_PuncMap;
	QMap<UnicodeKey, XKey> ucsKatakanaMap;
	QMap<UnicodeKey, XKey> ucsOthersMap;
	QMap<XKey, UnicodeKey> ksLatinExtMap;
	QMap<XKey, UnicodeKey> ksKatakanaMap;
	QMap<XKey, UnicodeKey> ksArabicMap;
	QMap<XKey, UnicodeKey> ksCyrillicMap;
	QMap<XKey, UnicodeKey> ksGreekMap;
	QMap<XKey, UnicodeKey> ksTechnicalMap;
	QMap<XKey, UnicodeKey> ksSpecialMap;
	QMap<XKey, UnicodeKey> ksPublishingMap;
	QMap<XKey, UnicodeKey> ksHebrewMap;
	QMap<XKey, UnicodeKey> ksThaiMap;
	QMap<XKey, UnicodeKey> ksKoreanMap;
	QMap<XKey, UnicodeKey> ksArmenianMap;
	QMap<XKey, UnicodeKey> ksGeorgianMap;
	QMap<XKey, UnicodeKey> ksAzeriMap;
	QMap<XKey, UnicodeKey> ksVietnameseMap;
	QMap<XKey, UnicodeKey> ksCurrencyMap;

} ;

#endif

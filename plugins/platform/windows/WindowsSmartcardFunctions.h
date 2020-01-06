/*
 * WindowsSmartcardFunctions.h - header for Smartcard Functions on Windows
 *
 * Copyright (c) 2019 State of New Jersey
 * 
 * Maintained by Austin McGuire <austin.mcguire@jjc.nj.gov> 
 * 
 * This file is part of a fork of Veyon - https://veyon.io
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

#include "PlatformSmartcardFunctions.h"

// clazy:excludeall=copyable-polymorphic

class WindowsSmartcardFunctions: public PlatformSmartcardFunctions
{
public:
    QString upnToUsername( const QString& upn ) const;
    QMap<QString,QString> smartCardCertificateIds() const;
    bool smartCardAuthenticate( const QString& serializedEntry, const QString& pin ) const;
    bool smartCardVerifyPin( const QString& serializedEntry, const QString& pin ) const;
    QString certificatePem( const QString& serializedEntry) const;
    QByteArray signWithSmartCardKey(QByteArray data, QCA::SignatureAlgorithm alg,const QString& serializedEntry, const QString& pin ) const;
private:
    static QString upnToUsernameStatic( const QString& upn );
    static bool UpnToNT4W(LPCWSTR lpwUpnName, LPWSTR lpwNT4Name, LPDWORD cchNT4Name);
 
};
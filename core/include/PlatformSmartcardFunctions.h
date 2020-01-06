/*
 * PlatformSmartcardFunctions.h - interface class for platform plugins
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

#include "CryptoCore.h"
#include "PlatformPluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class PlatformSmartcardFunctions
{
public:
    virtual QString upnToUsername( const QString& upn ) const = 0;
    virtual QMap<QString,QString> smartCardCertificateIds() const = 0;
    virtual bool smartCardAuthenticate( const QString& serializedEntry, const QString& pin ) const = 0;
    virtual bool smartCardVerifyPin( const QString& serializedEntry, const QString& pin ) const = 0;
    virtual QString certificatePem( const QString& serializedEntry) const = 0;
    virtual QByteArray signWithSmartCardKey(QByteArray data, QCA::SignatureAlgorithm alg, const QString& serializedEntry, const QString& pin ) const = 0;

};
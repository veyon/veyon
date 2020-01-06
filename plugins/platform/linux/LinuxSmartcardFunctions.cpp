/*
 * LinuxSmartcardFunctions.cpp - header for Smartcard Functions on Linux
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

#include "LinuxSmartcardFunctions.h"
#include "VeyonCore.h"


QString LinuxSmartcardFunctions::upnToUsername( const QString& upn ) const{
    vDebug() << "_entry";
    QString username = upn;

    const auto nameParts = upn.split( QString::fromLatin1("@") );
    if( nameParts.size() > 1 )      
    {
        username = nameParts[0];
    }
    vDebug() << "return: " << username;
    return username;
}

QMap<QString,QString> LinuxSmartcardFunctions::smartCardCertificateIds() const{
    vDebug() << "_entry";
    vDebug() << "(NOT IMPLEMENTED) return empty QMAP ";
    return QMap<QString,QString>();
}

bool LinuxSmartcardFunctions::smartCardAuthenticate( const QString& serializedEntry, const QString& pin ) const{
    vDebug() << "_entry";
    vDebug() << "(NOT IMPLEMENTED) return false ";
    return false;
}

bool LinuxSmartcardFunctions::smartCardVerifyPin( const QString& serializedEntry, const QString& pin ) const{
    vDebug() << "_entry";
    vDebug() << "(NOT IMPLEMENTED) return false ";
    return false;
}

QString LinuxSmartcardFunctions::certificatePem( const QString& serializedEntry) const{
    vDebug() << "_entry";
    vDebug() << "(NOT IMPLEMENTED) return empty QString";
    return QString();
}

QByteArray LinuxSmartcardFunctions::signWithSmartCardKey(QByteArray data, QCA::SignatureAlgorithm alg, const QString& serializedEntry, const QString& pin ) const{
    vDebug() << "_entry";
    vDebug() << "(NOT IMPLEMENTED) return empty QByteArray";
    return QByteArray();
}
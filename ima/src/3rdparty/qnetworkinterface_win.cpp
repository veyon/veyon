/****************************************************************************
**
** Copyright (C) 1992-2006 Trolltech ASA. All rights reserved.
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qnetworkinterface.h"
#include "qnetworkinterface_p.h"
#include "qnetworkinterface_win_p.h"

typedef DWORD (WINAPI *PtrGetAdaptersInfo)(PIP_ADAPTER_INFO, PULONG);
PtrGetAdaptersInfo ptrGetAdaptersInfo = 0;
typedef ULONG (WINAPI *PtrGetAdaptersAddresses)(ULONG, ULONG, PVOID, PIP_ADAPTER_ADDRESSES, PULONG);
PtrGetAdaptersAddresses ptrGetAdaptersAddresses = 0;

static void resolveLibs()
{
    // try to find the functions we need from Iphlpapi.dll
    static bool done = false;

    if (!done) {
        done = true;

        HINSTANCE iphlpapiHnd;
        QT_WA({
                iphlpapiHnd = LoadLibraryW(L"iphlpapi");
            }, {
                iphlpapiHnd = LoadLibraryA("iphlpapi");
            });
        if (iphlpapiHnd == NULL)
            return;             // failed to load, probably Windows 95

        ptrGetAdaptersInfo = (PtrGetAdaptersInfo)GetProcAddress(iphlpapiHnd, "GetAdaptersInfo");
        ptrGetAdaptersAddresses = (PtrGetAdaptersAddresses)GetProcAddress(iphlpapiHnd, "GetAdaptersAddresses");
    }
}

static QHostAddress addressFromSockaddr(sockaddr *sa)
{
    QHostAddress address;
    if (!sa)
        return address;

    if (sa->sa_family == AF_INET)
        address.setAddress(htonl(((sockaddr_in *)sa)->sin_addr.s_addr));
    else if (sa->sa_family == AF_INET6)
        address.setAddress(((qt_sockaddr_in6 *)sa)->sin6_addr.qt_s6_addr);
    else
        qWarning("Got unknown socket family %d", sa->sa_family);
    return address;

}

static QHostAddress netmaskFromPrefixLength(uint len, int family)
{
    Q_ASSERT(len < 128);
    union {
        quint8 u8[16];
        quint32 ipv4;
    } data;

    // initialize data
    memset(&data, 0, sizeof data);

    quint8 *ptr = data.u8;
    while (len > 0) {
        switch (len) {
        default:
            // a byte or more left
            *ptr |= 0x01;
        case 7:
            *ptr |= 0x02;
        case 6:
            *ptr |= 0x04;
        case 5:
            *ptr |= 0x08;
        case 4:
            *ptr |= 0x10;
        case 3:
            *ptr |= 0x20;
        case 2:
            *ptr |= 0x40;
        case 1:
            *ptr |= 0x80;
        }

        len -= 8;
        ++ptr;
    }

    if (family == AF_INET)
        return QHostAddress(htonl(data.ipv4));
    else if (family == AF_INET6)
        return QHostAddress(data.u8);
    else
        return QHostAddress();
}

static QList<QNetworkInterfacePrivate *> interfaceListingWinXP()
{
    QList<QNetworkInterfacePrivate *> interfaces;
    IP_ADAPTER_ADDRESSES staticBuf[2]; // 2 is arbitrary
    PIP_ADAPTER_ADDRESSES pAdapter = staticBuf;
    ULONG bufSize = sizeof staticBuf;

    ULONG flags = GAA_FLAG_INCLUDE_ALL_INTERFACES |
                  GAA_FLAG_INCLUDE_PREFIX |
                  GAA_FLAG_SKIP_DNS_SERVER |
                  GAA_FLAG_SKIP_MULTICAST;
    ULONG retval = ptrGetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAdapter, &bufSize);
    if (retval == ERROR_BUFFER_OVERFLOW) {
        // need more memory
        pAdapter = (IP_ADAPTER_ADDRESSES *)qMalloc(bufSize);

        // try again
        if (ptrGetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAdapter, &bufSize) != ERROR_SUCCESS) {
            qFree(pAdapter);
            return interfaces;
        }
    } else if (retval != ERROR_SUCCESS) {
        // error
        return interfaces;
    }

    // iterate over the list and add the entries to our listing
    for (PIP_ADAPTER_ADDRESSES ptr = pAdapter; ptr; ptr = ptr->Next) {
        QNetworkInterfacePrivate *iface = new QNetworkInterfacePrivate;
        interfaces << iface;

        iface->index = 0;
        if (ptr->Length >= offsetof(IP_ADAPTER_ADDRESSES, Ipv6IfIndex))
            iface->index = ptr->Ipv6IfIndex;
        else if (ptr->IfIndex != 0)
            iface->index = ptr->IfIndex;

        iface->flags = QNetworkInterface::CanBroadcast;
		if (ptr->OperStatus == IfOperStatusUp)
			iface->flags |= QNetworkInterface::IsUp | QNetworkInterface::IsRunning;
        if ((ptr->Flags & IP_ADAPTER_NO_MULTICAST) == 0)
            iface->flags |= QNetworkInterface::CanMulticast;

        iface->name = QString::fromLocal8Bit(ptr->AdapterName);
        if (ptr->PhysicalAddressLength)
            iface->hardwareAddress = iface->makeHwAddress(ptr->PhysicalAddressLength,
                                                          ptr->PhysicalAddress);
        else
            // loopback if it has no address
            iface->flags |= QNetworkInterface::IsLoopBack;

        // The GetAdaptersAddresses call has an interesting semantic:
        // It can return a number N of addresses and a number M of prefixes.
        // But if you have IPv6 addresses, generally N > M.
        // I cannot find a way to relate the Address to the Prefix, aside from stopping
        // the iteration at the last Prefix entry and assume that it applies to all addresses
        // from that point on.
        PIP_ADAPTER_PREFIX pprefix = 0;
        if (ptr->Length >= offsetof(IP_ADAPTER_ADDRESSES, FirstPrefix))
            pprefix = ptr->FirstPrefix;
        for (PIP_ADAPTER_UNICAST_ADDRESS addr = ptr->FirstUnicastAddress; addr; addr = addr->Next) {
            QNetworkAddressEntry entry;
            entry.setIp(addressFromSockaddr(addr->Address.lpSockaddr));
            if (pprefix) {
                entry.setNetmask(netmaskFromPrefixLength(pprefix->PrefixLength, addr->Address.lpSockaddr->sa_family));
                pprefix = pprefix->Next ? pprefix->Next : pprefix;
            }

            iface->addressEntries << entry;
        }
    }

    if (pAdapter != staticBuf)
        qFree(pAdapter);

    return interfaces;
}

static QList<QNetworkInterfacePrivate *> interfaceListingWin2k()
{
    QList<QNetworkInterfacePrivate *> interfaces;
    IP_ADAPTER_INFO staticBuf[2]; // 2 is arbitrary
    PIP_ADAPTER_INFO pAdapter = staticBuf;
    ULONG bufSize = sizeof staticBuf;

    DWORD retval = ptrGetAdaptersInfo(pAdapter, &bufSize);
    if (retval == ERROR_BUFFER_OVERFLOW) {
        // need more memory
        pAdapter = (IP_ADAPTER_INFO *)qMalloc(bufSize);

        // try again
        if (ptrGetAdaptersInfo(pAdapter, &bufSize) != ERROR_SUCCESS) {
            qFree(pAdapter);
            return interfaces;
        }
    } else if (retval != ERROR_SUCCESS) {
        // error
        return interfaces;
    }

    // iterate over the list and add the entries to our listing
    for (PIP_ADAPTER_INFO ptr = pAdapter; ptr; ptr = ptr->Next) {
        QNetworkInterfacePrivate *iface = new QNetworkInterfacePrivate;
        interfaces << iface;

        iface->index = ptr->Index;
        iface->flags = QNetworkInterface::IsUp | QNetworkInterface::IsRunning;
        if (ptr->Type == MIB_IF_TYPE_PPP)
            iface->flags |= QNetworkInterface::IsPointToPoint;
        else
            iface->flags |= QNetworkInterface::CanBroadcast;
        iface->name = QString::fromLocal8Bit(ptr->AdapterName);
        iface->hardwareAddress = QNetworkInterfacePrivate::makeHwAddress(ptr->AddressLength,
                                                                         ptr->Address);

        for (PIP_ADDR_STRING addr = &ptr->IpAddressList; addr; addr = addr->Next) {
            QNetworkAddressEntry entry;
            entry.setIp(QHostAddress(QLatin1String(addr->IpAddress.String)));
            entry.setNetmask(QHostAddress(QLatin1String(addr->IpMask.String)));

            // calculate the broadcast address
            quint32 ip = entry.ip().toIPv4Address();
            quint32 mask = entry.netmask().toIPv4Address();
            quint32 broadcast = (ip & mask) | (0xFFFFFFFFU & ~mask);

            entry.setBroadcast(QHostAddress((broadcast)));

            iface->addressEntries << entry;
        }
    }

    if (pAdapter != staticBuf)
        qFree(pAdapter);

    return interfaces;
}

static QList<QNetworkInterfacePrivate *> interfaceListing()
{
    resolveLibs();
    if (ptrGetAdaptersAddresses != NULL)
        return interfaceListingWinXP();
    else if (ptrGetAdaptersInfo != NULL)
        return interfaceListingWin2k();

    // failed
    return QList<QNetworkInterfacePrivate *>();
}

QList<QNetworkInterfacePrivate *> QNetworkInterfaceManager::scan()
{
    return interfaceListing();
}

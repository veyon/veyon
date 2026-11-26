// SPDX-FileCopyrightText: 2026 Veyon Community
// SPDX-License-Identifier: GPL-2.0-or-later

#define INITGUID
#include <winsock2.h>
#include <windows.h>
#include <netfw.h>

#include "InternetBlockAddressRanges.h"
#include "InternetBlocker.h"

// Windows Firewall evaluates block rules before allow rules, so a single
// outbound block rule whose remote addresses cover everything except the
// allowed networks blocks internet access for all applications while LAN
// traffic (including Veyon itself) keeps working.

namespace {

constexpr auto WindowsFirewallServiceError = HRESULT(0x800706D9);

// NET_FW_IP_PROTOCOL_ANY - not defined by all MinGW versions of icftypes.h
constexpr long FirewallProtocolAny = 256;

// global unicast IPv6 address space - everything outside of it (loopback,
// link-local, ULA, multicast) stays reachable
const auto BlockedIPv6Range = QStringLiteral("2000::-3fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");


class ComScope
{
public:
	ComScope() :
		m_result(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))
	{
	}

	~ComScope()
	{
		if (SUCCEEDED(m_result))
		{
			CoUninitialize();
		}
	}

	bool isUsable() const
	{
		// RPC_E_CHANGED_MODE: COM already initialized with a different mode - fine for us
		return SUCCEEDED(m_result) || m_result == RPC_E_CHANGED_MODE;
	}

	HRESULT result() const
	{
		return m_result;
	}

private:
	HRESULT m_result;
};


template<class T>
class ComPtr
{
public:
	ComPtr() = default;
	~ComPtr()
	{
		if (m_ptr)
		{
			m_ptr->Release();
		}
	}
	ComPtr(const ComPtr&) = delete;
	ComPtr& operator=(const ComPtr&) = delete;

	T* operator->() const
	{
		return m_ptr;
	}
	T** put()
	{
		return &m_ptr;
	}
	explicit operator bool() const
	{
		return m_ptr != nullptr;
	}

private:
	T* m_ptr = nullptr;
};


class Bstr
{
public:
	explicit Bstr(const QString& string) :
		m_bstr(SysAllocString(reinterpret_cast<const OLECHAR*>(string.utf16())))
	{
	}
	~Bstr()
	{
		SysFreeString(m_bstr);
	}
	Bstr(const Bstr&) = delete;
	Bstr& operator=(const Bstr&) = delete;

	operator BSTR() const
	{
		return m_bstr;
	}

private:
	BSTR m_bstr;
};


bool openFirewallRules(ComPtr<INetFwPolicy2>& policy, ComPtr<INetFwRules>& rules)
{
	auto hr = CoCreateInstance(CLSID_NetFwPolicy2, nullptr, CLSCTX_INPROC_SERVER,
							   IID_INetFwPolicy2, reinterpret_cast<void**>(policy.put()));
	if (FAILED(hr))
	{
		vCritical() << "CoCreateInstance(NetFwPolicy2) failed:" << Qt::hex << hr;
		return false;
	}

	hr = policy->get_Rules(rules.put());
	if (FAILED(hr))
	{
		vCritical() << "INetFwPolicy2::get_Rules() failed:" << Qt::hex << hr;
		return false;
	}

	return true;
}


void warnIfFirewallDisabled(INetFwPolicy2* policy)
{
	long currentProfiles = 0;
	if (FAILED(policy->get_CurrentProfileTypes(&currentProfiles)))
	{
		return;
	}

	for (const auto profile : {NET_FW_PROFILE2_DOMAIN, NET_FW_PROFILE2_PRIVATE, NET_FW_PROFILE2_PUBLIC})
	{
		if (currentProfiles & profile)
		{
			VARIANT_BOOL enabled = VARIANT_TRUE;
			if (SUCCEEDED(policy->get_FirewallEnabled(profile, &enabled)) && enabled == VARIANT_FALSE)
			{
				vWarning() << "Windows Firewall is disabled for active profile" << profile
						   << "- internet blocking rule will have no effect";
			}
		}
	}
}

} // namespace



bool InternetBlocker::block(const Settings& settings)
{
	ComScope com;
	if (com.isUsable() == false)
	{
		vCritical() << "CoInitializeEx() failed:" << Qt::hex << com.result();
		return false;
	}

	ComPtr<INetFwPolicy2> policy;
	ComPtr<INetFwRules> rules;
	if (openFirewallRules(policy, rules) == false)
	{
		return false;
	}

	const Bstr name(ruleName());

	// remove any previous instance (e.g. with outdated address list)
	rules->Remove(name);

	auto remoteAddresses = InternetBlockAddressRanges::blockedIPv4Ranges(settings.allowedIPv4Networks);
	if (settings.blockIPv6)
	{
		if (settings.allowedIPv6Networks != InternetBlockAddressRanges::defaultAllowedIPv6Networks())
		{
			vWarning() << "additional allowed IPv6 networks are not supported on Windows and will be ignored";
		}
		remoteAddresses.append(BlockedIPv6Range);
	}

	if (remoteAddresses.isEmpty())
	{
		vWarning() << "nothing to block";
		return true;
	}

	ComPtr<INetFwRule> rule;
	auto hr = CoCreateInstance(CLSID_NetFwRule, nullptr, CLSCTX_INPROC_SERVER,
							   IID_INetFwRule, reinterpret_cast<void**>(rule.put()));
	if (FAILED(hr))
	{
		vCritical() << "CoCreateInstance(NetFwRule) failed:" << Qt::hex << hr;
		return false;
	}

	const Bstr description(QStringLiteral("Blocks internet access while the Veyon \"Block internet\" feature is active"));
	const Bstr grouping(QStringLiteral("Veyon"));
	const Bstr addresses(remoteAddresses.join(QLatin1Char(',')));

	rule->put_Name(name);
	rule->put_Description(description);
	rule->put_Grouping(grouping);
	rule->put_Direction(NET_FW_RULE_DIR_OUT);
	rule->put_Action(NET_FW_ACTION_BLOCK);
	rule->put_Protocol(FirewallProtocolAny);
	rule->put_Profiles(NET_FW_PROFILE2_ALL);

	hr = rule->put_RemoteAddresses(addresses);
	if (FAILED(hr))
	{
		vCritical() << "INetFwRule::put_RemoteAddresses() failed:" << Qt::hex << hr << remoteAddresses;
		return false;
	}

	rule->put_Enabled(VARIANT_TRUE);

	hr = rules->Add(rule.operator->());
	if (FAILED(hr))
	{
		if (hr == WindowsFirewallServiceError)
		{
			vCritical() << "Windows Firewall service is not running - can't block internet access";
		}
		else
		{
			vCritical() << "INetFwRules::Add() failed:" << Qt::hex << hr;
		}
		return false;
	}

	warnIfFirewallDisabled(policy.operator->());

	return true;
}



bool InternetBlocker::unblock()
{
	ComScope com;
	if (com.isUsable() == false)
	{
		return false;
	}

	ComPtr<INetFwPolicy2> policy;
	ComPtr<INetFwRules> rules;
	if (openFirewallRules(policy, rules) == false)
	{
		return false;
	}

	const Bstr name(ruleName());

	// Remove() returns S_OK even if the rule does not exist; loop in case the
	// rule was added multiple times (e.g. by concurrent server instances)
	for (int i = 0; i < 16; ++i)
	{
		ComPtr<INetFwRule> existing;
		if (FAILED(rules->Item(name, existing.put())) || !existing)
		{
			return true;
		}

		const auto hr = rules->Remove(name);
		if (FAILED(hr))
		{
			vCritical() << "INetFwRules::Remove() failed:" << Qt::hex << hr;
			return false;
		}
	}

	return true;
}



bool InternetBlocker::isBlocked()
{
	ComScope com;
	if (com.isUsable() == false)
	{
		return false;
	}

	ComPtr<INetFwPolicy2> policy;
	ComPtr<INetFwRules> rules;
	if (openFirewallRules(policy, rules) == false)
	{
		return false;
	}

	const Bstr name(ruleName());
	ComPtr<INetFwRule> existing;
	return SUCCEEDED(rules->Item(name, existing.put())) && existing;
}



QString InternetBlocker::nftablesScript(const Settings& settings)
{
	Q_UNUSED(settings)
	return {};
}

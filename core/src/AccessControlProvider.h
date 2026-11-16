// SPDX-FileCopyrightText: 2016-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AccessControlRule.h"
#include "FeatureProviderInterface.h"
#include "NetworkObject.h"

class UserGroupsBackendInterface;
class NetworkObjectDirectory;

class VEYON_CORE_EXPORT AccessControlProvider : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	enum class Access {
		Deny,
		Allow,
		ToBeConfirmed,
	};

	enum class Reason {
		Unknown,
		NoAccessControlRuleMatched,
		AccessControlRuleMatched,
		NotConfirmedByUser,
		ConfirmedByUser,
		UserNotInAuthorizedUserGroups,
		UserInAuthorizedUserGroups,
	};

	enum class Argument {
		Details,
	};

	struct CheckResult
	{
		Access access = Access::Deny;
		Reason reason = Reason::Unknown;
		AccessControlRule::Pointer matchedRule = nullptr;
	};

	AccessControlProvider();

	QStringList userGroups() const;
	QStringList locations() const;
	QStringList locationsOfComputer(const QString& computer) const;

	CheckResult checkAccess(const QString& accessingUser, const QString& accessingComputer,
							const QStringList& connectedUsers);

	bool processAuthorizedGroups(const QString& accessingUser);

	AccessControlRule::Pointer processAccessControlRules(const QString& accessingUser,
														 const QString& accessingComputer,
														 const QString& localUser,
														 const QString& localComputer,
														 const QStringList& connectedUsers);

	bool isAccessToLocalComputerDenied() const;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("ef3f61f4-b7fa-41a1-ae09-74e022048617")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 0);
	}

	QString name() const override
	{
		return QStringLiteral("AccessControlProvider");
	}

	QString description() const override
	{
		return tr("Provider for access control features");
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	const Feature& feature() const
	{
		return m_accessControlFeature;
	}

	const FeatureList& featureList() const override
	{
		return m_features;
	}

	bool controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces) override
	{
		Q_UNUSED(featureUid)
		Q_UNUSED(operation)
		Q_UNUSED(arguments)
		Q_UNUSED(computerControlInterfaces)

		return false;
	}

	bool handleFeatureMessage(ComputerControlInterface::Pointer computerControlInterface,
							  const FeatureMessage& message) override;

	void sendDetails(QIODevice* ioDevice, const QString& details);

private:
	bool isMemberOfUserGroup(const QString& user, const QString& groupName) const;
	bool isLocatedAt(const QString& computer, const QString& locationName) const;
	bool haveGroupsInCommon(const QString& userOne, const QString& userTwo) const;
	bool haveSameLocations(const QString& computerOne, const QString& computerTwo) const;
	bool isLocalHost(const QString& accessingComputer) const;
	bool isLocalUser(const QString& accessingUser, const QString& localUser) const;
	bool isNoUserLoggedOn() const;

	QString lookupSubject(AccessControlRule::Subject subject,
						  const QString& accessingUser, const QString& accessingComputer,
						  const QString& localUser, const QString& localComputer) const;

	bool matchConditions(const AccessControlRule& rule,
						 const QString& accessingUser, const QString& accessingComputer,
						 const QString& localUser, const QString& localComputer,
						 const QStringList& connectedUsers) const;

	static QStringList objectNames(const NetworkObjectList& objects);
	static bool matchList(const QStringList& list, const QString& pattern);

	QList<AccessControlRule::Pointer> m_accessControlRules{};
	UserGroupsBackendInterface* m_userGroupsBackend;
	NetworkObjectDirectory* m_networkObjectDirectory;
	bool m_useDomainUserGroups;

	Feature m_accessControlFeature;
	FeatureList m_features{m_accessControlFeature};

	QStringList m_authorizedUserGroups;
};

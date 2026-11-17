// SPDX-FileCopyrightText: 2016-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QNetworkInterface>
#include <QRegularExpression>
#include <QScopeGuard>

#include "AccessControlProvider.h"
#include "HostAddress.h"
#include "NetworkObjectDirectory.h"
#include "NetworkObjectDirectoryManager.h"
#include "PlatformPluginInterface.h"
#include "PlatformUserFunctions.h"
#include "UserGroupsBackendManager.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"


AccessControlProvider::AccessControlProvider() :
	m_userGroupsBackend(VeyonCore::userGroupsBackendManager().configuredBackend()),
	m_networkObjectDirectory(VeyonCore::networkObjectDirectoryManager().configuredDirectory()),
	m_useDomainUserGroups(VeyonCore::config().useDomainUserGroups()),
	m_accessControlFeature(QLatin1String(staticMetaObject.className()),
						   Feature::Flag::Meta | Feature::Flag::Builtin,
						   Feature::Uid{QStringLiteral("1815941e-eaef-43ab-b9cd-5403dca3f749")}, {}, {}, {}, {}, {}),
	m_authorizedUserGroups(VeyonCore::config().authorizedUserGroups()),
	m_directoryQueries(m_userGroupsBackend, m_networkObjectDirectory, m_useDomainUserGroups)
{
	loadAccessControlRules(VeyonCore::config().accessControlRules());
}



AccessControlProvider::AccessControlProvider(UserGroupsBackendInterface* userGroupsBackend,
											 NetworkObjectDirectory* networkObjectDirectory,
											 const QJsonArray& accessControlRules,
											 const AccessControlDirectoryQueries::Clock& clock) :
	m_userGroupsBackend(userGroupsBackend),
	m_networkObjectDirectory(networkObjectDirectory),
	m_useDomainUserGroups(false),
	m_accessControlFeature(QLatin1String(staticMetaObject.className()),
						   Feature::Flag::Meta | Feature::Flag::Builtin,
						   Feature::Uid{QStringLiteral("1815941e-eaef-43ab-b9cd-5403dca3f749")}, {}, {}, {}, {}, {}),
	m_directoryQueries(m_userGroupsBackend, m_networkObjectDirectory, m_useDomainUserGroups, clock)
{
	loadAccessControlRules(accessControlRules);
}



void AccessControlProvider::beginCheckPass()
{
	m_directoryQueries.beginPass();
}



void AccessControlProvider::endCheckPass()
{
	m_directoryQueries.endPass();
}



void AccessControlProvider::loadAccessControlRules(const QJsonArray& accessControlRules)
{
	m_accessControlRules.reserve(accessControlRules.size());

	for (const auto& accessControlRule : accessControlRules)
	{
		m_accessControlRules.append(AccessControlRule::Pointer::create(accessControlRule));
	}
}



QStringList AccessControlProvider::userGroups() const
{
	auto userGroupList = m_userGroupsBackend->userGroups(m_useDomainUserGroups);

	std::sort(userGroupList.begin(), userGroupList.end());

	return userGroupList;
}



QStringList AccessControlProvider::locations() const
{
	auto locationList = objectNames(m_networkObjectDirectory->queryObjects(NetworkObject::Type::Location,
																		   NetworkObject::Attribute::None, {}));

	std::sort(locationList.begin(), locationList.end());

	return locationList;
}



QStringList AccessControlProvider::locationsOfComputer(const QString& computer) const
{
	const auto fqdn = HostAddress(computer).convert(HostAddress::Type::FullyQualifiedDomainName);

	vDebug() << "Searching for locations of computer" << computer << "via FQDN" << fqdn;

	if (fqdn.isEmpty())
	{
		vWarning() << "Empty FQDN - returning empty location list";
		return {};
	}

	const auto locationList = m_directoryQueries.locationsOfComputer(fqdn);

	vDebug() << "Found locations:" << locationList;

	return locationList;
}



AccessControlProvider::CheckResult AccessControlProvider::checkAccess(const QString& accessingUser,
																	  const QString& accessingComputer,
																	  const QStringList& connectedUsers)
{
	// directory results are reused within this check (or the re-check pass it belongs to)
	beginCheckPass();
	const auto endPass = qScopeGuard([this]() { endCheckPass(); });

	CheckResult denyAccessCheckResult{Access::Deny};
	if (VeyonCore::config().isAccessRestrictedToUserGroups())
	{
		m_directoryQueries.resetFailure();
		const auto isInAuthorizedGroups = processAuthorizedGroups(accessingUser);
		if (m_directoryQueries.hasFailed())
		{
			vWarning() << "user groups not available, denying access";
			return {Access::Deny, Reason::DirectoryUnavailable};
		}
		if (isInAuthorizedGroups)
		{
			return {Access::Allow, Reason::UserInAuthorizedUserGroups};
		}
		denyAccessCheckResult.reason = Reason::UserNotInAuthorizedUserGroups;
	}
	else if (VeyonCore::config().isAccessControlRulesProcessingEnabled())
	{
		const auto rule = processAccessControlRules(accessingUser,
													accessingComputer,
													VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName),
													HostAddress::localFQDN(),
													connectedUsers);
		// a condition evaluated without directory data would select the wrong rule
		if (m_directoryQueries.hasFailed())
		{
			vWarning() << "directory not available while processing access control rules, denying access";
			return {Access::Deny, Reason::DirectoryUnavailable};
		}
		if (rule)
		{
			switch (rule->action())
			{
			case AccessControlRule::Action::Allow:
				return {Access::Allow, Reason::AccessControlRuleMatched};
			case AccessControlRule::Action::AskForPermission:
				return {Access::ToBeConfirmed};
			default: break;
			}
			denyAccessCheckResult.reason = Reason::AccessControlRuleMatched;
			denyAccessCheckResult.matchedRule = rule;
		}
		else
		{
			denyAccessCheckResult.reason = Reason::NoAccessControlRuleMatched;
		}
	}
	else
	{
		vDebug() << "no access control method configured, allowing access.";

		// no access control method configured, therefore grant access
		return {Access::Allow};
	}

	vDebug() << "configured access control method did not succeed, denying access.";

	// configured access control method did not succeed, therefore deny access
	return denyAccessCheckResult;
}



bool AccessControlProvider::processAuthorizedGroups(const QString& accessingUser)
{
	vDebug() << "processing for user" << accessingUser;

	const auto groupsOfAccessingUser = groupsOfUser(accessingUser);

	vDebug() << groupsOfAccessingUser << m_authorizedUserGroups;

	const auto groupsOfAccessingUserSet = QSet<QString>{groupsOfAccessingUser.begin(), groupsOfAccessingUser.end()};
	const auto authorizedUserGroupSet = QSet<QString>{m_authorizedUserGroups.begin(), m_authorizedUserGroups.end()};

	return groupsOfAccessingUserSet.intersects(authorizedUserGroupSet);
}



AccessControlRule::Pointer AccessControlProvider::processAccessControlRules(const QString& accessingUser,
																			const QString& accessingComputer,
																			const QString& localUser,
																			const QString& localComputer,
																			const QStringList& connectedUsers)
{
	vDebug() << "processing rules for" << accessingUser << accessingComputer << localUser << localComputer << connectedUsers;

	m_directoryQueries.resetFailure();

	for (const auto& rule : std::as_const(m_accessControlRules))
	{
		// rule disabled?
		if (rule->action() == AccessControlRule::Action::None)
		{
			// then continue with next rule
			continue;
		}

		const auto matches = rule->areConditionsIgnored() ||
			matchConditions(*rule, accessingUser, accessingComputer, localUser, localComputer, connectedUsers);

		// no decision on missing directory data: stop at the first failed query (deny)
		if (hasDirectoryQueryFailed())
		{
			vWarning() << "directory query failed while processing rule" << rule->name() << "- denying access";
			return nullptr;
		}

		if (matches)
		{
			vDebug() << "rule" << rule->name() << "matched with action" << rule->action();
			return rule;
		}
	}

	vDebug() << "no matching rule, denying access";

	return nullptr;
}


/*!
 * \brief Returns whether any incoming access requests would be denied due to a deny rule matching the local state (e.g. teacher logged on)
 */
bool AccessControlProvider::isAccessToLocalComputerDenied() const
{
	if (VeyonCore::config().isAccessControlRulesProcessingEnabled() == false)
	{
		return false;
	}

	return isAccessDeniedForLocalState(VeyonCore::platform().userFunctions().queryCurrentUserProperty(PlatformUserFunctions::UserProperty::LoginName),
									   HostAddress::localFQDN());
}



bool AccessControlProvider::isAccessDeniedForLocalState(const QString& localUser, const QString& localComputer) const
{
	m_directoryQueries.resetFailure();

	for (const auto& rule : std::as_const(m_accessControlRules))
	{
		const auto conditionsMatch = matchConditions(*rule, {}, {}, localUser, localComputer, {});

		// only an optimisation (listening on localhost only): on missing directory data assume access
		// is possible, every connection is still checked (and denied) individually
		if (hasDirectoryQueryFailed())
		{
			vWarning() << "directory not available, not restricting access to the local computer";
			return false;
		}

		if (conditionsMatch)
		{
			switch (rule->action())
			{
			case AccessControlRule::Action::Deny:
				return true;
			case AccessControlRule::Action::Allow:
			case AccessControlRule::Action::AskForPermission:
				return false;
			default:
				break;
			}
		}
	}

	return false;
}



bool AccessControlProvider::hasDirectoryQueryFailed() const
{
	return m_directoryQueries.hasFailed();
}



bool AccessControlProvider::handleFeatureMessage(ComputerControlInterface::Pointer computerControlInterface, const FeatureMessage& message)
{
	if (message.featureUid() == m_accessControlFeature.uid())
	{
		computerControlInterface->setAccessControlFailed(message.argument(Argument::Details).toString());
		return true;
	}

	return false;
}



void AccessControlProvider::sendDetails(QIODevice* ioDevice, const QString& details)
{
	FeatureMessage{m_accessControlFeature.uid()}.addArgument(
													AccessControlProvider::Argument::Details, details)
		.sendAsRfbMessage(ioDevice);
}



QStringList AccessControlProvider::groupsOfUser(const QString& user) const
{
	return m_directoryQueries.groupsOfUser(user);
}



bool AccessControlProvider::isMemberOfUserGroup(const QString& user,
												const QString& groupName) const
{
	return matchList(groupsOfUser(user), groupName);
}



bool AccessControlProvider::isLocatedAt(const QString& computer, const QString& locationName) const
{
	return matchList(locationsOfComputer(computer), locationName);
}



bool AccessControlProvider::haveGroupsInCommon(const QString& userOne, const QString& userTwo) const
{
	const auto userOneGroups = groupsOfUser(userOne);
	if (hasDirectoryQueryFailed())
	{
		return false;
	}
	const auto userTwoGroups = groupsOfUser(userTwo);

	const auto userOneGroupSet = QSet<QString>{userOneGroups.begin(), userOneGroups.end()};
	const auto userTwoGroupSet = QSet<QString>{userTwoGroups.begin(), userTwoGroups.end()};

	return userOneGroupSet.intersects(userTwoGroupSet);
}



bool AccessControlProvider::haveSameLocations(const QString& computerOne, const QString& computerTwo) const
{
	const auto computerOneLocations = locationsOfComputer(computerOne);
	const auto computerTwoLocations = locationsOfComputer(computerTwo);

	return computerOneLocations.isEmpty() == false &&
		computerOneLocations == computerTwoLocations;
}



bool AccessControlProvider::isLocalHost(const QString& accessingComputer) const
{
	return HostAddress(accessingComputer).isLocalHost();
}



bool AccessControlProvider::isLocalUser(const QString& accessingUser, const QString& localUser) const
{
	return accessingUser.isEmpty() == false &&
		accessingUser == localUser;
}



bool AccessControlProvider::isNoUserLoggedOn() const
{
	return VeyonCore::platform().userFunctions().isAnyUserLoggedOn() == false;
}



QString AccessControlProvider::lookupSubject(AccessControlRule::Subject subject,
											 const QString& accessingUser, const QString& accessingComputer,
											 const QString& localUser, const QString& localComputer) const
{
	switch (subject)
	{
	case AccessControlRule::Subject::AccessingUser: return accessingUser;
	case AccessControlRule::Subject::AccessingComputer: return accessingComputer;
	case AccessControlRule::Subject::LocalUser: return localUser;
	case AccessControlRule::Subject::LocalComputer: return localComputer;
	default: break;
	}

	return {};
}



bool AccessControlProvider::matchConditions(const AccessControlRule& rule,
											const QString& accessingUser, const QString& accessingComputer,
											const QString& localUser, const QString& localComputer,
											const QStringList& connectedUsers) const
{
	bool hasConditions = false;

	// normally all selected conditions have to match in order to make the whole rule match
	// if conditions should be inverted (i.e. "is member of" is to be interpreted as "is NOT member of")
	// we have to check against the opposite boolean value
	bool matchResult = rule.areConditionsInverted() == false;

	vDebug() << rule.toJson() << matchResult;

	if (rule.isConditionEnabled(AccessControlRule::Condition::MemberOfUserGroup))
	{
		hasConditions = true;

		const auto condition = AccessControlRule::Condition::MemberOfUserGroup;
		const auto user = lookupSubject(rule.subject(condition), accessingUser, {}, localUser, {});
		const auto group = rule.argument(condition);

		if (user.isEmpty() || group.isEmpty() ||
			isMemberOfUserGroup(user, group) != matchResult ||
			hasDirectoryQueryFailed())
		{
			return false;
		}
	}

	if (rule.isConditionEnabled(AccessControlRule::Condition::GroupsInCommon))
	{
		hasConditions = true;

		if (accessingUser.isEmpty() || localUser.isEmpty() ||
			haveGroupsInCommon(accessingUser, localUser) != matchResult ||
			hasDirectoryQueryFailed())
		{
			return false;
		}
	}

	if (rule.isConditionEnabled(AccessControlRule::Condition::LocatedAt))
	{
		hasConditions = true;

		const auto condition = AccessControlRule::Condition::LocatedAt;
		const auto computer = lookupSubject(rule.subject(condition), {}, accessingComputer, {}, localComputer);
		const auto location = rule.argument(condition);

		if (computer.isEmpty() || location.isEmpty() ||
			isLocatedAt(computer, location) != matchResult ||
			hasDirectoryQueryFailed())
		{
			return false;
		}
	}

	if (rule.isConditionEnabled(AccessControlRule::Condition::SameLocation))
	{
		hasConditions = true;

		if (accessingComputer.isEmpty() || localComputer.isEmpty() ||
			haveSameLocations(accessingComputer, localComputer) != matchResult ||
			hasDirectoryQueryFailed())
		{
			return false;
		}
	}

	if (rule.isConditionEnabled(AccessControlRule::Condition::AccessFromLocalHost))
	{
		hasConditions = true;

		if (isLocalHost(accessingComputer) != matchResult)
		{
			return false;
		}
	}

	if (rule.isConditionEnabled(AccessControlRule::Condition::AccessFromLocalUser))
	{
		hasConditions = true;

		if (isLocalUser(accessingUser, localUser) != matchResult)
		{
			return false;
		}
	}

	if (rule.isConditionEnabled(AccessControlRule::Condition::AccessFromAlreadyConnectedUser))
	{
		hasConditions = true;

		if (connectedUsers.contains(accessingUser) != matchResult)
		{
			return false;
		}
	}

	if (rule.isConditionEnabled(AccessControlRule::Condition::NoUserLoggedOn))
	{
		hasConditions = true;

		if (isNoUserLoggedOn() != matchResult)
		{
			return false;
		}
	}

	if (rule.isConditionEnabled(AccessControlRule::Condition::ComputerAlreadyBeingAccessed))
	{
		hasConditions = true;

		if (!connectedUsers.isEmpty() != matchResult)
		{
			return false;
		}
	}

	// do not match the rule if no conditions are set at all
	if (hasConditions == false)
	{
		return false;
	}

	return true;
}



QStringList AccessControlProvider::objectNames(const NetworkObjectList& objects)
{
	QStringList nameList;
	nameList.reserve(objects.size());

	for (const auto& object : objects)
	{
		nameList.append(object.name());
	}

	return nameList;
}



bool AccessControlProvider::matchList(const QStringList& list, const QString& pattern)
{
	if (pattern.startsWith(QLatin1Char('/')) && pattern.endsWith(QLatin1Char('/')) &&
		pattern.length() > 2)
	{
		return list.indexOf(QRegularExpression(pattern.mid(1, pattern.length() - 2))) >= 0;
	}

	if (pattern.endsWith(QLatin1Char('*')))
	{
		const auto rx = QRegularExpression(QRegularExpression::wildcardToRegularExpression(pattern));
		if (rx.isValid())
		{
			return list.indexOf(rx) >= 0;
		}
	}

	return list.contains(pattern);
}

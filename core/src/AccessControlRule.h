// SPDX-FileCopyrightText: 2016-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QJsonObject>
#include <QObject>
#include <QVariant>

#include "VeyonCore.h"

class VEYON_CORE_EXPORT AccessControlRule
{
	Q_GADGET
public:
	enum class Action {
		None,
		Allow,
		Deny,
		AskForPermission,
	};

	Q_ENUM(Action)

	enum class Condition {
		None,
		MemberOfUserGroup,
		GroupsInCommon,
		LocatedAt,
		SameLocation,
		AccessFromLocalHost,
		AccessFromLocalUser,
		AccessFromAlreadyConnectedUser,
		NoUserLoggedOn,
		NoUserLoggedInRemotely,
		AccessedUserLoggedInLocally,
		UserSession,
		AuthenticationMethod,
		ComputerAlreadyBeingAccessed,
	};

	Q_ENUM(Condition)

	enum class Subject {
		None,
		AccessingUser,
		AccessingComputer,
		LocalUser,
		LocalComputer,
	};

	Q_ENUM(Subject)

	using ConditionArgument = QString;

	struct ConditionParameters
	{
		bool enabled{false};
		Subject subject{Subject::None};
		ConditionArgument argument;
	};

	using ConditionParameterMap = QMap<Condition, ConditionParameters>;

	using Pointer = QSharedPointer<AccessControlRule>;

	AccessControlRule();
	explicit AccessControlRule(const QJsonValue& jsonValue);

	~AccessControlRule() = default;

	AccessControlRule& operator=(const AccessControlRule& other);

	const QString& name() const
	{
		return m_name;
	}

	void setName(const QString& name)
	{
		m_name = name;
	}

	const QString& description() const
	{
		return m_description;
	}

	void setDescription(const QString& description)
	{
		m_description = description;
	}

	Action action() const
	{
		return m_action;
	}

	void setAction(Action action)
	{
		m_action = action;
	}

	const ConditionParameterMap& parameters() const
	{
		return m_parameters;
	}

	Subject subject(Condition condition) const
	{
		return m_parameters.value(condition).subject;
	}

	void setSubject(Condition condition, Subject subject)
	{
		m_parameters[condition].subject = subject;
	}

	bool areConditionsIgnored() const
	{
		return m_ignoreConditions;
	}

	void setConditionsIgnored(bool ignored)
	{
		m_ignoreConditions = ignored;
	}

	bool areConditionsInverted() const
	{
		return m_invertConditions;
	}

	void setConditionsInverted(bool inverted)
	{
		m_invertConditions = inverted;
	}

	bool isConditionEnabled(Condition condition) const
	{
		return m_parameters.value(condition).enabled;
	}

	void setConditionEnabled(Condition condition, bool enabled)
	{
		m_parameters[condition].enabled = enabled;
	}

	ConditionArgument argument(Condition condition) const
	{
		return m_parameters.value(condition).argument;
	}

	void clearParameters()
	{
		m_parameters.clear();
	}

	void setArgument(Condition condition, const ConditionArgument& conditionArgument)
	{
		m_parameters[condition].argument = conditionArgument;
	}

	QJsonObject toJson() const;


private:
	QString m_name;
	QString m_description;
	Action m_action;
	ConditionParameterMap m_parameters;
	bool m_invertConditions;
	bool m_ignoreConditions;
};

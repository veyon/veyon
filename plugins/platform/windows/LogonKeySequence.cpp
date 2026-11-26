// SPDX-FileCopyrightText: 2026 Veyon Community
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QHash>
#include <QRegularExpression>

#include "LogonKeySequence.h"


static const QHash<QString, LogonKeySequence::KeyCode>& keyNames()
{
	static const QHash<QString, LogonKeySequence::KeyCode> names{
		{QStringLiteral("tab"), LogonKeySequence::Key_Tab},
		{QStringLiteral("return"), LogonKeySequence::Key_Return},
		{QStringLiteral("enter"), LogonKeySequence::Key_Return},
		{QStringLiteral("escape"), LogonKeySequence::Key_Escape},
		{QStringLiteral("esc"), LogonKeySequence::Key_Escape},
		{QStringLiteral("space"), LogonKeySequence::Key_Space},
		{QStringLiteral("backspace"), LogonKeySequence::Key_BackSpace},
		{QStringLiteral("delete"), LogonKeySequence::Key_Delete},
		{QStringLiteral("del"), LogonKeySequence::Key_Delete},
		{QStringLiteral("up"), LogonKeySequence::Key_Up},
		{QStringLiteral("down"), LogonKeySequence::Key_Down},
		{QStringLiteral("left"), LogonKeySequence::Key_Left},
		{QStringLiteral("right"), LogonKeySequence::Key_Right},
		{QStringLiteral("home"), LogonKeySequence::Key_Home},
		{QStringLiteral("end"), LogonKeySequence::Key_End},
		{QStringLiteral("pageup"), LogonKeySequence::Key_PageUp},
		{QStringLiteral("pagedown"), LogonKeySequence::Key_PageDown},
		{QStringLiteral("insert"), LogonKeySequence::Key_Insert},
	};
	return names;
}



static const QHash<QString, LogonKeySequence::KeyCode>& modifierNames()
{
	static const QHash<QString, LogonKeySequence::KeyCode> names{
		{QStringLiteral("shift"), LogonKeySequence::Key_Shift},
		{QStringLiteral("ctrl"), LogonKeySequence::Key_Control},
		{QStringLiteral("control"), LogonKeySequence::Key_Control},
		{QStringLiteral("alt"), LogonKeySequence::Key_Alt},
		{QStringLiteral("win"), LogonKeySequence::Key_Super},
	};
	return names;
}



static bool parseKey(const QString& name, LogonKeySequence::KeyCode* key)
{
	const auto lowerName = name.toLower();

	if (const auto it = keyNames().constFind(lowerName); it != keyNames().constEnd())
	{
		*key = it.value();
		return true;
	}

	// F1 - F12
	static const QRegularExpression functionKey(QStringLiteral("^f([1-9]|1[0-2])$"));
	if (const auto match = functionKey.match(lowerName); match.hasMatch())
	{
		*key = LogonKeySequence::Key_F1 + static_cast<LogonKeySequence::KeyCode>(match.captured(1).toInt() - 1);
		return true;
	}

	// single letters/digits (lower case keysyms so no implicit Shift is generated)
	if (lowerName.size() == 1 && (lowerName[0].isLetterOrNumber()) && lowerName[0].unicode() < 0x80)
	{
		*key = lowerName[0].unicode();
		return true;
	}

	return false;
}



bool LogonKeySequence::parse(const QString& sequence, Steps* steps, QString* errorMessage)
{
	static const QRegularExpression separators(QStringLiteral("[,\\s]+"));
	static const QRegularExpression delayPattern(QStringLiteral("^(?:wait|sleep|delay)[:=]?(\\d+)$"),
												 QRegularExpression::CaseInsensitiveOption);

	const auto setError = [errorMessage](const QString& message) {
		if (errorMessage)
		{
			*errorMessage = message;
		}
		return false;
	};

	Steps result;

	const auto tokens = sequence.split(separators, Qt::SkipEmptyParts);
	for (const auto& token : tokens)
	{
		if (const auto match = delayPattern.match(token); match.hasMatch())
		{
			const auto delay = match.captured(1).toInt();
			if (delay <= 0 || delay > MaximumDelay)
			{
				return setError(QStringLiteral("invalid delay \"%1\" (1-%2 ms)").arg(token).arg(MaximumDelay));
			}
			Step step;
			step.delay = delay;
			result.append(step);
			continue;
		}

		auto parts = token.split(QLatin1Char('+'));
		// reject empty parts such as in "Ctrl+" or "+Tab"
		if (parts.contains(QString()))
		{
			return setError(QStringLiteral("invalid key combination \"%1\"").arg(token));
		}

		Step step;
		const auto keyName = parts.takeLast();
		for (const auto& modifier : std::as_const(parts))
		{
			const auto it = modifierNames().constFind(modifier.toLower());
			if (it == modifierNames().constEnd())
			{
				return setError(QStringLiteral("unknown modifier \"%1\" in \"%2\"").arg(modifier, token));
			}
			if (step.modifiers.contains(it.value()) == false)
			{
				step.modifiers.append(it.value());
			}
		}

		if (parseKey(keyName, &step.key) == false)
		{
			return setError(QStringLiteral("unknown key \"%1\"").arg(keyName));
		}

		result.append(step);
	}

	if (steps)
	{
		*steps = result;
	}

	return true;
}

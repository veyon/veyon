// SPDX-FileCopyrightText: 2026 Veyon Community
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QObject>
#include <QTest>

#include "LogonKeySequence.h"

using Step = LogonKeySequence::Step;

class LogonKeySequenceTest : public QObject
{
	Q_OBJECT

	static Step key(LogonKeySequence::KeyCode key, QList<LogonKeySequence::KeyCode> modifiers = {})
	{
		Step step;
		step.key = key;
		step.modifiers = modifiers;
		return step;
	}

	static Step pause(int delay)
	{
		Step step;
		step.delay = delay;
		return step;
	}

private Q_SLOTS:
	void emptySequence()
	{
		LogonKeySequence::Steps steps{key(1)};
		QVERIFY(LogonKeySequence::parse(QStringLiteral("  "), &steps));
		QVERIFY(steps.isEmpty());
	}

	void parsesKeysModifiersAndDelays()
	{
		LogonKeySequence::Steps steps;
		QString error;
		QVERIFY2(LogonKeySequence::parse(QStringLiteral("Escape, wait:500 shift+TAB Ctrl+Alt+a F12 Enter Sleep=20 7 Win+Up"),
										 &steps, &error), qPrintable(error));

		const LogonKeySequence::Steps expected{
			key(LogonKeySequence::Key_Escape),
			pause(500),
			key(LogonKeySequence::Key_Tab, {LogonKeySequence::Key_Shift}),
			key('a', {LogonKeySequence::Key_Control, LogonKeySequence::Key_Alt}),
			key(LogonKeySequence::Key_F1 + 11),
			key(LogonKeySequence::Key_Return),
			pause(20),
			key('7'),
			key(LogonKeySequence::Key_Up, {LogonKeySequence::Key_Super}),
		};
		QCOMPARE(steps, expected);
	}

	void upperCaseLettersDoNotImplyShift()
	{
		LogonKeySequence::Steps steps;
		QVERIFY(LogonKeySequence::parse(QStringLiteral("A"), &steps));
		QCOMPARE(steps, LogonKeySequence::Steps{key('a')});
	}

	void rejectsInvalidSequences_data()
	{
		QTest::addColumn<QString>("sequence");
		QTest::newRow("unknown key") << QStringLiteral("Tab Foo");
		QTest::newRow("unknown modifier") << QStringLiteral("Hyper+Tab");
		QTest::newRow("dangling modifier") << QStringLiteral("Ctrl+");
		QTest::newRow("leading plus") << QStringLiteral("+Tab");
		QTest::newRow("zero delay") << QStringLiteral("Wait:0");
		QTest::newRow("huge delay") << QStringLiteral("Wait:999999");
		QTest::newRow("F13") << QStringLiteral("F13");
		QTest::newRow("non-ascii") << QStringLiteral("ä");
	}

	void rejectsInvalidSequences()
	{
		QFETCH(QString, sequence);
		LogonKeySequence::Steps steps{key(1)};
		QString error;
		QVERIFY(LogonKeySequence::parse(sequence, &steps, &error) == false);
		QVERIFY(error.isEmpty() == false);
		// output must not be touched on error
		QCOMPARE(steps, LogonKeySequence::Steps{key(1)});
	}
};

QTEST_GUILESS_MAIN(LogonKeySequenceTest)
#include "LogonKeySequenceTest.moc"

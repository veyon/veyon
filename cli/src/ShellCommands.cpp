// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QFile>
#include <QProcess>

#include "CommandLineIO.h"
#include "ShellCommands.h"


ShellCommands::ShellCommands(QObject* parent) :
	QObject(parent),
	m_commands({
		{QStringLiteral("run"), tr("Run command file")},
	})
{
}



QStringList ShellCommands::commands() const
{
	return m_commands.keys();
}



QString ShellCommands::commandHelp(const QString& command) const
{
	return m_commands.value(command);
}



CommandLinePluginInterface::RunResult ShellCommands::handle_main()
{
	QTextStream stream(stdin);

	while (true)
	{
		printf("VEYON> ");

		QString line;
		if (stream.readLineInto(&line) && line != QLatin1String("exit"))
		{
			runCommand(line);
		}
		else
		{
			break;
		}
	}

	return NoResult;
}



CommandLinePluginInterface::RunResult ShellCommands::handle_run(const QStringList& arguments)
{
	QFile scriptFile(arguments.value(0));
	if (scriptFile.exists() == false ||
		scriptFile.open(QFile::ReadOnly | QFile::Text) == false)
	{
		CommandLineIO::error(tr("File \"%1\" does not exist!").arg(scriptFile.fileName()));
		return Failed;
	}

	while (scriptFile.canReadLine())
	{
		runCommand(QString::fromUtf8(scriptFile.readLine()));
	}

	return Successful;
}



void ShellCommands::runCommand(const QString& command)
{
	// ADDED: splitting arguments containing spaces
	QProcess::execute(QCoreApplication::applicationFilePath(),
					  QProcess::splitCommand(command));
}

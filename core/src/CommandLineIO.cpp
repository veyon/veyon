// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CommandLineIO.h"


void CommandLineIO::print(const QString& message)
{
	fprintf(stdout, "%s\n", qUtf8Printable(message));
	fflush(stdout);
}



void CommandLineIO::newline()
{
	putc('\n', stdout);
}



void CommandLineIO::info(const QString& message)
{
	fprintf(stdout, "[%s] %s\n", qUtf8Printable(VeyonCore::tr("INFO")), qUtf8Printable(message));
	fflush(stdout);
}



void CommandLineIO::warning(const QString& message)
{
	fprintf(stderr, "[%s] %s\n", qUtf8Printable(VeyonCore::tr("WARNING")), qUtf8Printable(message));
	fflush(stderr);
}



void CommandLineIO::error(const QString& message)
{
	fprintf(stderr, "[%s] %s\n", qUtf8Printable(VeyonCore::tr("ERROR")), qUtf8Printable(message));
	fflush(stderr);
}



void CommandLineIO::printTable(const CommandLineIO::Table& table, char horizontal, char vertical, char corner)
{
	// determine column count
	int columnCount = table.first.size();
	for (const auto& row : table.second)
	{
		columnCount = qMax(columnCount, row.size());
	}

	// determine maximum width for each column
	QVector<int> columnWidths(columnCount);
	for (int col = 0; col < table.first.size(); ++col)
	{
		columnWidths[col] = qMax(columnWidths[col], table.first[col].size() + 2);
	}

	for (const auto& row : table.second)
	{
		for (int col = 0; col < row.size(); ++col)
		{
			columnWidths[col] = qMax(columnWidths[col], row[col].size() + 2);
		}
	}

	printTableRuler(columnWidths, horizontal, corner);
	printTableRow(columnWidths, vertical, table.first);
	printTableRuler(columnWidths, horizontal, corner);

	for (const auto& row : table.second)
	{
		printTableRow(columnWidths, vertical, row);
	}

	printTableRuler(columnWidths, horizontal, corner);
}



void CommandLineIO::printUsage(const QString& module, const QString& command,
							   const Arguments& mandatoryArguments, const Arguments& optionalArguments)
{
	QStringList arguments;
	for (const auto& argument : mandatoryArguments)
	{
		if (argument.second.isEmpty())
		{
			arguments.append(QStringLiteral("<%1>").arg(argument.first));
		}
		else
		{
			arguments.append(QStringLiteral("%1 <%2>").arg(argument.second, argument.first));
		}
	}

	for (const auto& argument : optionalArguments)
	{
		if (argument.second.isEmpty())
		{
			arguments.append(QStringLiteral("[<%1>]").arg(argument.first));
		}
		else
		{
			arguments.append(QStringLiteral("[%1 <%2>]").arg(argument.second, argument.first));
		}
	}

	newline();
	print(VeyonCore::tr("USAGE"));
	newline();
	print(QStringLiteral("    %1 %2 %3\n").arg(module, command, arguments.join(QLatin1Char(' '))));
}



void CommandLineIO::printDescription(const QString& description)
{
	print(VeyonCore::tr("DESCRIPTION"));
	newline();
	print(QStringLiteral("    %2\n").arg(description));
}



void CommandLineIO::printExamples(const QString& module, const QString& command, const CommandLineIO::Examples& examples)
{
	print(VeyonCore::tr("EXAMPLES"));
	newline();

	for (const auto& example : examples)
	{
		print(QStringLiteral("    * %1:\n\n        %2 %3 %4\n").arg(example.first, module, command, example.second.join(QLatin1Char(' '))));
	}
}



void CommandLineIO::printTableRuler(const CommandLineIO::TableColumnWidths& columnWidths, char horizontal, char corner)
{
	putc(corner, stdout);
	for (const auto& width : columnWidths)
	{
		for (int i = 0; i < width; ++i)
		{
			putc(horizontal, stdout);
		}
		putc(corner, stdout);
	}
	newline();
}



void CommandLineIO::printTableRow(const TableColumnWidths& columnWidths, char vertical, const TableRow& row)
{
	putc(vertical, stdout);
	for (int col = 0; col < columnWidths.size(); ++col)
	{
		const auto cell = row.value(col);
		fprintf(stdout, " %s%c", qUtf8Printable(cell + QString(columnWidths[col] - cell.size() - 1, QLatin1Char(' '))), vertical);
		fflush(stdout);
	}
	newline();
}

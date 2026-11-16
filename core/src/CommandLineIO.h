// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT CommandLineIO
{
public:
	using TableCell = QString;
	using TableHeader = QList<TableCell>;
	using TableRow = QList<TableCell>;
	using TableRows = QVector<TableRow>;
	using TableColumnWidths = QVector<int>;
	using Table = QPair<TableHeader, TableRows>;
	using ArgumentValue = QString;
	using ArgumentSpecifier = QString;
	using Arguments = QList<QPair<ArgumentValue, ArgumentSpecifier>>;
	using Examples = QList<QPair<QString, QStringList>>;

	static void print(const QString& message);
	static void newline();
	static void info(const QString& message);
	static void warning(const QString& message);
	static void error(const QString& message);
	static void printTable(const Table& table, char horizontal = '-', char vertical = '|', char corner = '+');
	static void printUsage(const QString& module, const QString& command,
						   const Arguments& mandatoryArguments, const Arguments& optionalArguments = {});
	static void printDescription(const QString& description);
	static void printExamples(const QString& module, const QString& command, const Examples& examples);

private:
	static void printTableRuler(const TableColumnWidths& columnWidths, char horizontal, char corner);
	static void printTableRow(const TableColumnWidths& columnWidths, char vertical, const TableRow& row);
};

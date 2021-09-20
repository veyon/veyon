/*
 * CommandLineIO.h - text input/output for command line plugins
 *
 * Copyright (c) 2018-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

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
	using Arguments = QMap<ArgumentValue, ArgumentSpecifier>;
	using Examples = QList<QPair<QString, QStringList> >;

	static void print( const QString& message );
	static void newline();
	static void info( const QString& message );
	static void warning( const QString& message );
	static void error( const QString& message );
	static void printTable( const Table& table, char horizontal = '-', char vertical = '|', char corner = '+' );
	static void printUsage( const QString& module, const QString& command,
							const Arguments& mandatoryArguments, const Arguments& optionalArguments = {} );
	static void printDescription( const QString& description );
	static void printExamples( const QString& module, const QString& command, const Examples& examples );

private:
	static void printTableRuler( const TableColumnWidths& columnWidths, char horizontal, char corner );
	static void printTableRow( const TableColumnWidths& columnWidths, char vertical, const TableRow& row );

} ;

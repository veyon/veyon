/*
 * CommandLineIO.h - text input/output for command line plugins
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef COMMAND_LINE_IO_H
#define COMMAND_LINE_IO_H

#include "VeyonCore.h"

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT CommandLineIO
{
public:
	typedef QString TableCell;
	typedef QList<TableCell> TableHeader;
	typedef QList<TableCell> TableRow;
	typedef QVector<TableRow> TableRows;
	typedef QVector<int> TableColumnWidths;
	typedef QPair<TableHeader, TableRows> Table;

	static void print( const QString& message );
	static void info( const QString& message );
	static void error( const QString& message );
	static void printTable( const Table& table, char horizontal = '-', char vertical = '|', char corner = '+' );

private:
	static void printTableRuler( const TableColumnWidths& columnWidths, char horizontal, char corner );
	static void printTableRow( const TableColumnWidths& columnWidths, char vertical, const TableRow& row );

} ;

#endif

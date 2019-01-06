/*
 * CommandLineIO.cpp - text input/output for command line plugins
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

#include "CommandLineIO.h"


void CommandLineIO::print( const QString& message )
{
	printf( "%s\n", qUtf8Printable( message ) );
}



void CommandLineIO::info( const QString &message )
{
	fprintf( stderr, "[INFO] %s\n", qUtf8Printable( message ) );
}



void CommandLineIO::error( const QString& message )
{
	fprintf( stderr, "[ERROR] %s\n", qUtf8Printable( message ) );
}



void CommandLineIO::printTable( const CommandLineIO::Table& table, char horizontal, char vertical, char corner )
{
	// determine column count
	int columnCount = table.first.size();
	for( const auto& row : table.second )
	{
		columnCount = qMax( columnCount, row.size() );
	}

	// determine maximum width for each column
	QVector<int> columnWidths( columnCount );
	for( int col = 0; col < table.first.size(); ++col )
	{
		columnWidths[col] = qMax( columnWidths[col], table.first[col].size()+2 );
	}

	for( const auto& row : table.second )
	{
		for( int col = 0; col < row.size(); ++col )
		{
			columnWidths[col] = qMax( columnWidths[col], row[col].size()+2 );
		}
	}

	printTableRuler( columnWidths, horizontal, corner );
	printTableRow( columnWidths, vertical, table.first );
	printTableRuler( columnWidths, horizontal, corner );

	for( const auto& row : table.second )
	{
		printTableRow( columnWidths, vertical, row );
	}

	printTableRuler( columnWidths, horizontal, corner );
}



void CommandLineIO::printTableRuler( const CommandLineIO::TableColumnWidths& columnWidths, char horizontal, char corner )
{
	printf( "%c", corner );
	for( const auto& width : columnWidths )
	{
		for( int i = 0; i < width; ++i )
		{
			printf( "%c", horizontal );
		}
		printf( "%c", corner );
	}
	printf( "\n" );
}



void CommandLineIO::printTableRow( const TableColumnWidths& columnWidths, char vertical, const TableRow& row )
{
	printf( "%c", vertical );
	for( int col = 0; col < columnWidths.size(); ++col )
	{
		const auto cell = row.value( col );
		printf( " %s%c", qUtf8Printable( cell + QString( columnWidths[col] - cell.size() - 1, ' ' ) ), vertical );
	}
	printf( "\n" );
}

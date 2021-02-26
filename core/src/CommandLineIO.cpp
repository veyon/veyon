/*
 * CommandLineIO.cpp - text input/output for command line plugins
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

#include "CommandLineIO.h"


void CommandLineIO::print( const QString& message )
{
	fprintf( stdout, "%s\n", qUtf8Printable( message ) );
	fflush( stdout );
}



void CommandLineIO::newline()
{
	putc( '\n', stdout );
}



void CommandLineIO::info( const QString &message )
{
	fprintf( stderr, "[%s] %s\n", qUtf8Printable( VeyonCore::tr( "INFO" ) ), qUtf8Printable( message ) );
	fflush( stderr );
}



void CommandLineIO::warning( const QString &message )
{
	fprintf( stderr, "[%s] %s\n", qUtf8Printable( VeyonCore::tr( "WARNING" ) ), qUtf8Printable( message ) );
	fflush( stderr );
}



void CommandLineIO::error( const QString& message )
{
	fprintf( stderr, "[%s] %s\n", qUtf8Printable( VeyonCore::tr( "ERROR" ) ), qUtf8Printable( message ) );
	fflush( stderr );
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



void CommandLineIO::printUsage( const QString& module, const QString& command,
								const Arguments& mandatoryArguments, const Arguments& optionalArguments )
{
	QStringList arguments;
	for( auto it = mandatoryArguments.begin(), end = mandatoryArguments.end(); it != end; ++it )
	{
		if( it.value().isEmpty() )
		{
			arguments.append( QStringLiteral("<%1>").arg( it.key() ) );
		}
		else
		{
			arguments.append( QStringLiteral("%1 <%2>").arg( it.value(), it.key() ) );
		}
	}

	for( auto it = optionalArguments.begin(), end = optionalArguments.end(); it != end; ++it )
	{
		if( it.value().isEmpty() )
		{
			arguments.append( QStringLiteral("[<%1>]").arg( it.key() ) );
		}
		else
		{
			arguments.append( QStringLiteral("[%1 <%2>]").arg( it.value(), it.key() ) );
		}
	}

	newline();
	print( VeyonCore::tr( "USAGE") );
	newline();
	print( QStringLiteral("    %1 %2 %3\n").arg( module, command, arguments.join(QLatin1Char(' ')) ) );
}



void CommandLineIO::printDescription( const QString& description )
{
	print( VeyonCore::tr( "DESCRIPTION") );
	newline();
	print( QStringLiteral("    %2\n").arg( description ) );
}



void CommandLineIO::printExamples( const QString& module, const QString& command, const CommandLineIO::Examples& examples )
{
	print( VeyonCore::tr( "EXAMPLES") );
	newline();

	for( const auto& example : examples )
	{
		print( QStringLiteral("    * %1:\n\n        %2 %3 %4\n").arg( example.first, module, command,
														   example.second.join(QLatin1Char(' ')) ) );
	}
}



void CommandLineIO::printTableRuler( const CommandLineIO::TableColumnWidths& columnWidths, char horizontal, char corner )
{
	putc( corner, stdout );
	for( const auto& width : columnWidths )
	{
		for( int i = 0; i < width; ++i )
		{
			putc( horizontal, stdout );
		}
		putc( corner, stdout );
	}
	newline();
}



void CommandLineIO::printTableRow( const TableColumnWidths& columnWidths, char vertical, const TableRow& row )
{
	putc( vertical, stdout );
	for( int col = 0; col < columnWidths.size(); ++col )
	{
		const auto cell = row.value( col );
		fprintf( stdout, " %s%c", qUtf8Printable( cell + QString( columnWidths[col] - cell.size() - 1, QLatin1Char(' ') ) ), vertical );
		fflush( stdout );
	}
	newline();
}

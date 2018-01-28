/*
 * CommandLineIO.cpp - text input/output for command line plugins
 *
 * Copyright (c) 2018 Tobias Junghans <tobydox@users.sf.net>
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
	printf( "%s\n", qPrintable( message ) );
}



void CommandLineIO::info( const QString &message )
{
	printf( "[INFO] %s\n", qPrintable( message ) );
}



void CommandLineIO::error( const QString& message )
{
	printf( "[ERROR] %s\n", qPrintable( message ) );
}

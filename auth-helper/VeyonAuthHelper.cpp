/*
 * VeyonAuthHelper.cpp - main file for Veyon Authentication Helper
 *
 * Copyright (c) 2010-2017 Tobias Junghans <tobydox@users.sf.net>
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

#include <QDataStream>
#include <QFile>

#include <security/pam_appl.h>


static char* pam_username = nullptr;
static char* pam_password = nullptr;

static int pam_conv( int num_msg, const struct pam_message **msg,
						struct pam_response **resp,
						void *appdata_ptr )
{
	struct pam_response *reply = nullptr;

	reply = (pam_response *) malloc( sizeof(struct pam_response) * num_msg );
	if( !reply )
	{
		return PAM_CONV_ERR;
	}

	for( int replies = 0; replies < num_msg; ++replies )
	{
		switch( msg[replies]->msg_style )
		{
			case PAM_PROMPT_ECHO_ON:
				reply[replies].resp_retcode = PAM_SUCCESS;
				reply[replies].resp = pam_username;
				break;
			case PAM_PROMPT_ECHO_OFF:
				reply[replies].resp_retcode = PAM_SUCCESS;
				reply[replies].resp = pam_password;
				break;
			case PAM_TEXT_INFO:
			case PAM_ERROR_MSG:
				reply[replies].resp_retcode = PAM_SUCCESS;
				reply[replies].resp = nullptr;
				break;
			default:
				free( reply );
				return PAM_CONV_ERR;
		}
	}

	*resp = reply;
	return PAM_SUCCESS;
}


int main()
{
	QString username, password;
	QFile stdIn;
	stdIn.open( 0, QFile::ReadOnly );
	QDataStream ds( &stdIn );
	ds >> username;
	ds >> password;

	pam_username = qstrdup( username.toUtf8().constData() );
	pam_password = qstrdup( password.toUtf8().constData() );

	struct pam_conv pconv = { &pam_conv, nullptr };
	pam_handle_t *pamh;
	int err = pam_start( "su", nullptr, &pconv, &pamh );

	err = pam_authenticate( pamh, PAM_SILENT );
	if( err == PAM_SUCCESS )
	{
		pam_open_session( pamh, PAM_SILENT );
		pam_end( pamh, err );
		return 0;
	}
	else
	{
		printf( "%s\n", pam_strerror( pamh, err ) );
	}

	pam_end( pamh, err );

	delete[] pam_username;
	delete[] pam_password;

	return -1;
}

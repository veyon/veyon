#include <cstdlib>

#include <QtCore/QDataStream>
#include <QtCore/QFile>

#include <security/pam_appl.h>


static QString username, password;

static int pam_conv( int num_msg, const struct pam_message **msg,
						struct pam_response **resp,
						void *appdata_ptr )
{
	struct pam_response *reply = NULL;

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
				reply[replies].resp = strdup( username.toUtf8().constData() );
				break;
			case PAM_PROMPT_ECHO_OFF:
				reply[replies].resp_retcode = PAM_SUCCESS;
				reply[replies].resp = strdup( password.toUtf8().constData() );
				break;
			case PAM_TEXT_INFO:
			case PAM_ERROR_MSG:
				reply[replies].resp_retcode = PAM_SUCCESS;
				reply[replies].resp = NULL;
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
	QFile stdIn;
	stdIn.open( 0, QFile::ReadOnly );
	QDataStream ds( &stdIn );
	ds >> username;
	ds >> password;

	struct pam_conv pconv = { &pam_conv, NULL };
	pam_handle_t *pamh;
	int err = pam_start( "su", NULL, &pconv, &pamh );

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

	return -1;
}

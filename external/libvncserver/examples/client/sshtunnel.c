/**
 * @example sshtunnel.c
 * An example of an RFB client tunneled through SSH by using https://github.com/bk138/libsshtunnel
 */

#include <rfb/rfbclient.h>
#include <libsshtunnel.h>
#include <signal.h>

/* The one global bool that's global so we can set it via
   a signal handler... */
int maintain_connection = 1;

void intHandler(int dummy) {
    maintain_connection = 0;
}

void ssh_signal_error(void *client,
		      ssh_tunnel_error_t error_code,
		      const char *error_message) {
    fprintf(stderr, "libsshtunnel error: %s", error_message);
}

int ssh_fingerprint_check(void* client,
			  const char *fingerprint,
			  int fingerprint_len,
                          const char *host) {
    fprintf(stderr, "ssh_fingerprint_check: host %s has ", host);
    for(int i = 0; i < fingerprint_len; i++)
	printf("%02X ", (unsigned char)fingerprint[i]);
    printf("\n");

    return 0;
}


int main(int argc, char *argv[])
{
    rfbClient *client = rfbGetClient(8,3,4);

    /*
      Get args and create SSH tunnel
     */
    int rc = ssh_tunnel_init();
    if(rc) {
        fprintf(stderr, "Tunnel initialization failed (%d)\n", rc);
        return EXIT_FAILURE;
    }

    ssh_tunnel_t *data;
    if (argc == 6) {
	/* SSH tunnel w/ password */
	data = ssh_tunnel_open_with_password(argv[1], argv[2], argv[3], argv[4], atoi(argv[5]), client, ssh_fingerprint_check, ssh_signal_error);
    } else if (argc == 7) {
	/* SSH tunnel w/ privkey */
	FILE *privkey_file;
        char *privkey_buffer;
	long privkey_buffer_len;

	// Open the file in binary mode for reading
	privkey_file = fopen(argv[3], "rb");

	if (privkey_file == NULL) {
	    perror("Error opening privkey");
	    return EXIT_FAILURE;
	}

	// Determine the size of the file
	fseek(privkey_file, 0, SEEK_END);
	privkey_buffer_len = ftell(privkey_file);
	fseek(privkey_file, 0, SEEK_SET);

	if (privkey_buffer_len == -1) {
	    perror("Error getting file size of privkey");
	    fclose(privkey_file);
	    return EXIT_FAILURE;
	}

	// Allocate memory for the buffer
	privkey_buffer = (char *)malloc(privkey_buffer_len);

	if (privkey_buffer == NULL) {
	    perror("Error allocating memory for privkey");
	    fclose(privkey_file);
	    return EXIT_FAILURE;
	}

	// Read the content of the file into the buffer
	fread(privkey_buffer, 1, privkey_buffer_len, privkey_file);

	if (ferror(privkey_file) != 0) {
	    perror("Error reading privkey");
	    fclose(privkey_file);
	    free(privkey_buffer);
	    return EXIT_FAILURE;
	}

	// Close the file
	fclose(privkey_file);

	data = ssh_tunnel_open_with_privkey(argv[1], argv[2], privkey_buffer, privkey_buffer_len, argv[4], argv[5], atoi(argv[6]), client, ssh_fingerprint_check, ssh_signal_error);

	free(privkey_buffer);

    } else {
	fprintf(stderr,
		"Usage (w/ password): %s <ssh-server-host> <ssh-server-username> <ssh-server-password> <rfb-host> <rfb-port>\n"
		"Usage (w/ privkey):  %s <ssh-server-host> <ssh-server-username> <privkey_filename> <privkey_password> <rfb-host> <rfb-port>\n",
		argv[0], argv[0]);
	return(EXIT_FAILURE);
    }


    /*
      The actual VNC connection setup.
     */
    client->serverHost = strdup("127.0.0.1");
    if(data) // might be NULL if ssh setup failed
	client->serverPort = ssh_tunnel_get_port(data);
    rfbClientSetClientData(client, (void*)42, data);

    if (!data || !rfbInitClient(client,NULL,NULL))
	return EXIT_FAILURE;

    printf("Successfully connected to %s:%d - hit Ctrl-C to disconnect\n", client->serverHost, client->serverPort);

    signal(SIGINT, intHandler);

    while (maintain_connection) {
	int n = WaitForMessage(client,50);
	if(n < 0)
	    break;
	if(n)
	    if(!HandleRFBServerMessage(client))
		break;
    }

    /* Disconnect client inside tunnel */
    if(client && client->sock != RFB_INVALID_SOCKET)
	    rfbCloseSocket(client->sock);

    /* Close the tunnel and clean up */
    ssh_tunnel_close(rfbClientGetClientData(client, (void*)42));

    /* free client */
    rfbClientCleanup(client);

    /* Teardown ssh tunnel machinery */
    ssh_tunnel_exit();

    return EXIT_SUCCESS;
}


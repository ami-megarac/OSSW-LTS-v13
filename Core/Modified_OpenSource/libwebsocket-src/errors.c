/******************************************************************************
  Copyright (c) 2013 Morten Houmøller Nygaard - www.mortz.dk - admin@mortz.dk
 
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#include "errors.h"

/**
 * This function is called when the server experience an error. This will
 * free the necessary allocations and then shutdown the server.
 */
void server_error(const char *message, int server_socket, ws_list *l) {
	shutdown(server_socket, SHUT_RD);

	printf("\nServer experienced an error: \n\t%s\nShutting down ...\n\n", 
			message);
	fflush(stdout);

	if (l != NULL) {
		list_free(l);
		l = NULL;
	}

	close(server_socket);
	exit(EXIT_FAILURE);
}

/**
 * This function is called when some error happens during the handshake. 
 * This will free all the allocations done by the specific client, send a 
 * http error status to the client, and then shut the TCP connection between 
 * client and server down.
 *
 * @param type(const char *) message [The error message for the server]
 * @param type(const char *) status [The http error status code]
 * @param type(ws_client *) n [Client]
 */
void handshake_error(const char *message, const char *status, ws_client *n) {
	int ret=0;
	printf("\nClient experienced an error: \n\t%s\nShutting him down ...\n\n", 
			message);
	fflush(stdout);	

	ret=send(n->socket_id, status, strlen(status), 0);
	if(ret == -1)
	{
		printf("Error in send function\n");
		goto EXIT;
	}
	shutdown(n->socket_id, SHUT_RDWR);
EXIT:
	if (n != NULL) /* SCA Fix [Null pointer dereferences]:: False Positive */
 /* Reason for False Positive - Its null pointer check condition, NULL pointer check is must for avoiding the NULL pointer dereferences*/
	{
		client_free(n);
		close(n->socket_id);
		free(n);
		n = NULL;
	}
}

/**
 * This function is called when the client experience an error. This will
 * free all the allocations done by the specific client, send a closing frame
 * to the client, and then shut the TCP connection between client and server
 * down.
 *
 * @param type(const char *) message [The error message for the server]
 * @param type(ws_connection_close) c [The status of the closing]
 * @param type(ws_client *) n [Client] 
 */
void client_error(const char *message, ws_connection_close c, ws_client *n) {
	printf("\nClient experienced an error: \n\t%s\nShutting him down ...\n\n", 
			message);
	fflush(stdout);	
	
	ws_closeframe(n, c);
	shutdown(n->socket_id, SHUT_RDWR);

	if (n != NULL) /* SCA Fix [Null pointer dereferences]:: False Positive */
	/* Reason for False Positive - Its null pointer check condition, NULL pointer check is must for avoiding the NULL pointer dereferences*/ 
	{
		client_free(n);
		close(n->socket_id);
		free(n);
		n = NULL;
	}
}

/**
 * This function is called when serial port is busy. This will
 * free all the allocations done by the specific client, send a closing 
 * frame to the client as well as some info, and then shut the TCP connection 
 * between client and server down.
 *
 * @param type(const char *) message [The error message which will send to client]
 * @param type(ws_connection_close) c [The status of the closing]
 * @param type(ws_client *) n [Client]
 *
 * retval  0 succeed
 * retval -1 failed 
 */
int serial_error(const char *message, ws_connection_close c, ws_client *n) {
	ws_connection_close status;
	ws_message *m = message_new();
	m->len = strlen(message);

	char *temp = malloc( sizeof(char)*(m->len+1) );
	if (temp == NULL) {
		message_free(m);
		free(m);
		return -1;
	}   
	memset(temp, '\0', (m->len+1));
	memcpy(temp, message, m->len);
	m->msg = temp;
	temp = NULL;

	if ( (status = encodeMessage(m)) != CONTINUE) {
		message_free(m);
		free(m);
		return -1;
	}   
	ws_send(n, m); 
	message_free(m);
	free(m);

	ws_closeframe(n, c);
	shutdown(n->socket_id, SHUT_RDWR);

	if (n != NULL) /* SCA Fix [Null pointer dereferences]:: False Positive */
    /* Reason for False Positive - Its null pointer check condition, NULL pointer check is must for avoiding the NULL pointer dereferences*/
	{
		client_free(n);
		close(n->socket_id);
		free(n);
		n = NULL;
	}
	return 0;
}

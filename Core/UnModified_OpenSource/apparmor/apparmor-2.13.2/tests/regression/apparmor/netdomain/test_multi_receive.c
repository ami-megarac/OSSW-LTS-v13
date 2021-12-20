/* Multiple iteration sending test. */

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

int receive_udp(char *bind_ip, char *bind_port);
int receive_tcp(char *bind_ip, char *bind_port);


int main(int argc, char *argv[])
{
	int ret;
	
	if (argc < 4)
	{
		printf("Usage: %s bind_ip bind_port proto\n", argv[0]);
		exit(1);
	}

	if (strcmp(argv[3], "udp") == 0)
	{
		ret = receive_udp(argv[1], argv[2]);
	}
	else if (strcmp(argv[3], "tcp") == 0)
	{
		ret = receive_tcp(argv[1], argv[2]);
	}
	else
	{
		printf("Unknown protocol.\n");
	}

	if (ret == -1)
	{
		printf("Receive message failed.\n");
		exit(1);
	}

	exit(0);
}

int receive_udp(char *bind_ip, char *bind_port)
{

	int sock;
	char *buf;
	struct sockaddr_in remote, local;
	int ret = -1;
	int select_return;

	fd_set read_set, err_set;
	struct timeval timeout;

	buf = (char *) malloc(255);
	memset(buf, '\0', 255);	
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("Socket error: ");
		return(-1);
	}

	local.sin_family  = AF_INET;
	local.sin_port = htons(atoi(bind_port));
	inet_aton(bind_ip, &local.sin_addr);

	if (bind(sock, (struct sockaddr *) &local, sizeof(local)) < 0)
	{
		perror("Bind error: ");
		return(-1);
	}

	FD_ZERO(&read_set);
	FD_SET(sock, &read_set);
	FD_ZERO(&err_set);
	FD_SET(sock, &err_set);
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	select_return = select(sock + 1, &read_set, NULL, &err_set, &timeout);
	if (select_return < 0)
	{
		perror("Select error: ");
		ret = -1;
	}

	 

 	if ((select_return > 0) && (FD_ISSET(sock, &read_set)) && (!FD_ISSET(sock, &err_set)))
	{

		if (recvfrom(sock, buf, 255, 0, (struct sockaddr *)0, (int *)0) >= 1)
		{
			printf("MESSAGE: %s\n", buf);
			ret = 0;
		}
		else
		{
			printf("recvfrom failed\n");
			ret = -1;
		}
	}
	free(buf);
	return(ret);

}

int receive_tcp(char *bind_ip, char *bind_port)
{
	int sock, cli_sock;
	char *buf;
	struct sockaddr_in remote, local;
	int ret = -1;
	int select_return;
	
	fd_set read_set, err_set;
	struct timeval timeout;
	
	buf = (char *) malloc(255);
	memset(buf, '\0', 255);
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Socket error:");
		return(-1);
	}

	local.sin_family = AF_INET;
	local.sin_port = htons(atoi(bind_port));
	inet_aton(bind_ip, &local.sin_addr);

	if (bind(sock, (struct sockaddr *) &local, sizeof(local)) < 0)
	{
		perror("Could not bind.");
		return (-1);
	}

	if (listen(sock, 5) == -1)
	{
		perror("Could not listen: ");
		return(-1);
	}

	FD_ZERO(&read_set);
	FD_SET(sock, &read_set);
	FD_ZERO(&err_set);
	FD_SET(sock, &err_set);
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	select_return = select(sock + 1, &read_set, NULL, &err_set, &timeout);
	if (select_return < 0)
	{
		perror("Select failed: ");
		ret = -1;
	}

	if ((select_return > 0) && (FD_ISSET(sock, &read_set)) && (!FD_ISSET(sock, &err_set)))
	{
		if ((cli_sock = accept(sock, NULL, NULL)) < 0)
		{
			perror("Accept failed: ");
			ret = -1;
		}
		else
		{
                	if (recv(cli_sock, buf, 255, 0) >= 1)
                	{
                       	 	printf("MESSAGE: %s\n", buf);
                        	ret = 0;
                	}
			else
			{
				perror("recv failure: ");
				ret = -1;
			}
		}
	}
	else
	{
		perror("There were select failures: ");
		ret = -1;
	}
	free(buf);
	return(ret);
}

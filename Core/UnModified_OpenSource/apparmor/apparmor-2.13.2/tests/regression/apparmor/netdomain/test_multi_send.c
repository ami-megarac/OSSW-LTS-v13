/* Multiple iteration sending test. */

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

int send_udp(char *bind_ip, char *bind_port, char *remote_ip, char *remote_port, char *message);
int send_tcp(char *bind_ip, char *bind_port, char *remote_ip, char *remote_port, char *message);

int main(int argc, char *argv[])
{
	int send_ret;

	if (argc < 7)
	{
		printf("Usage: %s bind_ip bind_port remote_ip remote_port proto message\n", argv[0]);
		exit(1);
	}

	send_ret = -1;
	if (strcmp(argv[5], "udp") == 0)
	{
		send_ret = send_udp(argv[1], argv[2], argv[3], argv[4], argv[6]);
	}
	else if (strcmp(argv[5], "tcp") == 0)
	{
		send_ret = send_tcp(argv[1], argv[2], argv[3], argv[4], argv[6]);
	}
	else
	{
		printf("Unknown protocol.\n");
	}

	if (send_ret == -1)
	{
		printf("Send message failed.\n");
		exit(1);
	}

	exit(0);
}

int send_udp(char *bind_ip, char *bind_port, char *remote_ip, char *remote_port, char *message)
{
	int sock;
	struct sockaddr_in remote, local;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("Could not open socket: ");
		return(-1);
	}

	remote.sin_family = AF_INET;
	remote.sin_port = htons(atoi(remote_port));
	inet_aton(remote_ip, &remote.sin_addr);

	local.sin_family  = AF_INET;
	local.sin_port = htons(atoi(bind_port));
	inet_aton(bind_ip, &local.sin_addr);

	if (bind(sock, (struct sockaddr *) &local, sizeof(local)) < 0)
	{
		perror("Could not bind: ");
		return(-1);
	}

	printf("Sending \"%s\"\n", message);
	if (sendto(sock, message, strlen(message), 0, (struct sockaddr *) &remote, sizeof(remote)) <= 0)
	{
		perror("Send failed: ");
		return(-1);
	}
	close(sock);
	return(0);
	
}

int send_tcp(char *bind_ip, char *bind_port, char *remote_ip, char *remote_port, char *message)
{
        int sock;
        struct sockaddr_in remote, local;

        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
                perror("Could not open socket: ");
                return(-1);
        }

        remote.sin_family = AF_INET;
        remote.sin_port = htons(atoi(remote_port));
        inet_aton(remote_ip, &remote.sin_addr);

        local.sin_family  = AF_INET;
        local.sin_port = htons(atoi(bind_port));
        inet_aton(bind_ip, &local.sin_addr);

        if (bind(sock, (struct sockaddr *) &local, sizeof(local)) < 0)
        {
                perror("Could not bind: ");
                return(-1);
        }
	if (connect(sock, (struct sockaddr *) &remote, sizeof(remote)) < 0)
	{
		perror("Could not connect: ");
		return(-1);
	}

        printf("Sending \"%s\"\n", message);
        if (send(sock, message, strlen(message), 0) <= 0)
        {
                perror("Send failed: ");
                return(-1);
        }
	close(sock);
        return(0);


}


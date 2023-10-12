#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CLIENTS 32
#define REQ_MSG_BUF_SIZE (1<<10)

//TODO: rethink this whole function an make it actually decent
void handle_request(int client_fd) {
	fprintf(stderr, "handling request by fd %d\n", client_fd);
	char buf[REQ_MSG_BUF_SIZE] = {0};
	ssize_t msglen = recv(client_fd, buf, REQ_MSG_BUF_SIZE, 0);
	if (!msglen) return;
	if (msglen < 0) {
		perror("recv failed");
		return;
	}
	{
		char tmp = buf[5];
		buf[5]=0;
		if (msglen < 6 || strcmp("GET /", buf)) {
			fputs("bad reqest\n", stderr);
			return;
		}
		buf[5] = tmp;
	}
	
	for (int i=5; buf[i]; i++) // delim the path
		if (buf[i]==' ' || buf[i]=='\t' || buf[i]=='\n') {
			buf[i] = 0;
			break;
		}
	char *cntnt_typ = "text/plain"; // default
	for (int i=strlen(buf)-1; i>=0; i--) // get file and mime type
		if (buf[i] == '.') {
			if (!strcmp(buf+i+1, "html"))
				cntnt_typ = "text/html";
			else if (!strcmp(buf+i+1, "css"))
				cntnt_typ = "text/css";
			else if (!strcmp(buf+i+1, "js"))
				cntnt_typ = "application/javascript";
			break;
		}
	printf("\tgetting file \"%s\" - \"%s\"\n", buf+5, cntnt_typ);
	char *header = malloc(strlen(cntnt_typ)+27);
	strcpy(header, "HTTP/2 200\ncontent-type: ");
	strcat(header, cntnt_typ);
	strcat(header, "\n\n");
	
	int f_fd = open(buf+5, O_RDONLY);
	if (f_fd < 0) {
		fprintf(stderr, "cannot read file: %s\n", buf+5);
		return;
	}

	struct stat f_stat;
	if (fstat(f_fd, &f_stat)) {
		fprintf(stderr, "errno %d: cannot read file stats for %s", errno, buf+5);
		return;
	}

	char *fbuf = malloc(strlen(header)+f_stat.st_size+1);
	strcpy(fbuf, header);
	fbuf[read(f_fd, fbuf+strlen(header), f_stat.st_size)+strlen(header)]=0;
	free(header);
	
	close(f_fd);
	send(client_fd, fbuf, strlen(fbuf), 0);
	free(fbuf);
}

int sf;

void closesock(void) {
	close(sf);
}

int main(int argc, char **argv) {
	for (int i=strlen(argv[0]); i>0; i--)
		if (argv[0][i] == '/') {
			argv[0]+=i+1;
			break;
		}
	if (!strcmp(argv[0], "meows-help")) {
		puts("MEOWS - a small & simple web server\n"
			"The program can be invoked as \"meows-help\" to show this mesage.\n"
			"Normal usage: meows [PORT]\t(default is 8080)\n");
		return 0;
	}
	const int portnr = argc>1? atoi(argv[1]):8080;

	int sockfd;
	struct sockaddr_in sockaddr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        return -1;
    }

	sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = htons(portnr);

	if (bind(sockfd, 
            (struct sockaddr *)&sockaddr, 
            sizeof(sockaddr)) < 0) {
        perror("socket binding failed");
        return -1;
    }
	puts("Server started succesfully.");

	if (listen(sockfd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        close(sockfd);
		return -1;
    }
	printf("Server listening on port: %d\n", portnr);
	sf = sockfd;
	atexit(closesock);

	struct sockaddr_in client_addr[MAX_CLIENTS];
	socklen_t client_addrlen[MAX_CLIENTS];
	struct pollfd client_fd[MAX_CLIENTS+1]; // last one is for sockfd
	for (int i=0; i<=MAX_CLIENTS; i++) {
		client_fd[i].fd = -1; // init array, negative fd means free slot
		client_fd[i].events = POLLIN | POLLPRI | POLLERR | POLLHUP;
		client_addrlen[i] = sizeof(client_fd[0]);
	}
	client_fd[MAX_CLIENTS].fd = sockfd;

	while(1) {
		int poll_res = poll(client_fd, MAX_CLIENTS+1, -1);
		if(poll_res < 0) {
			perror("poll error");
			if (poll_res != EAGAIN) break;
			else continue;
		}
		if (client_fd[MAX_CLIENTS].revents & (POLLIN | POLLPRI)) { // new connection
			int i = -1;
			while (++i < MAX_CLIENTS && client_fd[i].fd >= 0); // find free slot
			if (i==MAX_CLIENTS)
				fputs("connection limit reached\n", stderr); // ignore it
			else if((client_fd[i].fd = accept(sockfd,
					(struct sockaddr *)(client_addr+i), client_addrlen+i)) < 0)
				perror("connection accept failure");
		}
		for(int i=0; i<MAX_CLIENTS; i++) {
			struct pollfd *cfd = client_fd+i;
			if (!cfd->revents || cfd->fd < 0) continue;
			if (cfd->revents & (POLLIN | POLLPRI)) { // client sent a request
				handle_request(cfd->fd);
				close(cfd->fd); // for now, we close the connection after responding
				cfd->fd = -1;
			}
			else if (cfd->revents & (POLLHUP | POLLERR)) { // client hung up or errored
				close(cfd->fd);
				cfd->fd = -1;
			}
			else if (cfd->revents && cfd->revents != POLLNVAL) // hould never be the case in theory
				fprintf(stderr, "unknown event: %d\n", cfd->revents);
			cfd->revents = 0;
		}
	}

	puts("Closing the socket and terminating...");
	//close(sockfd);
	return 0;
}

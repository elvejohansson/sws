#include <cerrno>
#include <cstdio>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>

int main() {
    int status;
    addrinfo hints;
    addrinfo *servinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, "8080", &hints, &servinfo)) != 0) {
        fprintf(stderr, "[error] getaddrinfo %s\n", gai_strerror(status));
        exit(1);
    }

    int sockfd;
    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
        fprintf(stderr, "[error] could not get socket descriptor %s\n", strerror(errno));
        exit(1);
    }

    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET,SO_REUSEADDR, &yes, sizeof yes) == -1) {
        perror("setsockopt");
        exit(1);
    }

    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        fprintf(stderr, "[error] could not bind socket %s\n", strerror(errno));
        exit(1);
    }

    int backlog = 10;
    if (listen(sockfd, backlog)) {
        fprintf(stderr, "[error] could not listen on socket %s\n", strerror(errno));
        exit(1);
    }

    printf("[info] listening on :8080\n");

    struct sockaddr_storage their_address;
    socklen_t address_size = sizeof their_address;
    int accept_fd = accept(sockfd, (sockaddr *)&their_address, &address_size);

    char buffer[1024];
    int value = recv(accept_fd, buffer, 1024, 0);

    printf("%s", buffer);

    std::string response = "HTTP/1.1 200 OK\nServer: sws\n\nOK";
    send(accept_fd, response.c_str(), response.length(), 0);

    freeaddrinfo(servinfo);

    return 0;
}

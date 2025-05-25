#include "parser.h"
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

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

    using clock = std::chrono::steady_clock;

    while (true) {
        struct sockaddr_storage their_address;
        socklen_t address_size = sizeof their_address;
        int accept_fd = accept(sockfd, (sockaddr *)&their_address, &address_size);

        std::chrono::time_point<clock> before = clock::now();

        printf("[debug] accepting connection\n");

        char buffer[1024];
        int value = recv(accept_fd, buffer, 1024, 0);

        Request* message = parse(buffer);

        Response response;
        response.version = message->version;
        response.code = StatusCode::OK;
        response.path = message->path;
        response.data = "THE STUFF";

        std::string serialized_response = response_tostring(&response);
        send(accept_fd, serialized_response.c_str(), serialized_response.length(), 0);

        std::chrono::time_point<clock> after = clock::now();
        double seconds = std::chrono::duration<double>(after - before).count();

        printf("[debug] request processing took %.1f ms\n", seconds * 1'000);
        printf("[info] %s %s\n", method_tostring(message->method).c_str(), message->path.c_str());

        close(accept_fd);
    }

    freeaddrinfo(servinfo);

    return 0;
}

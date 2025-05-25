#include "parser.h"
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

std::string get_content_type_for_file_path(std::string file_path) {
    std::size_t found = file_path.find_last_of(".");
    std::string extension = file_path.substr(found + 1, file_path.length());

    if (extension == "html") {
        return "text/html";
    } else if (extension == "js") {
        return "text/javascript";
    } else if (extension == "css") {
        return "text/css";
    } else if (extension == "webp") {
        return "image/webp";
    } else if (extension == "svg") {
        return "image/svg+xml";
    } else if (extension == "ico") {
        return "image/vnd.microsoft.icon";
    } else {
        // RFC 2046, section 4.5.1 states: The "octet-stream" subtype is used
        // to indicate that a body contains arbitrary binary data
        return "application/octet-stream";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[error] directory path is required at argument 1, usage: sws <path>\n");
        exit(1);
    }
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

    std::string directory_path = argv[1];

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

        if (message->path == "/") {
            message->path = "/index.html";
        } else if (message->path[message->path.length() - 1] == '/') {
            message->path += "index.html";
        }

        std::string file_path = directory_path + message->path;

        Response response;
        response.version = message->version;
        response.path = message->path;
        response.headers.push_back({"Server", "sws"});

        if (!std::filesystem::exists(file_path)) {
            response.code = StatusCode::NOT_FOUND;
            response.headers.push_back({"Content-Type", "text/html; charset=utf-8"});
            response.data = "<!DOCTYPE html><html><head><title>404 - Not Found</title></head><body><h1>Not Found</h1><p>No resource found at requested URL.</p><hr></hr><p><i>sws/alpha</i></p></body></html>";
        } else {
            std::ifstream file(file_path);

            response.code = StatusCode::OK;
            std::string type = get_content_type_for_file_path(message->path);
            response.headers.push_back({"Content-Type", type});

            std::string line;
            while (std::getline(file, line)) {
                response.data += line;
            }
        }

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

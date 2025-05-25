#include "parser.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>

std::string method_tostring(Method method) {
    if (method == Method::GET) {
        return "GET";
    } else {
        return "N/A";
    }
}

int code_for_status_code(StatusCode code) {
    switch (code) {
    case StatusCode::OK:
        return 200;
    case StatusCode::NOT_FOUND:
        return 404;
    }
}

std::string text_for_status_code(StatusCode code) {
    switch (code) {
    case StatusCode::OK:
        return "OK";
    case StatusCode::NOT_FOUND:
        return "Not Found";
    }
}

Request* parse(std::string data) {
    Request* request;
    request = (Request*)malloc(sizeof(Request));

    std::string first_line = data.substr(0, data.find("\r"));

    std::string method = first_line.substr(0, first_line.find(" "));

    if (method == "GET") {
        request->method = Method::GET;
    } else {
        fprintf(stderr, "[error] unrecognized method %s\n", method.c_str());
    }

    first_line.erase(0, first_line.find(" ") + 1);

    std::string path = first_line.substr(0, first_line.find(" "));
    request->path = path;
    first_line.erase(0, first_line.find(" ") + 1);

    request->version = first_line.substr(first_line.find("/") + 1, first_line.length());

    // writing this like a language lexer for now, could probably make this
    // more ergonomic with some better string processing functions
    std::string header_data = data.substr(data.find("\r") + 2, data.length());
    std::string header_token;
    std::string header_value;
    for (int i = 0; i < header_data.length(); i++) {
        char current_char = header_data.at(i);

        if (std::isalpha(current_char) != 0) {
            header_token.push_back(current_char);
            i++;

            while (std::isalpha(header_data.at(i)) || header_data.at(i) == '-') {
                header_token.push_back(header_data.at(i));
                i++;
            }

            if (header_data.at(i) != ':') {
                printf("%s\n", header_data.c_str());
                fprintf(stderr, "[error] malformed header, at character %c\n", header_data.at(i));
                header_token = "";
                header_value = "";
                i++;
                continue;
            }

            i++;

            while (header_data.at(i) != '\r' && header_data.at(i + 1) != '\n') {
                if (std::isspace(header_data.at(i))) {
                    i++;
                    continue;
                }

                header_value.push_back(header_data.at(i));
                i++;
            }

            request->headers.push_back(std::tuple{header_token, header_value});
            header_token = "";
            header_value = "";
        }
    }

    request->data = data;

    return request;
}

std::string response_tostring(Response* response) {
    std::string buffer;
    buffer += "HTTP/";
    buffer += response->version;
    buffer += " ";
    buffer += std::to_string(code_for_status_code(response->code));
    buffer += " ";
    buffer += text_for_status_code(response->code);

    buffer += "\nServer: sws";
    buffer += "\nContent-Length: " + std::to_string(response->data.length());
    buffer += "\nContent-Type: text/html; charset=utf-8";
    buffer += "\n\n";

    buffer += response->data;

    return buffer;
}

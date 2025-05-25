#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <tuple>
#include <vector>

enum Method {
    GET,
};

enum StatusCode {
    OK,
};

struct Request {
    Method method;
    std::string path;
    std::string version;
    std::vector<std::tuple<std::string, std::string>> headers;
    std::string data;
};

struct Response {
    std::string version;
    StatusCode code;
    std::string path;

    std::vector<std::tuple<std::string, std::string>> headers;
    std::string data;
};

std::string method_tostring(Method method);

int code_for_status_code(StatusCode code);

std::string text_for_status_code(StatusCode code);

Request* parse(std::string data);

std::string response_tostring(Response* response);

#endif

#include "errors.h"
#include "str.h"
#include <arpa/inet.h>
#include <asm-generic/ioctls.h>
#include <asm-generic/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFLEN 1024

const char *base_response = "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: %d\r\n"
                            "\r\n";

const char *ok_response = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 2\r\n"
                          "\r\n"
                          "OK";

const char *bad_request = "HTTP/1.1 400 Bad Request\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 48\r\n"
                          "\r\n"
                          "Length of the request must not exceed 1024 bytes";

int handle_request(int cfd);

int main(void)
{
    // 1. creating an socket file
    // 2. binding an address to the socket
    // 3. specifying the number of awaiting connections
    // 4. accepting the connections
    // 5. printing out the client and the server address
    // 6. making a JSON response

    printf("Hello, HTTP server\n");
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sfd >= 0, "failed to create a socket file");

    int optval = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct addrinfo hints;
    struct addrinfo *result, *resp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_protocol = 0;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    int status = getaddrinfo(NULL, "8090", &hints, &result);
    assert(status == 0, "failed to get address info");

    char host[INET_ADDRSTRLEN] = {0};
    uint16_t port = 0;
    for (resp = result; resp != NULL; resp = resp->ai_next) {
        if (resp->ai_family == AF_INET) {
            status = bind(sfd, resp->ai_addr, resp->ai_addrlen);
            assert(status == 0, "failed to bind the address");

            struct sockaddr_in *addr = (struct sockaddr_in *)resp->ai_addr;
            const char *dst = inet_ntop(AF_INET, &addr->sin_addr, host, INET_ADDRSTRLEN);
            assert(dst != NULL, "inet_ntop");

            port = ntohs(addr->sin_port);

            break;
        }
    }

    assert(resp != NULL, "failed to resolve an address");
    freeaddrinfo(result);

    status = listen(sfd, 10);
    assert(status == 0, "failed on listen");

    printf("Listening on '%s:%d'\n", host, port);

    struct sockaddr_in caddr;
    unsigned int caddr_len = sizeof(struct sockaddr_in);

    while (1) {
        int cfd = accept(sfd, (struct sockaddr *)&caddr, &caddr_len);
        assert(cfd >= 0, "failed to accept the connection");

        struct sockaddr_in *addr_in = (struct sockaddr_in *)&caddr;
        inet_ntop(AF_INET, (char *)&(addr_in->sin_addr), host, INET_ADDRSTRLEN);
        printf("Client Address: %s:%d\n", host, htons(addr_in->sin_port));

        status = handle_request(cfd);
        assert(status == 0, "failed to handle request");

        close(cfd);
    }
}

int handle_request(int cfd)
{
    // {
    //  "id_line": "GET / HTTP/1.1",
    //  "headers": [
    //     "Client Address: 127.0.0.1:33820"
    //     "The received data: GET / HTTP/1.1"
    //     "Host: localhost:8080"
    //     "User-Agent: curl/7.68.0"
    //     "Accept: */*"
    //  ],
    //  "body": "Hello"
    // }
    char req[BUFLEN] = {0};
    int numread;
    int numwrite;
    numread = read(cfd, req, BUFLEN);
    assert(numread, "failed to read the data");

    int remaining_bytes = 0;
    int status = ioctl(cfd, FIONREAD, &remaining_bytes);
    assert(status == 0, "ioctl");
    if (remaining_bytes > 0) {
        numwrite = write(cfd, bad_request, strlen(bad_request));
        assert(numwrite, "bad request write");
        return 0;
    }

    char buf[BUFLEN] = {0};

    string *body = newstr(buf);
    append(body, "{\"id_line\":\"");
    memset(buf, 0, BUFLEN);

    int ri = 0;
    int crfound;
    int bi;
    for (bi = crfound = 0; ri < BUFLEN && req[ri] != '\0'; ri++) {
        if (req[ri] == '\r') {
            crfound = 1;
            continue;
        }
        if (crfound && req[ri] == '\n') {
            append(body, buf);
            ri++;
            break;
        }
        buf[bi++] = req[ri];
    }
    append(body, "\",");

    append(body, "\"headers\":[");
    memset(buf, 0, BUFLEN);
    int linenumber = 0;
    for (bi = crfound = 0; ri < BUFLEN && req[ri] != '\0'; ri++) {
        if (req[ri] == '\r') {
            crfound = 1;
            continue;
        }
        if (crfound && req[ri] == '\n') {
            if (buf[0] == '\0') {
                ri++;
                break;
            }
            if (linenumber) append(body, ",");
            append(body, "\"");
            append(body, buf);
            append(body, "\"");
            memset(buf, bi = 0, BUFLEN);
            linenumber++;
            continue;
        }
        buf[bi++] = req[ri];
    }
    append(body, "],");

    append(body, "\"body\":\"");
    for (bi = crfound = 0; ri < BUFLEN && req[ri] != '\0'; ri++) {
        buf[bi++] = req[ri];
    }
    append(body, buf);
    append(body, "\"");

    append(body, "}");

    memset(buf, 0, BUFLEN);
    sprintf(buf, base_response, body->len);

    string *response = newstr(buf);
    append(response, (const char *)body->data);

    numwrite = write(cfd, response->data, response->len);
    assert(numwrite, "failed to write the data");

    freestr(body);
    freestr(response);

    return 0;
}

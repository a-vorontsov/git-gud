#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <ctype.h>
#include <zlib.h>

// TODO:
// api.stackexchange.com

char rfc3986[256] = {0};
char html5[256] = {0};

long unsigned int MAX_SIZE = 200000;

void url_encoder_rfc_tables_init() {
    int i;
    for (i = 0; i < 256; i++) {

        rfc3986[i] = isalnum(i) || i == '~' || i == '-' || i == '.' || i == '_' ? i : 0;
        html5[i] = isalnum(i) || i == '*' || i == '-' || i == '.' || i == '_' ? i : (i == ' ') ? '+' : 0;
    }
}

char *url_encode( char *table, unsigned char *s, char *enc) {
    for (; *s; s++) {
        if (table[*s]) {
            sprintf(enc, "%c", table[*s]);
        } else {
            sprintf(enc, "%%%02X", *s);
        }
        while (*++enc);
    }

    return(enc);
}

char *sendReq(char *route) {
    char firstHalf[500] = "api.stackexchange.com";
    char *secondHalf = route;
    char request[MAX_SIZE];
    struct hostent *server;
    struct sockaddr_in serveraddr;
    int port = 80;

    int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);

    server = gethostbyname(firstHalf);

    memset((char *) &serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;

    memmove((char *)&serveraddr.sin_addr.s_addr, (char *)server->h_addr_list[0], server->h_length);

    serveraddr.sin_port = htons(port);

    if (connect(tcpSocket, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
        printf("Error Connecting\n");
    }
    memset(request, 0, MAX_SIZE);

    sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\nAccept: */*\r\nAccept-Encoding: deflate\r\n\r\n", secondHalf, firstHalf);

    if (send(tcpSocket, request, strlen(request), 0) < 0) {
        fprintf(stderr, "Error with send()");
    }
    memset(request, 0, MAX_SIZE);

    if (recv(tcpSocket, request, MAX_SIZE-1, 0) < 0) {
        fprintf(stderr, "Error with recv()");
    }

    close(tcpSocket);

    char *data = strstr(request, "\r\n\r\n");
    if (data != NULL) {
        data += 4;
    }

    unsigned char *c = malloc(MAX_SIZE);

    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    // setup "b" as the input and "c" as the compressed output
    infstream.avail_in = (uInt)strlen(data); // size of input
    infstream.next_in = (Bytef *)data; // input char array

    infstream.avail_out = (uInt)MAX_SIZE; // size of output
    infstream.next_out = c; // output char array

    // the actual decompression work.
    if (inflateInit2(&infstream, -MAX_WBITS) != Z_OK) {
        fprintf(stderr, "Error with inflateInit2()");
    }
    int inf = inflate(&infstream, Z_NO_FLUSH);
    printf("\ninflate: %d\n", inf);
    inflateEnd(&infstream);
    return (char *)c;
}

void getQuestion(char question[1024]) {
    char request[1536];
    char urlEncoded[1200];
    url_encoder_rfc_tables_init();
    url_encode(rfc3986, (unsigned char*) question, urlEncoded);
    sprintf(request,
            "/2.2/search/excerpts?key=U4DMV*8nvpm3EOpvf69Rxw((&pagesize=1&order=desc&sort=relevance&q=%s&site=stackoverflow&filter=!1zSijXI74x1547R0kRXdT",
            urlEncoded);
    char *response = sendReq(request);
    printf("\noutput: %s\n", response);
}

void getAnswer(char question[1024]) {
    char request[1536];
    char urlEncoded[1200];
    url_encoder_rfc_tables_init();
    url_encode(rfc3986, (unsigned char*) question, urlEncoded);
    sprintf(request,
            "/2.2/questions/%s/answers?order=desc&sort=activity&site=stackoverflow&filter=!Fcb(61J.xH8zQMnNMwf2k.*R8T",
            urlEncoded);
    printf("\nreq: %s\n", request);
    char *response = sendReq(request);
    printf("\noutput: %s\n", response);
}

int main(int argc, char *argv[]) {
    argv[0] = "git";
    int pipefd[2];
    pipe(pipefd);

    if (fork() == 0) {
        close(pipefd[0]); // Close read end of child pipe

        dup2(pipefd[1], 2); // Send stderr to pipe

        execvp(argv[0], argv); // Execute git commands
    } else {
        char buffer[1024];

        close(pipefd[1]); // Close read end of parent pipe

        int nbytes = read(pipefd[0], buffer, sizeof(buffer)); // Get size of pipe buffer
        strtok(buffer, "\n");
        if (nbytes > 0) {
            printf("Err:\n%s\n", buffer);
            getQuestion(buffer);
            getAnswer("20413459");
        }
    }
    return 0;
}

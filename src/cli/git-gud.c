#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <zlib.h>
#include <curl/curl.h>

char rfc3986[256] = {0};
char html5[256] = {0};

long unsigned int MAX_SIZE = 200000;

struct string {
  char *ptr;
  size_t len;
};

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

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

char *inflateBytes(char *input) {
    unsigned char *c = malloc(MAX_SIZE);

    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    // setup "b" as the input and "c" as the compressed output
    infstream.avail_in = (uInt)strlen(input); // size of input
    infstream.next_in = (Bytef *)input; // input char array

    infstream.avail_out = (uInt)MAX_SIZE; // size of output
    infstream.next_out = c; // output char array

    // the actual decompression work.
    if (inflateInit2(&infstream, -MAX_WBITS) != Z_OK) {
        fprintf(stderr, "Error with inflateInit2()");
    }
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);
    return (char *)c;
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);

  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
  }

  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}

char *sendCurlReq(char *route) {
    CURL *curl = curl_easy_init();
    struct curl_slist *slist = NULL;
    if (curl) {
        CURLcode res;
        slist = curl_slist_append(slist, "Accept: application/json");
        slist = curl_slist_append(slist, "Accept-Encoding: deflate");
        struct string s;
        init_string(&s);

        curl_easy_setopt(curl, CURLOPT_URL, route);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

        res = curl_easy_perform(curl);
        curl_slist_free_all(slist);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        return inflateBytes(s.ptr);
    } else {
        return "";
    }
}

void getQuestion(char question[1024]) {
    char request[1536];
    char urlEncoded[1200];
    url_encoder_rfc_tables_init();
    url_encode(rfc3986, (unsigned char*) question, urlEncoded);
    sprintf(request,
            "http://api.stackexchange.com/2.2/search/excerpts?key=U4DMV*8nvpm3EOpvf69Rxw((&pagesize=1&order=desc&sort=relevance&q=%s&site=stackoverflow&filter=!1zSijXI74x1547R0kRXdT",
            urlEncoded);
    char *response = sendCurlReq(request);
    printf("\noutput: %s\n", response);
}

void getAnswer(char question[1024]) {
    char request[1536];
    char urlEncoded[1200];
    url_encoder_rfc_tables_init();
    url_encode(rfc3986, (unsigned char*) question, urlEncoded);
    sprintf(request,
            "http://api.stackexchange.com/2.2/questions/%s/answers?key=U4DMV*8nvpm3EOpvf69Rxw((&site=stackoverflow&page=1&pagesize=1&order=desc&sort=votes&filter=!Fcb(61J.xH8zQMnNMwf2k.*R8T",
            urlEncoded);
    char *response = sendCurlReq(request);
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

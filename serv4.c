/*
 * George Maroulis 03/2017 TCP Server 4 UPatras CEID
 * Programming Language = C
 */

#include "keyvalue.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

ssize_t writen(int fd, const void *vptr, size_t n);

int ns = 0;
char keys[1000][50];
char values[1000][50];
char *found;
int socketfd;
int cllen;
int port;
int *clientfd;
struct sockaddr_in client;
struct sockaddr_in saddr;
char *found;
pthread_mutex_t mutex;

void *thre(void *clientfd) {
  char scom[1024];
  char com[1024];
  int sp = 0;
  int i = 0;
  int we;
  char key[50];
  char value[50];
  int offset = 0;
  int nxt = 0;
  int wre;
  found = NULL;
  int len = 0;
  while (1) {
    int nc;
    int ntcount;
    wre = read(clientfd, com + len, 1024 - len);
    if (wre < 0) {
      goto end;
    } else if (wre == 0) {
      goto end;
    }
    ntcount = 0;
    for (i = 0; i < len + wre; i++) {
      if (com[i] == '\0') {
        ntcount++;
      }
    }
  start:;
    nc = 0;
    printf("ntcount:%d\n", ntcount);
    if ((com[0] == 'p' || com[0] == 'g') && com[1] != '\0') {
      if (com[0] == 'p' && ntcount >= 2) {
        int j = 0;
        int k = 0;
        for (i = 1; i < wre + len; i++) {
          if (com[i] != '\0') {
            if (j == 0) {
              key[k] = com[i];
              k++;
              if (k >= 50) {
                goto end1;
              }
            } else if (j == 1) {
              value[k] = com[i];
              k++;
              if (k >= 50) {
                goto end2;
              }
            }
          } else {
            if (j == 0) {
            end1:;
              nc = k + 2;
              key[k] = '\0';
              j++;
              k = 0;
            } else if (j == 1) {
            end2:;
              value[k] = '\0';
              nc += k + 1;
              break;
            }
          }
        }
        len = wre - nc + len;
        ntcount = ntcount - 2;
        put(key, value);
        printf("key:%s\n", key);
        printf("value:%s\n", value);
        for (i = 0; i < len; i++) {
          com[i] = com[i + nc];
        }
      } else if (com[0] == 'g' && ntcount >= 1) {
        int k = 0;
        int x;
        for (i = 1; i < wre + len; i++) {
          if (com[i] != '\0') {
            key[k] = com[i];
            k++;
            if (k >= 50) {
              goto end3;
            }
          } else {
          end3:;
            nc = k + 2;
            key[k] = '\0';
            break;
          }
        }
        found = get(key);
        scom[0] = '\0';
        sp = 0;
        printf("key:%s\n", key);
        if (found == NULL) {
          scom[sp] = 'n';
          sp++;
        } else {
          scom[sp] = 'f';
          sp++;
          for (x = 0; x < strlen(found); x++) {
            scom[sp] = found[x];
            sp++;
          }
          scom[sp] = '\0';
          sp++;
        }
        we = writen(clientfd, &scom, (size_t)sp);
        len = wre - nc + len;
        ntcount = ntcount - 1;
        for (i = 0; i < len; i++) {
          com[i] = com[i + nc];
        }
      }
      // printf("to nxt einai :%d\n", nxt);
      // printf("to wre einai :%d\n\n", wre);
      if ((com[nxt] == 'p' && ntcount >= 2) ||
          (com[nxt] == 'g' && ntcount >= 1)) {
        wre = 0;
        goto start;
      }
    } else {
      goto end;
    }
  }
end:;
  close(clientfd);
}

int main(int argc, char **argv) {
  int wre = 0;
  int count = 0;
  if (argc != 2)
    return -1;
  port = atoi(argv[1]);
  // opening socket
  socketfd = socket(AF_INET, SOCK_STREAM, 0);
  if (socketfd < 0) {
    printf("socket\n");
    return -1;
  }
  // enable SO_REUSEADDR
  if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) <
      0) {
    printf("so_reuse\n");
    return -1;
  }
  // set sockets details
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  saddr.sin_port = htons(port);
  // bind the socket
  if (bind(socketfd, (struct sockaddr *)&saddr, sizeof(saddr))) {
    printf("bind\n");
    return -1;
  }
  // listen to the port
  if (listen(socketfd, 5) < 0) {
    printf("listen\n");
    return -1;
  }
  clientfd = (int *)malloc(sizeof(int));
  cllen = sizeof(client);
  pthread_t *thr;
  thr = (pthread_t *)malloc(sizeof(pthread_t));
  while (1) {
    found = NULL;
    // accept requests
    clientfd[count] = accept(socketfd, (struct sockaddr *)&client, &cllen);
    if (clientfd < 0) {
      printf("accept\n");
      return -1;
    }
    pthread_create(&thr[count], NULL, thre, (void *)clientfd[count]);
    count++;
    thr = (pthread_t *)realloc(thr, sizeof(pthread_t) * (count + 1));
    clientfd = (int *)realloc(clientfd, sizeof(int) * (count + 1));
  }
  return 0;
}

char *get(char *key) {
  pthread_mutex_lock(&mutex);
  int i;
  for (i = 0; i < ns; i++) {
    if (strcmp(keys[i], key) == 0) {
      pthread_mutex_unlock(&mutex);
      return values[i];
    }
  }
  pthread_mutex_unlock(&mutex);
  return NULL;
}

void put(char *key, char *value) {
  pthread_mutex_lock(&mutex);
  int i = 0;
  for (i = 0; i < ns + 1; i++) {
    if (strcmp(keys[i], key) == 0) {
      if (strlen(key) < 50) {
        strcpy(values[i], value);
        goto end;
      } else {
        goto end;
      }
    }
  }
  if (strlen(key) < 50 && strlen(value) < 50) {
    strcpy(keys[ns], key);
    strcpy(values[ns], value);
  }
  ns++;
end:;
  pthread_mutex_unlock(&mutex);
}

ssize_t writen(int fd, const void *vptr, size_t n) {
  size_t nleft;
  ssize_t nwritten;
  const char *ptr;
  ptr = vptr;
  nleft = n;
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
      if (errno == EINTR)
        nwritten = 0; /* and call write() again */
      else
        return -1; /* error */
    }
    nleft -= nwritten;
    ptr += nwritten;
  }
  return n;
}
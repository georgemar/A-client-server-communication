/*
 * George Maroulis 03/2017 TCP Server 3 UPatras CEID
 * Programming Language = C
 */

#include "keyvalue.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/sockios.h>
#include <netdb.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

ssize_t writen(int fd, const void *vptr, size_t n);

int ns = 0;
char *kaddr;
char *vaddr;
int *naddr;
int *taddr;
sem_t *sem;
char *found;
char scom[1024];
char values[50];
int sp;
sem_t *sem1;

void sig_chld(int x) {
  while (waitpid(-1, NULL, 0) > 0) {
  }
  signal(SIGCHLD, sig_chld);
}

int main(int argc, char **argv) {
  int shmid = shmget(IPC_PRIVATE, sizeof(sem_t), 0600);
  sem = (sem_t *)shmat(shmid, 0, 0);
  sem_init(sem, 1, 1);
  int shmid2 = shmget(IPC_PRIVATE, sizeof(sem_t), 0600);
  sem1 = (sem_t *)shmat(shmid2, 0, 0);
  sem_init(sem1, 1, 1);
  int kopen, vopen, nopen;
  nopen = shm_open("/n", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  kopen = shm_open("/k", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  vopen = shm_open("/v", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (vopen == -1 || kopen == -1 || nopen == -1) {
    printf("Open failed\n");
    return -1;
  }
  ftruncate(nopen, sizeof(int));
  /*taddr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, nopen,
  0); if (taddr == -1) { printf("mmap failed\n"); return -1;
  }
  memcpy(taddr, &ns, sizeof(int));*/
  int i = 0;
  struct linger a;
  a.l_onoff = 1;
  a.l_linger = 100;
  int we;
  int pid;
  char key[50];
  char value[50];
  char com[10240];
  int socketfd;
  int cllen;
  int nr_procs;
  int clientfd;
  int port;
  int count = 1;
  struct sockaddr_in client;
  struct sockaddr_in saddr;
  if (argc != 3)
    return -1;
  port = atoi(argv[1]);
  nr_procs = atoi(argv[2]);
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
  int keepalive = 1;
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
  for (i = 0; i < nr_procs; i++) {
    pid = fork();
    if (pid == 0) {
      cllen = sizeof(client);
      signal(SIGCHLD, sig_chld);
      while (1) {
        found = NULL;
        // accept requests
        sem_wait(sem1);
        clientfd = accept(socketfd, (struct sockaddr *)&client, &cllen);
        sem_post(sem1);
        if (clientfd < 0) {
          printf("accept\n");
          return -1;
        }
        int offset = 0;
        int nxt = 0;
        int wre;
        int len = 0;
        ftruncate(kopen, sizeof(char) * 1000 * 50);
        ftruncate(vopen, sizeof(char) * 1000 * 50);
        ftruncate(nopen, sizeof(int));
        naddr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED,
                     nopen, 0);
        kaddr = mmap(NULL, sizeof(char) * 1000 * 50, PROT_READ | PROT_WRITE,
                     MAP_SHARED, kopen, 0);
        vaddr = mmap(NULL, sizeof(char) * 1000 * 50, PROT_READ | PROT_WRITE,
                     MAP_SHARED, vopen, 0);
        while (1) {
          int nc;
          int ntcount;
          wre = read(clientfd, com + len, 1024 - len);
          if (wre < 0) {
            goto end;
          } else if (wre == 0) {
            goto end;
          }
          printf("Elava %d bytes\n", wre);
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
              len = len + wre - nc;
              printf("wre:%d,nc:%d\n", wre, nc);
              printf("lenp:%d\n", len);
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
              printf("found:%s\n", found);
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
              len = len + wre - nc;
              printf("leng:%d\n", len);
              printf("wre:%d,nc:%d\n", wre, nc);
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
    }
  }
  wait(NULL);
}

char *get(char *key) {
  int i;
  char keys[50];
  sem_wait(sem);
  memcpy(&ns, naddr, sizeof(int));
  for (i = 0; i < ns; i++) {
    memcpy(keys, kaddr + (i * 50), sizeof(char) * 50);
    memcpy(values, vaddr + (i * 50), sizeof(char) * 50);
    if (strcmp(keys, key) == 0) {
      sem_post(sem);
      return values;
    }
  }
  sem_post(sem);
  return NULL;
}

void put(char *key, char *value) {
  int i;
  char tkeys[50];
  sem_wait(sem);
  memcpy(&ns, naddr, sizeof(int));
  for (i = 0; i < ns; i++) {
    memcpy(tkeys, kaddr + (i * 50), sizeof(char) * 50);
    if (strcmp(tkeys, key) == 0) {
      memcpy(vaddr + i * 50, value, sizeof(char) * ((strlen(value) + 1)));
      goto end;
    }
  }
  memcpy(kaddr + ns * 50, key, sizeof(char) * (strlen(key) + 1));
  memcpy(vaddr + ns * 50, value, sizeof(char) * (strlen(value) + 1));
  ns++;
  memcpy(naddr, &ns, sizeof(int));
end:;
  sem_post(sem);
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
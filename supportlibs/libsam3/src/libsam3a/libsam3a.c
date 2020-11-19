/* async SAMv3 library
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 * I2P-Bote:
 * 5m77dFKGEq6~7jgtrfw56q3t~SmfwZubmGdyOLQOPoPp8MYwsZ~pfUCwud6LB1EmFxkm4C3CGlzq-hVs9WnhUV
 * we are the Borg. */
#include "libsam3a.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef __MINGW32__
//#include <winsock.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif
#endif

#ifdef __unix__
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#endif
////////////////////////////////////////////////////////////////////////////////
int libsam3a_debug = 0;

#define DEFAULT_TCP_PORT (7656)
#define DEFAULT_UDP_PORT (7655)

////////////////////////////////////////////////////////////////////////////////
uint64_t sam3atimeval2ms(const struct timeval *tv) {
  return ((uint64_t)tv->tv_sec) * 1000 + ((uint64_t)tv->tv_usec) / 1000;
}

void sam3ams2timeval(struct timeval *tv, uint64_t ms) {
  tv->tv_sec = ms / 1000;
  tv->tv_usec = (ms % 1000) * 1000;
}

////////////////////////////////////////////////////////////////////////////////
static inline int isValidKeyChar(char ch) {
  return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
         (ch >= '0' && ch <= '9') || ch == '-' || ch == '~';
}

int sam3aIsValidPubKey(const char *key) {
  if (key != NULL && strlen(key) == SAM3A_PUBKEY_SIZE) {
    for (int f = 0; f < SAM3A_PUBKEY_SIZE; ++f)
      if (!isValidKeyChar(key[f]))
        return 0;
    return 1;
  }
  return 0;
}

int sam3aIsValidPrivKey(const char *key) {
  if (key != NULL && strlen(key) == SAM3A_PRIVKEY_SIZE) {
    for (int f = 0; f < SAM3A_PRIVKEY_SIZE; ++f)
      if (!isValidKeyChar(key[f]))
        return 0;
    return 1;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
/*
static int sam3aSocketSetTimeoutSend (int fd, int timeoutms) {
  if (fd >= 0 && timeoutms >= 0) {
    struct timeval tv;
    //
    ms2timeval(&tv, timeoutms);
    return (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0 ? -1 :
0);
  }
  return -1;
}


static int sam3aSocketSetTimeoutReceive (int fd, int timeoutms) {
  if (fd >= 0 && timeoutms >= 0) {
    struct timeval tv;
    //
    ms2timeval(&tv, timeoutms);
    return (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0 ? -1 :
0);
  }
  return -1;
}
*/

static int sam3aBytesAvail(int fd) {
  int av = 0;
  //
  if (ioctl(fd, FIONREAD, &av) < 0)
    return -1;
  return av;
}

static uint32_t sam3aResolveHost(const char *hostname) {
  struct hostent *host;
  //
  if (hostname == NULL || !hostname[0])
    return 0;
  if ((host = gethostbyname(hostname)) == NULL || host->h_name == NULL ||
      !host->h_addr_list[0][0]) {
    if (libsam3a_debug)
      fprintf(stderr, "ERROR: can't resolve '%s'\n", hostname);
    return 0;
  }
  return ((struct in_addr *)host->h_addr_list[0])->s_addr;
}

static int sam3aConnect(uint32_t ip, int port, int *complete) {
  int fd, val = 1;
  //
  if (complete != NULL)
    *complete = 0;
  if (ip == 0 || ip == 0xffffffffUL || port < 1 || port > 65535)
    return -1;
  //
  // yes, this is Linux-specific; you know what? i don't care.
  if ((fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)) < 0)
    return -1;
  //
  setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
  //
  for (;;) {
    struct sockaddr_in addr;
    //
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ip;
    //
    if (connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
      if (errno == EINPROGRESS)
        break; // the process is started
      if (errno != EINTR) {
        close(fd);
        return -1;
      }
    } else {
      // connection complete
      if (complete != NULL)
        *complete = 1;
      break;
    }
  }
  //
  return fd;
}

// <0: error; 0: ok
static int sam3aDisconnect(int fd) {
  if (fd >= 0) {
    shutdown(fd, SHUT_RDWR);
    return close(fd);
  }
  //
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
// <0: error; >=0: bytes sent
static int sam3aSendBytes(int fd, const void *buf, int bufSize) {
  const char *c = (const char *)buf;
  int total = 0;
  //
  if (fd < 0 || (buf == NULL && bufSize > 0))
    return -1;
  //
  while (bufSize > 0) {
    int wr = send(fd, c, bufSize, MSG_NOSIGNAL);
    //
    if (wr < 0) {
      if (errno == EINTR)
        continue; // interrupted by signal
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // bufSize is too big
        if (bufSize == 1)
          break; // can't send anything
        // try to send a half of a buffer
        if ((wr = sam3aSendBytes(fd, c, bufSize / 2)) < 0)
          return wr; // error
      } else {
        return -1; // alas
      }
    }
    //
    if (wr == 0)
      break; // can't send anything
    c += wr;
    bufSize -= wr;
    total += wr;
  }
  //
  return total;
}

/* <0: error; >=0: bytes received */
/* note that you should call this function when there is some bytes to read, so
 * 0 means 'connection closed' */
/*
static int sam3aReceive (int fd, void *buf, int bufSize) {
  char *c = (char *)buf;
  int total = 0;
  //
  if (fd < 0 || (buf == NULL && bufSize > 0)) return -1;
  //
  while (bufSize > 0) {
    int av = sam3aBytesAvail(fd), rd;
    //
    if (av == 0) break; // no more
    if (av > bufSize) av = bufSize;
    rd = recv(fd, c, av, 0);
    if (rd < 0) {
      if (errno == EINTR) continue; // interrupted by signal
      if (errno == EAGAIN || errno == EWOULDBLOCK) break; // the thing that
should not be return -1; // error
    }
    if (rd == 0) break;
    c += rd;
    bufSize -= rd;
    total += rd;
  }
  //
  return total;
}
*/

////////////////////////////////////////////////////////////////////////////////
char *sam3PrintfVA(int *plen, const char *fmt, va_list app) {
  char buf[1024], *p = buf;
  int size = sizeof(buf) - 1, len = 0;
  //
  if (plen != NULL)
    *plen = 0;
  for (;;) {
    va_list ap;
    char *np;
    int n;
    //
    va_copy(ap, app);
    n = vsnprintf(p, size, fmt, ap);
    va_end(ap);
    //
    if (n > -1 && n < size) {
      len = n;
      break;
    }
    if (n > -1)
      size = n + 1;
    else
      size *= 2;
    if (p == buf) {
      if ((p = malloc(size)) == NULL)
        return NULL;
    } else {
      if ((np = realloc(p, size)) == NULL) {
        free(p);
        return NULL;
      }
      p = np;
    }
  }
  //
  if (p == buf) {
    if ((p = malloc(len + 1)) == NULL)
      return NULL;
    memcpy(p, buf, len + 1);
  }
  if (plen != NULL)
    *plen = len;
  return p;
}

__attribute__((format(printf, 2, 3))) char *sam3Printf(int *plen,
                                                       const char *fmt, ...) {
  va_list ap;
  char *res;
  //
  va_start(ap, fmt);
  res = sam3PrintfVA(plen, fmt, ap);
  va_end(ap);
  //
  return res;
}

/*
 * check if we have EOL in received socket data
 * this function should be called when sam3aBytesAvail() result > 0
 * return: <0: error; 0: no EOL in bytes2check, else: # of bytes to EOL
 * (including EOL itself)
 */
/*
static int sam3aCheckEOL (int fd, int bytes2check) {
  char *d = dest;
  //
  if (bytes2check < 0 || fd < 0) return -1;
  memset(dest, 0, maxSize);
  while (maxSize > 1) {
    char *e;
    int rd = recv(fd, d, maxSize-1, MSG_PEEK);
    //
    if (rd < 0 && errno == EINTR) continue; // interrupted by signal
    if (rd == 0) {
      rd = recv(fd, d, 1, 0);
      if (rd < 0 && errno == EINTR) continue; // interrupted by signal
      if (d[0] == '\n') {
        d[0] = 0; // remove '\n'
        return 0;
      }
    } else {
      if (rd < 0) return -1; // error or connection closed; alas
    }
    // check for EOL
    d[maxSize-1] = 0;
    if ((e = strchr(d, '\n')) != NULL) {
      rd = e-d+1; // bytes to receive
      if (sam3atcpReceive(fd, d, rd) < 0) return -1; // alas
      d[rd-1] = 0; // remove '\n'
      return 0; // done
    } else {
      // let's receive this part and go on
      if (sam3atcpReceive(fd, d, rd) < 0) return -1; // alas
      maxSize -= rd;
      d += rd;
    }
  }
  // alas, the string is too big
  return -1;
}
*/

////////////////////////////////////////////////////////////////////////////////
/*
int sam3audpSendToIP (uint32_t ip, int port, const void *buf, int bufSize) {
  struct sockaddr_in addr;
  int fd, res;
  //
  if (buf == NULL || bufSize < 1) return -1;
  if (port < 1 || port > 65535) port = 7655;
  //
  if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    if (libsam3a_debug) fprintf(stderr, "ERROR: can't create socket\n");
    return -1;
  }
  //
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = ip;
  //
  res = sendto(fd, buf, bufSize, 0, (struct sockaddr *)&addr, sizeof(addr));
  //
  if (res < 0) {
    if (libsam3a_debug) {
      res = errno;
      fprintf(stderr, "UDP ERROR (%d): %s\n", res, strerror(res));
    }
    res = -1;
  } else {
    if (libsam3a_debug) fprintf(stderr, "UDP: %d bytes sent\n", res);
  }
  //
  close(fd);
  //
  return (res >= 0 ? 0 : -1);
}


int sam3audpSendTo (const char *hostname, int port, const void *buf, int
bufSize, uint32_t *ip) { struct hostent *host = NULL;
  //
  if (buf == NULL || bufSize < 1) return -1;
  if (hostname == NULL || !hostname[0]) hostname = "localhost";
  if (port < 1 || port > 65535) port = 7655;
  //
  host = gethostbyname(hostname);
  if (host == NULL || host->h_name == NULL || !host->h_name[0]) {
    if (libsam3a_debug) fprintf(stderr, "ERROR: can't resolve '%s'\n",
hostname); return -1;
  }
  //
  if (ip != NULL) *ip = ((struct in_addr *)host->h_addr)->s_addr;
  return sam3audpSendToIP(((struct in_addr *)host->h_addr)->s_addr, port, buf,
bufSize);
}
*/

////////////////////////////////////////////////////////////////////////////////
typedef struct SAMFieldList {
  char *name;
  char *value;
  struct SAMFieldList *next;
} SAMFieldList;

static void sam3aFreeFieldList(SAMFieldList *list) {
  while (list != NULL) {
    SAMFieldList *c = list;
    //
    list = list->next;
    if (c->name != NULL)
      free(c->name);
    if (c->value != NULL)
      free(c->value);
    free(c);
  }
}

static void sam3aDumpFieldList(const SAMFieldList *list) {
  for (; list != NULL; list = list->next) {
    fprintf(stderr, "%s=[%s]\n", list->name, list->value);
  }
}

static const char *sam3aFindField(const SAMFieldList *list, const char *field) {
  if (list != NULL && field != NULL) {
    for (list = list->next; list != NULL; list = list->next) {
      if (list->name != NULL && strcmp(field, list->name) == 0)
        return list->value;
    }
  }
  return NULL;
}

static char *xstrdup(const char *s, int len) {
  if (len >= 0) {
    char *res = malloc(len + 1);
    //
    if (res != NULL) {
      if (len > 0)
        memcpy(res, s, len);
      res[len] = 0;
    }
    //
    return res;
  }
  //
  return NULL;
}

// returns NULL if there are no more tokens
static inline const char *xstrtokend(const char *s) {
  while (*s && isspace(*s))
    ++s;
  //
  if (*s) {
    char qch = 0;
    //
    while (*s) {
      if (*s == qch) {
        qch = 0;
      } else if (*s == '"') {
        qch = *s;
      } else if (qch) {
        if (*s == '\\' && s[1])
          ++s;
      } else if (isspace(*s)) {
        break;
      }
      ++s;
    }
  } else {
    s = NULL;
  }
  //
  return s;
}

SAMFieldList *sam3aParseReply(const char *rep) {
  SAMFieldList *first = NULL, *last, *c;
  const char *p = rep, *e, *e1;
  //
  // first 2 words
  while (*p && isspace(*p))
    ++p;
  if ((e = xstrtokend(p)) == NULL)
    return NULL;
  if ((e1 = xstrtokend(e)) == NULL)
    return NULL;
  //
  if ((first = last = c = malloc(sizeof(SAMFieldList))) == NULL)
    return NULL;
  c->next = NULL;
  c->name = c->value = NULL;
  if ((c->name = xstrdup(p, e - p)) == NULL)
    goto error;
  while (*e && isspace(*e))
    ++e;
  if ((c->value = xstrdup(e, e1 - e)) == NULL)
    goto error;
  //
  p = e1;
  while (*p) {
    while (*p && isspace(*p))
      ++p;
    if ((e = xstrtokend(p)) == NULL)
      break; // no more tokens
    //
    if (libsam3a_debug)
      fprintf(stderr, "<%s>\n", p);
    //
    if ((c = malloc(sizeof(SAMFieldList))) == NULL)
      return NULL;
    c->next = NULL;
    c->name = c->value = NULL;
    last->next = c;
    last = c;
    //
    if ((e1 = memchr(p, '=', e - p)) != NULL) {
      // key=value
      if ((c->name = xstrdup(p, e1 - p)) == NULL)
        goto error;
      if ((c->value = xstrdup(e1 + 1, e - e1 - 1)) == NULL)
        goto error;
    } else {
      // only key (there is no such replies in SAMv3, but...
      if ((c->name = xstrdup(p, e - p)) == NULL)
        goto error;
      if ((c->value = strdup("")) == NULL)
        goto error;
    }
    p = e;
  }
  //
  if (libsam3a_debug)
    sam3aDumpFieldList(first);
  //
  return first;
error:
  sam3aFreeFieldList(first);
  return NULL;
}

// example:
//   r0: 'HELLO'
//   r1: 'REPLY'
//   field: NULL or 'RESULT'
//   VALUE: NULL or 'OK'
// returns bool
int sam3aIsGoodReply(const SAMFieldList *list, const char *r0, const char *r1,
                     const char *field, const char *value) {
  if (list != NULL && list->name != NULL && list->value != NULL) {
    if (r0 != NULL && strcmp(r0, list->name) != 0)
      return 0;
    if (r1 != NULL && strcmp(r1, list->value) != 0)
      return 0;
    if (field != NULL) {
      for (list = list->next; list != NULL; list = list->next) {
        if (list->name == NULL || list->value == NULL)
          return 0; // invalid list, heh
        if (strcmp(field, list->name) == 0) {
          if (value != NULL && strcmp(value, list->value) != 0)
            return 0;
          return 1;
        }
      }
    }
    return 1;
  }
  return 0;
}

// NULL: error; else: list of fields
// first item is always 2-word reply, with first word in name and second in
// value
/*
SAMFieldList *sam3aReadReply (int fd) {
  char rep[2048]; // should be enough for any reply
  //
  if (sam3atcpReceiveStr(fd, rep, sizeof(rep)) < 0) return NULL;
  if (libsam3a_debug) fprintf(stderr, "SAM REPLY: [%s]\n", rep);
  return sam3aParseReply(rep);
}
*/

////////////////////////////////////////////////////////////////////////////////
// by Bob Jenkins
// public domain
// http://burtleburtle.net/bob/rand/smallprng.html
//
////////////////////////////////////////////////////////////////////////////////
typedef struct {
  uint32_t a, b, c, d;
} BJRandCtx;

#define BJPRNG_ROT(x, k) (((x) << (k)) | ((x) >> (32 - (k))))

static uint32_t bjprngRand(BJRandCtx *x) {
  uint32_t e;
  /* original:
     e = x->a-BJPRNG_ROT(x->b, 27);
  x->a = x->b^BJPRNG_ROT(x->c, 17);
  x->b = x->c+x->d;
  x->c = x->d+e;
  x->d = e+x->a;
  */
  /* better, but slower at least in idiotic m$vc */
  e = x->a - BJPRNG_ROT(x->b, 23);
  x->a = x->b ^ BJPRNG_ROT(x->c, 16);
  x->b = x->c + BJPRNG_ROT(x->d, 11);
  x->c = x->d + e;
  x->d = e + x->a;
  //
  return x->d;
}

static void bjprngInit(BJRandCtx *x, uint32_t seed) {
  x->a = 0xf1ea5eed;
  x->b = x->c = x->d = seed;
  for (int i = 0; i < 20; ++i)
    bjprngRand(x);
}

static inline uint32_t hashint(uint32_t a) {
  a -= (a << 6);
  a ^= (a >> 17);
  a -= (a << 9);
  a ^= (a << 4);
  a -= (a << 3);
  a ^= (a << 10);
  a ^= (a >> 15);
  return a;
}

static uint32_t genSeed(void) {
  volatile uint32_t seed = 1;
  uint32_t res;
#ifndef WIN32
  struct sysinfo sy;
  pid_t pid = getpid();
  //
  sysinfo(&sy);
  res = hashint((uint32_t)pid) ^ hashint((uint32_t)time(NULL)) ^
        hashint((uint32_t)sy.sharedram) ^ hashint((uint32_t)sy.bufferram) ^
        hashint((uint32_t)sy.uptime);
#else
  res = hashint((uint32_t)GetCurrentProcessId()) ^
        hashint((uint32_t)GetTickCount());
#endif
  res += __sync_fetch_and_add(&seed, 1);
  //
  return hashint(res);
}

////////////////////////////////////////////////////////////////////////////////
int sam3aGenChannelName(char *dest, int minlen, int maxlen) {
  BJRandCtx rc;
  int len;
  //
  if (dest == NULL || minlen < 1 || maxlen < minlen || minlen > 65536 ||
      maxlen > 65536)
    return -1;
  bjprngInit(&rc, genSeed());
  len = minlen + (bjprngRand(&rc) % (maxlen - minlen + 1));
  while (len-- > 0) {
    int ch = bjprngRand(&rc) % 64;
    //
    if (ch >= 0 && ch < 10)
      ch += '0';
    else if (ch >= 10 && ch < 36)
      ch += 'A' - 10;
    else if (ch >= 36 && ch < 62)
      ch += 'a' - 36;
    else if (ch == 62)
      ch = '-';
    else if (ch == 63)
      ch = '_';
    else if (ch > 64)
      abort();
    *dest++ = ch;
  }
  *dest++ = 0;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
int sam3aIsActiveSession(const Sam3ASession *ses) {
  return (ses != NULL && ses->fd >= 0 && !ses->cancelled);
}

int sam3aIsActiveConnection(const Sam3AConnection *conn) {
  return (conn != NULL && conn->fd >= 0 && !conn->cancelled);
}

////////////////////////////////////////////////////////////////////////////////
static inline void strcpyerrs(Sam3ASession *ses, const char *errstr) {
  // memset(ses->error, 0, sizeof(ses->error));
  ses->error[sizeof(ses->error) - 1] = 0;
  if (errstr != NULL)
    strncpy(ses->error, errstr, sizeof(ses->error) - 1);
}

static inline void strcpyerrc(Sam3AConnection *conn, const char *errstr) {
  // memset(conn->error, 0, sizeof(conn->error));
  conn->error[sizeof(conn->error) - 1] = 0;
  if (errstr != NULL)
    strncpy(conn->error, errstr, sizeof(conn->error) - 1);
}

static void connDisconnect(Sam3AConnection *conn) {
  conn->cbAIOProcessorR = conn->cbAIOProcessorW = NULL;
  if (conn->aio.data != NULL) {
    free(conn->aio.data);
    conn->aio.data = NULL;
  }
  if (!conn->cancelled && conn->fd >= 0) {
    conn->cancelled = 1;
    shutdown(conn->fd, SHUT_RDWR);
    if (conn->callDisconnectCB && conn->cb.cbDisconnected != NULL)
      conn->cb.cbDisconnected(conn);
  }
}

static void sesDisconnect(Sam3ASession *ses) {
  ses->cbAIOProcessorR = ses->cbAIOProcessorW = NULL;
  if (ses->aio.data != NULL) {
    free(ses->aio.data);
    ses->aio.data = NULL;
  }
  if (!ses->cancelled && ses->fd >= 0) {
    ses->cancelled = 1;
    shutdown(ses->fd, SHUT_RDWR);
    for (Sam3AConnection *c = ses->connlist; c != NULL; c = c->next)
      connDisconnect(c);
    if (ses->callDisconnectCB && ses->cb.cbDisconnected != NULL)
      ses->cb.cbDisconnected(ses);
  }
}

static void sesError(Sam3ASession *ses, const char *errstr) {
  if (errstr == NULL || !errstr[0])
    errstr = "I2P_ERROR";
  strcpyerrs(ses, errstr);
  if (ses->cb.cbError != NULL)
    ses->cb.cbError(ses);
  sesDisconnect(ses);
}

static void connError(Sam3AConnection *conn, const char *errstr) {
  if (errstr == NULL || !errstr[0])
    errstr = "I2P_ERROR";
  strcpyerrc(conn, errstr);
  if (conn->cb.cbError != NULL)
    conn->cb.cbError(conn);
  connDisconnect(conn);
}

////////////////////////////////////////////////////////////////////////////////
static int aioSender(int fd, Sam3AIO *aio) {
  int wr = sam3aSendBytes(fd, aio->data + aio->dataPos,
                          aio->dataUsed - aio->dataPos);
  //
  if (wr < 0)
    return -1;
  aio->dataPos += wr;
  return 0;
}

// dataUsed: max line size (with '\n')
// dataSize: must be at least (dataUsed+1)
static int aioLineReader(int fd, Sam3AIO *aio) {
  //
  for (;;) {
    int av = sam3aBytesAvail(fd), rd;
    //
    if (av < 0)
      return -1;
    if (av == 0)
      return 0; // do nothing
    if (aio->dataPos >= aio->dataUsed - 1)
      return -1; // line too long
    if ((rd = (aio->dataUsed - 1) - aio->dataPos) > av)
      rd = av;
    if ((rd = recv(fd, aio->data + aio->dataPos, rd, MSG_PEEK)) < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    if (rd == 0)
      return 0; // do nothing
    // now look for '\n'
    for (int f = aio->dataPos; f < aio->dataPos + rd; ++f) {
      if (aio->data[f] == '\n') {
        // got it!
        if (recv(fd, aio->data + aio->dataPos, f - aio->dataPos + 1, 0) !=
            f + 1)
          return -1;                      // the thing that should not be
        aio->data[f] = 0;                 // convert to asciiz
        aio->dataUsed = aio->dataPos = f; // length
        return 1;                         // '\n' found!
      }
      if (!aio->data[f])
        return -1; // there should not be zero bytes
    }
    // no '\n' found
    if (recv(fd, aio->data + aio->dataPos, rd, 0) != rd)
      return -1; // the thing that should not be
    aio->dataPos += rd;
  }
}

////////////////////////////////////////////////////////////////////////////////
static void aioSesCmdReplyReader(Sam3ASession *ses) {
  int res = aioLineReader(ses->fd, &ses->aio);
  //
  if (res < 0) {
    sesError(ses, "IO_ERROR");
    return;
  }
  if (res > 0) {
    // we got full line
    if (libsam3a_debug)
      fprintf(stderr, "CMDREPLY: %s\n", ses->aio.data);
    if (ses->aio.cbReplyCheckSes != NULL)
      ses->aio.cbReplyCheckSes(ses);
  }
}

static void aioSesCmdSender(Sam3ASession *ses) {
  if (ses->aio.dataPos < ses->aio.dataUsed) {
    if (aioSender(ses->fd, &ses->aio) < 0) {
      sesError(ses, "IO_ERROR");
      return;
    }
  }
  //
  if (ses->aio.dataPos == ses->aio.dataUsed) {
    // hello sent, now wait for reply
    // 2048 bytes of reply line should be enough
    if (ses->aio.dataSize < 2049) {
      char *n = realloc(ses->aio.data, 2049);
      //
      if (n == NULL) {
        sesError(ses, "MEMORY_ERROR");
        return;
      }
      ses->aio.data = n;
      ses->aio.dataSize = 2049;
    }
    ses->aio.dataUsed = 2048;
    ses->aio.dataPos = 0;
    ses->cbAIOProcessorR = aioSesCmdReplyReader;
    ses->cbAIOProcessorW = NULL;
  }
}

static __attribute__((format(printf, 3, 4))) int
aioSesSendCmdWaitReply(Sam3ASession *ses, void (*cbCheck)(Sam3ASession *ses),
                       const char *fmt, ...) {
  va_list ap;
  char *str;
  int len;
  //
  va_start(ap, fmt);
  str = sam3PrintfVA(&len, fmt, ap);
  va_end(ap);
  //
  if (str == NULL)
    return -1;
  if (ses->aio.data != NULL)
    free(ses->aio.data);
  ses->aio.data = str;
  ses->aio.dataUsed = len;
  ses->aio.dataSize = len + 1;
  ses->aio.dataPos = 0;
  ses->aio.cbReplyCheckSes = cbCheck;
  ses->cbAIOProcessorR = NULL;
  ses->cbAIOProcessorW = aioSesCmdSender;
  //
  if (libsam3a_debug)
    fprintf(stderr, "CMD: %s", str);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
static void aioSesHelloChecker(Sam3ASession *ses) {
  SAMFieldList *rep = sam3aParseReply(ses->aio.data);
  //
  if (rep != NULL && sam3aIsGoodReply(rep, "HELLO", "REPLY", "RESULT", "OK") &&
      sam3aIsGoodReply(rep, NULL, NULL, "VERSION", "3.0")) {
    ses->cbAIOProcessorR = ses->cbAIOProcessorW = NULL;
    sam3aFreeFieldList(rep);
    if (ses->aio.udata != NULL) {
      void (*cbComplete)(Sam3ASession * ses) = ses->aio.udata;
      //
      cbComplete(ses);
    }
  } else {
    sam3aFreeFieldList(rep);
    sesError(ses, NULL);
  }
}

static int sam3aSesStartHandshake(Sam3ASession *ses,
                                  void (*cbComplete)(Sam3ASession *ses)) {
  if (cbComplete != NULL)
    ses->aio.udata = cbComplete;
  if (aioSesSendCmdWaitReply(ses, aioSesHelloChecker, "%s\n",
                             "HELLO VERSION MIN=3.0 MAX=3.0") < 0)
    return -1;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
static void aioSesNameMeChecker(Sam3ASession *ses) {
  SAMFieldList *rep = sam3aParseReply(ses->aio.data);
  const char *v = NULL;
  //
  if (rep == NULL) {
    sesError(ses, NULL);
    return;
  }
  if (!sam3aIsGoodReply(rep, "NAMING", "REPLY", "RESULT", "OK") ||
      (v = sam3aFindField(rep, "VALUE")) == NULL ||
      strlen(v) != SAM3A_PUBKEY_SIZE) {
    // if (libsam3a_debug) fprintf(stderr, "sam3aCreateSession: invalid NAMING
    // reply (%d)...\n", (v != NULL ? strlen(v) : -1));
    if ((v = sam3aFindField(rep, "RESULT")) != NULL && strcmp(v, "OK") == 0)
      v = NULL;
    sesError(ses, v);
    sam3aFreeFieldList(rep);
    return;
  }
  strcpy(ses->pubkey, v);
  sam3aFreeFieldList(rep);
  //
  ses->cbAIOProcessorR = ses->cbAIOProcessorW = NULL;
  ses->callDisconnectCB = 1;
  if (ses->cb.cbCreated != NULL)
    ses->cb.cbCreated(ses);
}

static void aioSesCreateChecker(Sam3ASession *ses) {
  SAMFieldList *rep = sam3aParseReply(ses->aio.data);
  const char *v;
  //
  if (rep == NULL) {
    sesError(ses, NULL);
    return;
  }
  if (!sam3aIsGoodReply(rep, "SESSION", "STATUS", "RESULT", "OK") ||
      (v = sam3aFindField(rep, "DESTINATION")) == NULL ||
      strlen(v) != SAM3A_PRIVKEY_SIZE) {
    sam3aFreeFieldList(rep);
    if ((v = sam3aFindField(rep, "RESULT")) != NULL && strcmp(v, "OK") == 0)
      v = NULL;
    sesError(ses, v);
    return;
  }
  // ok
  // fprintf(stderr, "\nPK: %s\n", v);
  strcpy(ses->privkey, v);
  sam3aFreeFieldList(rep);
  // get our public key
  if (aioSesSendCmdWaitReply(ses, aioSesNameMeChecker, "%s\n",
                             "NAMING LOOKUP NAME=ME") < 0) {
    sesError(ses, "MEMORY_ERROR");
  }
}

// handshake for SESSION CREATE complete
static void aioSesHandshacked(Sam3ASession *ses) {
  static const char *typenames[3] = {"RAW", "DATAGRAM", "STREAM"};
  //
  if (aioSesSendCmdWaitReply(
          ses, aioSesCreateChecker,
          "SESSION CREATE STYLE=%s ID=%s DESTINATION=%s%s%s\n",
          typenames[(int)ses->type], ses->channel, ses->privkey,
          (ses->params != NULL ? " " : ""),
          (ses->params != NULL ? ses->params : "")) < 0) {
    sesError(ses, "MEMORY_ERROR");
  }
}

static void aioSesConnected(Sam3ASession *ses) {
  int res;
  socklen_t len = sizeof(res);
  //
  if (getsockopt(ses->fd, SOL_SOCKET, SO_ERROR, &res, &len) == 0 && res == 0) {
    // ok, connected
    if (sam3aSesStartHandshake(ses, NULL) < 0)
      sesError(ses, NULL);
  } else {
    // connection error
    sesError(ses, "CONNECTION_ERROR");
  }
}

////////////////////////////////////////////////////////////////////////////////
int sam3aCreateSessionEx(Sam3ASession *ses, const Sam3ASessionCallbacks *cb,
                         const char *hostname, int port, const char *privkey,
                         Sam3ASessionType type, const char *params,
                         int timeoutms) {
  if (ses != NULL) {
    // int complete = 0;
    //
    memset(ses, 0, sizeof(Sam3ASession));
    ses->fd = -1;
    if (cb != NULL)
      ses->cb = *cb;
    if (hostname == NULL || !hostname[0])
      hostname = "127.0.0.1";
    if (port < 0 || port > 65535)
      goto error;
    if (privkey != NULL && strlen(privkey) != SAM3A_PRIVKEY_SIZE)
      goto error;
    if ((int)type < 0 || (int)type > 2)
      goto error;
    if (privkey == NULL)
      privkey = "TRANSIENT";
    strcpy(ses->privkey, privkey);
    if (params != NULL && (ses->params = strdup(params)) == NULL)
      goto error;
    ses->timeoutms = timeoutms;
    //
    if (!port)
      port = DEFAULT_TCP_PORT;
    ses->type = type;
    ses->port = (type == SAM3A_SESSION_STREAM ? port : DEFAULT_UDP_PORT);
    if ((ses->ip = sam3aResolveHost(hostname)) == 0)
      goto error;
    sam3aGenChannelName(ses->channel, 32, 64);
    if (libsam3a_debug)
      fprintf(stderr, "sam3aCreateSession: channel=[%s]\n", ses->channel);
    //
    ses->aio.udata = aioSesHandshacked;
    ses->cbAIOProcessorW = aioSesConnected;
    if ((ses->fd = sam3aConnect(ses->ip, port, NULL)) < 0)
      goto error;
    /*
    if (complete) {
      ses->cbAIOProcessorW(ses);
      if (!sam3aIsActiveSession(ses)) return -1;
    }
    */
    //
    return 0; // ok, connection process initiated
  error:
    if (ses->fd >= 0)
      sam3aDisconnect(ses->fd);
    if (ses->params != NULL)
      free(ses->params);
    memset(ses, 0, sizeof(Sam3ASession));
    ses->fd = -1;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
int sam3aCancelSession(Sam3ASession *ses) {
  if (ses != NULL) {
    sesDisconnect(ses);
    return 0;
  }
  return -1;
}

int sam3aCloseSession(Sam3ASession *ses) {
  if (ses != NULL) {
    sam3aCancelSession(ses);
    while (ses->connlist != NULL)
      sam3aCloseConnection(ses->connlist);
    if (ses->cb.cbDestroy != NULL)
      ses->cb.cbDestroy(ses);
    if (ses->params != NULL) {
      free(ses->params);
      ses->params = NULL;
    }
    memset(ses, 0, sizeof(Sam3ASession));
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
static void aioSesKeyGenChecker(Sam3ASession *ses) {
  SAMFieldList *rep = sam3aParseReply(ses->aio.data);
  //
  if (rep == NULL) {
    sesError(ses, NULL);
    return;
  }
  if (sam3aIsGoodReply(rep, "DEST", "REPLY", NULL, NULL)) {
    const char *pub = sam3aFindField(rep, "PUB"),
               *priv = sam3aFindField(rep, "PRIV");
    //
    if (pub != NULL && strlen(pub) == SAM3A_PUBKEY_SIZE && priv != NULL &&
        strlen(priv) == SAM3A_PRIVKEY_SIZE) {
      strcpy(ses->pubkey, pub);
      strcpy(ses->privkey, priv);
      sam3aFreeFieldList(rep);
      if (ses->cb.cbCreated != NULL)
        ses->cb.cbCreated(ses);
      sam3aCancelSession(ses);
      return;
    }
  }
  sam3aFreeFieldList(rep);
  sesError(ses, NULL);
}

// handshake for SESSION CREATE complete
static void aioSesKeyGenHandshacked(Sam3ASession *ses) {
  if (aioSesSendCmdWaitReply(ses, aioSesKeyGenChecker, "%s\n",
                             "DEST GENERATE") < 0) {
    sesError(ses, "MEMORY_ERROR");
  }
}

int sam3aGenerateKeysEx(Sam3ASession *ses, const Sam3ASessionCallbacks *cb,
                        const char *hostname, int port, int timeoutms) {
  if (ses != NULL) {
    memset(ses, 0, sizeof(Sam3ASession));
    ses->fd = -1;
    if (cb != NULL)
      ses->cb = *cb;
    if (hostname == NULL || !hostname[0])
      hostname = "127.0.0.1";
    if (port < 0 || port > 65535)
      goto error;
    ses->timeoutms = timeoutms;
    //
    if (!port)
      port = DEFAULT_TCP_PORT;
    ses->port = port;
    if ((ses->ip = sam3aResolveHost(hostname)) == 0)
      goto error;
    //
    ses->aio.udata = aioSesKeyGenHandshacked;
    ses->cbAIOProcessorW = aioSesConnected;
    if ((ses->fd = sam3aConnect(ses->ip, port, NULL)) < 0)
      goto error;
    //
    return 0; // ok, connection process initiated
  error:
    if (ses->fd >= 0)
      sam3aDisconnect(ses->fd);
    if (ses->params != NULL)
      free(ses->params);
    memset(ses, 0, sizeof(Sam3ASession));
    ses->fd = -1;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
static void aioSesNameResChecker(Sam3ASession *ses) {
  SAMFieldList *rep = sam3aParseReply(ses->aio.data);
  //
  if (rep == NULL) {
    sesError(ses, NULL);
    return;
  }
  if (sam3aIsGoodReply(rep, "NAMING", "REPLY", "RESULT", NULL)) {
    const char *rs = sam3aFindField(rep, "RESULT"),
               *pub = sam3aFindField(rep, "VALUE");
    //
    if (strcmp(rs, "OK") == 0) {
      if (pub != NULL && strlen(pub) == SAM3A_PUBKEY_SIZE) {
        strcpy(ses->destkey, pub);
        sam3aFreeFieldList(rep);
        if (ses->cb.cbCreated != NULL)
          ses->cb.cbCreated(ses);
        sam3aCancelSession(ses);
        return;
      }
      sam3aFreeFieldList(rep);
      sesError(ses, NULL);
    } else {
      sesError(ses, rs);
      sam3aFreeFieldList(rep);
    }
  }
}

// handshake for SESSION CREATE complete
static void aioSesNameResHandshacked(Sam3ASession *ses) {
  if (aioSesSendCmdWaitReply(ses, aioSesNameResChecker,
                             "NAMING LOOKUP NAME=%s\n", ses->params) < 0) {
    sesError(ses, "MEMORY_ERROR");
  }
}

int sam3aNameLookupEx(Sam3ASession *ses, const Sam3ASessionCallbacks *cb,
                      const char *hostname, int port, const char *name,
                      int timeoutms) {
  if (ses != NULL) {
    memset(ses, 0, sizeof(Sam3ASession));
    ses->fd = -1;
    if (cb != NULL)
      ses->cb = *cb;
    if (name == NULL || !name[0] ||
        (name[0] && toupper(name[0]) == 'M' && name[1] &&
         toupper(name[1]) == 'E' && (!name[2] || isspace(name[2]))))
      goto error;
    if (hostname == NULL || !hostname[0])
      hostname = "127.0.0.1";
    if (port < 0 || port > 65535)
      goto error;
    if ((ses->params = strdup(name)) == NULL)
      goto error;
    ses->timeoutms = timeoutms;
    //
    if (!port)
      port = DEFAULT_TCP_PORT;
    ses->port = port;
    if ((ses->ip = sam3aResolveHost(hostname)) == 0)
      goto error;
    //
    ses->aio.udata = aioSesNameResHandshacked;
    ses->cbAIOProcessorW = aioSesConnected;
    if ((ses->fd = sam3aConnect(ses->ip, port, NULL)) < 0)
      goto error;
    //
    return 0; // ok, connection process initiated
  error:
    if (ses->fd >= 0)
      sam3aDisconnect(ses->fd);
    if (ses->params != NULL)
      free(ses->params);
    memset(ses, 0, sizeof(Sam3ASession));
    ses->fd = -1;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
static void aioConnCmdReplyReader(Sam3AConnection *conn) {
  int res = aioLineReader(conn->fd, &conn->aio);
  //
  if (res < 0) {
    connError(conn, "IO_ERROR");
    return;
  }
  if (res > 0) {
    // we got full line
    if (libsam3a_debug)
      fprintf(stderr, "CMDREPLY: %s\n", conn->aio.data);
    if (conn->aio.cbReplyCheckConn != NULL)
      conn->aio.cbReplyCheckConn(conn);
  }
}

static void aioConnCmdSender(Sam3AConnection *conn) {
  if (conn->aio.dataPos < conn->aio.dataUsed) {
    if (aioSender(conn->fd, &conn->aio) < 0) {
      connError(conn, "IO_ERROR");
      return;
    }
  }
  //
  if (conn->aio.dataPos == conn->aio.dataUsed) {
    // hello sent, now wait for reply
    // 2048 bytes of reply line should be enough
    if (conn->aio.dataSize < 2049) {
      char *n = realloc(conn->aio.data, 2049);
      //
      if (n == NULL) {
        connError(conn, "MEMORY_ERROR");
        return;
      }
      conn->aio.data = n;
      conn->aio.dataSize = 2049;
    }
    conn->aio.dataUsed = 2048;
    conn->aio.dataPos = 0;
    conn->cbAIOProcessorR = aioConnCmdReplyReader;
    conn->cbAIOProcessorW = NULL;
  }
}

static __attribute__((format(printf, 3, 4))) int
aioConnSendCmdWaitReply(Sam3AConnection *conn,
                        void (*cbCheck)(Sam3AConnection *conn), const char *fmt,
                        ...) {
  va_list ap;
  char *str;
  int len;
  //
  va_start(ap, fmt);
  str = sam3PrintfVA(&len, fmt, ap);
  va_end(ap);
  //
  if (str == NULL)
    return -1;
  if (conn->aio.data != NULL)
    free(conn->aio.data);
  conn->aio.data = str;
  conn->aio.dataUsed = len;
  conn->aio.dataSize = len + 1;
  conn->aio.dataPos = 0;
  conn->aio.cbReplyCheckConn = cbCheck;
  conn->cbAIOProcessorR = NULL;
  conn->cbAIOProcessorW = aioConnCmdSender;
  //
  if (libsam3a_debug)
    fprintf(stderr, "CMD: %s", str);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
static void aioConnDataReader(Sam3AConnection *conn) {
  char *buf = NULL;
  int bufsz = 0;
  //
  while (sam3aIsActiveConnection(conn)) {
    int av = sam3aBytesAvail(conn->fd), rd;
    //
    if (av < 0) {
      if (buf != NULL)
        free(buf);
      connError(conn, "IO_ERROR");
      return;
    }
    if (av == 0)
      av = 1;
    if (bufsz < av) {
      char *n = realloc(buf, av + 1);
      //
      if (n == NULL) {
        if (buf != NULL)
          free(buf);
        connError(conn, "IO_ERROR");
        return;
      }
      buf = n;
      bufsz = av;
    }
    memset(buf, 0, av + 1);
    //
    rd = recv(conn->fd, buf, av, 0);
    //
    if (rd < 0) {
      if (errno == EINTR)
        continue; // interrupted by signal
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break; // no more data
      free(buf);
      connError(conn, "IO_ERROR");
      return;
    }
    //
    if (rd == 0) {
      // connection closed
      free(buf);
      connDisconnect(conn);
      return;
    }
    //
    if (conn->cb.cbRead != NULL)
      conn->cb.cbRead(conn, buf, rd);
  }
  free(buf);
}

static void aioConnDataWriter(Sam3AConnection *conn) {
  if (!sam3aIsActiveConnection(conn)) {
    conn->aio.dataPos = conn->aio.dataUsed = 0;
    return;
  }
  //
  if (conn->aio.dataPos >= conn->aio.dataUsed) {
    conn->aio.dataPos = conn->aio.dataUsed = 0;
    return;
  }
  //
  while (sam3aIsActiveConnection(conn) &&
         conn->aio.dataPos < conn->aio.dataUsed) {
    int wr = sam3aSendBytes(conn->fd, conn->aio.data + conn->aio.dataPos,
                            conn->aio.dataUsed - conn->aio.dataPos);
    //
    if (wr < 0) {
      connError(conn, "IO_ERROR");
      return;
    }
    if (wr == 0)
      break; // can't write more bytes
    conn->aio.dataPos += wr;
    if (conn->aio.dataPos < conn->aio.dataUsed) {
      memmove(conn->aio.data, conn->aio.data + conn->aio.dataPos,
              conn->aio.dataUsed - conn->aio.dataPos);
      conn->aio.dataUsed -= conn->aio.dataPos;
      conn->aio.dataPos = 0;
    }
  }
  //
  if (conn->aio.dataPos >= conn->aio.dataUsed) {
    conn->aio.dataPos = conn->aio.dataUsed = 0;
    if (conn->cb.cbSent != NULL)
      conn->cb.cbSent(conn);
    if (conn->aio.dataSize > 8192) {
      // shrink buffer
      char *nn = realloc(conn->aio.data, 8192);
      //
      if (nn != NULL) {
        conn->aio.data = nn;
        conn->aio.dataSize = 8192;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
static void aioConnHelloChecker(Sam3AConnection *conn) {
  SAMFieldList *rep = sam3aParseReply(conn->aio.data);
  //
  if (rep != NULL && sam3aIsGoodReply(rep, "HELLO", "REPLY", "RESULT", "OK") &&
      sam3aIsGoodReply(rep, NULL, NULL, "VERSION", "3.0")) {
    conn->cbAIOProcessorR = conn->cbAIOProcessorW = NULL;
    sam3aFreeFieldList(rep);
    if (conn->aio.udata != NULL) {
      void (*cbComplete)(Sam3AConnection * conn) = conn->aio.udata;
      //
      cbComplete(conn);
    }
  } else {
    sam3aFreeFieldList(rep);
    connError(conn, NULL);
  }
}

static int sam3aConnStartHandshake(Sam3AConnection *conn,
                                   void (*cbComplete)(Sam3AConnection *conn)) {
  if (cbComplete != NULL)
    conn->aio.udata = cbComplete;
  if (aioConnSendCmdWaitReply(conn, aioConnHelloChecker, "%s\n",
                              "HELLO VERSION MIN=3.0 MAX=3.0") < 0)
    return -1;
  return 0;
}

static void aioConnConnected(Sam3AConnection *conn) {
  int res;
  socklen_t len = sizeof(res);
  //
  if (getsockopt(conn->fd, SOL_SOCKET, SO_ERROR, &res, &len) == 0 && res == 0) {
    // ok, connected
    if (sam3aConnStartHandshake(conn, NULL) < 0)
      connError(conn, NULL);
  } else {
    // connection error
    connError(conn, "CONNECTION_ERROR");
  }
}

////////////////////////////////////////////////////////////////////////////////
static void aioConnConnectChecker(Sam3AConnection *conn) {
  SAMFieldList *rep = sam3aParseReply(conn->aio.data);
  //
  if (rep == NULL) {
    connError(conn, NULL);
    return;
  }
  if (!sam3aIsGoodReply(rep, "STREAM", "STATUS", "RESULT", "OK")) {
    const char *v = sam3aFindField(rep, "RESULT");
    //
    connError(conn, v);
    sam3aFreeFieldList(rep);
  } else {
    // no error
    sam3aFreeFieldList(rep);
    conn->callDisconnectCB = 1;
    conn->cbAIOProcessorR = aioConnDataReader;
    conn->cbAIOProcessorW = aioConnDataWriter;
    conn->aio.dataPos = conn->aio.dataUsed = 0;
    if (conn->cb.cbConnected != NULL)
      conn->cb.cbConnected(conn);
    // indicate that we are ready for new data
    if (sam3aIsActiveConnection(conn) && conn->cb.cbSent != NULL)
      conn->cb.cbSent(conn);
  }
}

// handshake for SESSION CREATE complete
static void aioConConnectHandshacked(Sam3AConnection *conn) {
  if (aioConnSendCmdWaitReply(conn, aioConnConnectChecker,
                              "STREAM CONNECT ID=%s DESTINATION=%s\n",
                              conn->ses->channel, conn->destkey) < 0) {
    connError(conn, "MEMORY_ERROR");
  }
}

////////////////////////////////////////////////////////////////////////////////
Sam3AConnection *sam3aStreamConnectEx(Sam3ASession *ses,
                                      const Sam3AConnectionCallbacks *cb,
                                      const char *destkey, int timeoutms) {
  if (sam3aIsActiveSession(ses) && ses->type == SAM3A_SESSION_STREAM &&
      destkey != NULL && strlen(destkey) == SAM3A_PUBKEY_SIZE) {
    Sam3AConnection *conn = calloc(1, sizeof(Sam3AConnection));
    //
    if (conn == NULL)
      return NULL;
    if (cb != NULL)
      conn->cb = *cb;
    strcpy(conn->destkey, destkey);
    conn->timeoutms = timeoutms;
    //
    conn->aio.udata = aioConConnectHandshacked;
    conn->cbAIOProcessorW = aioConnConnected;
    if ((conn->fd = sam3aConnect(ses->ip, ses->port, NULL)) < 0)
      goto error;
    //
    conn->ses = ses;
    conn->next = ses->connlist;
    ses->connlist = conn;
    return conn; // ok, connection process initiated
  error:
    if (conn->fd >= 0)
      sam3aDisconnect(conn->fd);
    memset(conn, 0, sizeof(Sam3AConnection));
    free(conn);
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
static void aioConnAcceptCheckerA(Sam3AConnection *conn) {
  SAMFieldList *rep = sam3aParseReply(conn->aio.data);
  //
  if (rep != NULL || strlen(conn->aio.data) != SAM3A_PUBKEY_SIZE ||
      !sam3aIsValidPubKey(conn->aio.data)) {
    sam3aFreeFieldList(rep);
    connError(conn, NULL);
    return;
  }
  sam3aFreeFieldList(rep);
  strcpy(conn->destkey, conn->aio.data);
  conn->callDisconnectCB = 1;
  conn->cbAIOProcessorR = aioConnDataReader;
  conn->cbAIOProcessorW = aioConnDataWriter;
  conn->aio.dataPos = conn->aio.dataUsed = 0;
  if (conn->cb.cbAccepted != NULL)
    conn->cb.cbAccepted(conn);
  // indicate that we are ready for new data
  if (sam3aIsActiveConnection(conn) && conn->cb.cbSent != NULL)
    conn->cb.cbSent(conn);
}

static void aioConnAcceptChecker(Sam3AConnection *conn) {
  SAMFieldList *rep = sam3aParseReply(conn->aio.data);
  //
  if (rep == NULL) {
    connError(conn, NULL);
    return;
  }
  if (!sam3aIsGoodReply(rep, "STREAM", "STATUS", "RESULT", "OK")) {
    const char *v = sam3aFindField(rep, "RESULT");
    //
    connError(conn, v);
    sam3aFreeFieldList(rep);
  } else {
    // no error
    sam3aFreeFieldList(rep);
    // 2048 bytes of reply line should be enough
    if (conn->aio.dataSize < 2049) {
      char *n = realloc(conn->aio.data, 2049);
      //
      if (n == NULL) {
        connError(conn, "MEMORY_ERROR");
        return;
      }
      conn->aio.data = n;
      conn->aio.dataSize = 2049;
    }
    conn->aio.dataUsed = 2048;
    conn->aio.dataPos = 0;
    conn->cbAIOProcessorR = aioConnCmdReplyReader;
    conn->cbAIOProcessorW = NULL;
    conn->aio.cbReplyCheckConn = aioConnAcceptCheckerA;
  }
}

// handshake for SESSION CREATE complete
static void aioConAcceptHandshacked(Sam3AConnection *conn) {
  if (aioConnSendCmdWaitReply(conn, aioConnAcceptChecker,
                              "STREAM ACCEPT ID=%s\n",
                              conn->ses->channel) < 0) {
    connError(conn, "MEMORY_ERROR");
  }
}

////////////////////////////////////////////////////////////////////////////////
Sam3AConnection *sam3aStreamAcceptEx(Sam3ASession *ses,
                                     const Sam3AConnectionCallbacks *cb,
                                     int timeoutms) {
  if (sam3aIsActiveSession(ses) && ses->type == SAM3A_SESSION_STREAM) {
    Sam3AConnection *conn = calloc(1, sizeof(Sam3AConnection));
    //
    if (conn == NULL)
      return NULL;
    if (cb != NULL)
      conn->cb = *cb;
    conn->timeoutms = timeoutms;
    //
    conn->aio.udata = aioConAcceptHandshacked;
    conn->cbAIOProcessorW = aioConnConnected;
    if ((conn->fd = sam3aConnect(ses->ip, ses->port, NULL)) < 0)
      goto error;
    //
    conn->ses = ses;
    conn->next = ses->connlist;
    ses->connlist = conn;
    return conn; // ok, connection process initiated
  error:
    if (conn->fd >= 0)
      sam3aDisconnect(conn->fd);
    memset(conn, 0, sizeof(Sam3AConnection));
    free(conn);
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
int sam3aSend(Sam3AConnection *conn, const void *data, int datasize) {
  if (datasize == -1)
    datasize = (data != NULL ? strlen((const char *)data) : 0);
  //
  if (sam3aIsActiveConnection(conn) && conn->callDisconnectCB &&
      conn->cbAIOProcessorW != NULL &&
      ((datasize > 0 && data != NULL) || datasize == 0)) {
    // try to add data to send buffer
    if (datasize > 0) {
      if (conn->aio.dataUsed + datasize > conn->aio.dataSize) {
        // we need more pepper!
        int newsz = conn->aio.dataUsed + datasize;
        char *nb = realloc(conn->aio.data, newsz);
        //
        if (nb == NULL)
          return -1; // alas
        conn->aio.data = nb;
        conn->aio.dataSize = newsz;
      }
      //
      memcpy(conn->aio.data + conn->aio.dataUsed, data, datasize);
      conn->aio.dataUsed += datasize;
    }
    return 0;
  }
  //
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
int sam3aIsHaveActiveConnections(const Sam3ASession *ses) {
  if (sam3aIsActiveSession(ses)) {
    for (const Sam3AConnection *c = ses->connlist; c != NULL; c = c->next) {
      if (sam3aIsActiveConnection(c))
        return 1;
    }
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
int sam3aCancelConnection(Sam3AConnection *conn) {
  if (conn != NULL) {
    connDisconnect(conn);
    return 0;
  }
  return -1;
}

int sam3aCloseConnection(Sam3AConnection *conn) {
  if (conn != NULL) {
    sam3aCancelConnection(conn);
    if (conn->cb.cbDestroy != NULL)
      conn->cb.cbDestroy(conn);
    for (Sam3AConnection *p = NULL, *c = conn->ses->connlist; c != NULL;
         p = c, c = c->next) {
      if (c == conn) {
        // got it!
        if (p == NULL)
          c->ses->connlist = c->next;
        else
          p->next = c->next;
        break;
      }
    }
    if (conn->params != NULL) {
      free(conn->params);
      conn->params = NULL;
    }
    memset(conn, 0, sizeof(Sam3AConnection));
    free(conn);
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
int sam3aAddSessionToFDS(Sam3ASession *ses, int maxfd, fd_set *rds,
                         fd_set *wrs) {
  if (ses != NULL) {
    if (sam3aIsActiveSession(ses)) {
      if (rds != NULL && ses->cbAIOProcessorR != NULL) {
        if (maxfd < ses->fd)
          maxfd = ses->fd;
        FD_SET(ses->fd, rds);
      }
      //
      if (wrs != NULL && ses->cbAIOProcessorW != NULL) {
        if (maxfd < ses->fd)
          maxfd = ses->fd;
        FD_SET(ses->fd, wrs);
      }
      //
      for (Sam3AConnection *c = ses->connlist; c != NULL; c = c->next) {
        if (sam3aIsActiveConnection(c)) {
          if (rds != NULL && c->cbAIOProcessorR != NULL) {
            if (maxfd < c->fd)
              maxfd = c->fd;
            FD_SET(c->fd, rds);
          }
          //
          if (wrs != NULL && c->cbAIOProcessorW != NULL) {
            if (!c->callDisconnectCB || (c->aio.dataPos < c->aio.dataUsed))
              if (maxfd < c->fd)
                maxfd = c->fd;
            FD_SET(c->fd, wrs);
          }
        }
      }
    }
    return maxfd;
  }
  //
  return -1;
}

void sam3aProcessSessionIO(Sam3ASession *ses, fd_set *rds, fd_set *wrs) {
  if (sam3aIsActiveSession(ses)) {
    if (ses->fd >= 0 && !ses->cancelled && ses->cbAIOProcessorR != NULL &&
        rds != NULL && FD_ISSET(ses->fd, rds))
      ses->cbAIOProcessorR(ses);
    if (ses->fd >= 0 && !ses->cancelled && ses->cbAIOProcessorW != NULL &&
        wrs != NULL && FD_ISSET(ses->fd, wrs))
      ses->cbAIOProcessorW(ses);
    //
    for (Sam3AConnection *c = ses->connlist; c != NULL; c = c->next) {
      if (c->fd >= 0 && !c->cancelled && c->cbAIOProcessorR != NULL &&
          rds != NULL && FD_ISSET(c->fd, rds))
        c->cbAIOProcessorR(c);
      if (c->fd >= 0 && !c->cancelled && c->cbAIOProcessorW != NULL &&
          wrs != NULL && FD_ISSET(c->fd, wrs))
        c->cbAIOProcessorW(c);
    }
  }
}

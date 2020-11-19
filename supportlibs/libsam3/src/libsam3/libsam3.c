/* This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 *
 * I2P-Bote:
 * 5m77dFKGEq6~7jgtrfw56q3t~SmfwZubmGdyOLQOPoPp8MYwsZ~pfUCwud6LB1EmFxkm4C3CGlzq-hVs9WnhUV
 * we are the Borg. */
#include "libsam3.h"

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
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#endif
////////////////////////////////////////////////////////////////////////////////
int libsam3_debug = 0;

////////////////////////////////////////////////////////////////////////////////
/* convert struct timeval to milliseconds */
/*
static inline uint64_t timeval2ms (const struct timeval *tv) {
  return ((uint64_t)tv->tv_sec)*1000+((uint64_t)tv->tv_usec)/1000;
}
*/

/* convert milliseconds to timeval struct */
static inline void ms2timeval(struct timeval *tv, uint64_t ms) {
  tv->tv_sec = ms / 1000;
  tv->tv_usec = (ms % 1000) * 1000;
}

int sam3tcpSetTimeoutSend(int fd, int timeoutms) {
  if (fd >= 0 && timeoutms >= 0) {
    struct timeval tv;
    //
    ms2timeval(&tv, timeoutms);
    return (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0 ? -1
                                                                         : 0);
  }
  return -1;
}

int sam3tcpSetTimeoutReceive(int fd, int timeoutms) {
  if (fd >= 0 && timeoutms >= 0) {
    struct timeval tv;
    //
    ms2timeval(&tv, timeoutms);
    return (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0 ? -1
                                                                         : 0);
  }
  return -1;
}

int sam3CheckValidKeyLength(const char *pubkey) {
  if (strlen(pubkey) >= SAM3_PUBKEY_SIZE &&
      strlen(pubkey) <= SAM3_PUBKEY_SIZE + SAM3_CERT_SIZE) {
    return 1;
  }
  return 0;
}

int sam3tcpConnectIP(uint32_t ip, int port) {
  struct sockaddr_in addr;
  int fd, val = 1;
  char ipstr[18];
  //
  if (ip == 0 || ip == 0xffffffffUL || port < 1 || port > 65535)
    return -1;
  //
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    if (libsam3_debug)
      fprintf(stderr, "ERROR: can't create socket\n");
    return -1;
  }
  //
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = ip;
  //
  ipstr[0] = 0;
  if (libsam3_debug) {
    if (getnameinfo((struct sockaddr *)&addr, sizeof(struct sockaddr_in), ipstr,
                    sizeof(ipstr), NULL, 0, NI_NUMERICHOST) == 0) {
      fprintf(stderr, "connecting to [%s:%d]...\n", ipstr, port);
    }
  }
  //
  setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
  //
  if (connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
    if (libsam3_debug)
      fprintf(stderr, "ERROR: can't connect\n");
    close(fd);
    return -1;
  }
  //
  if (libsam3_debug && ipstr[0])
    fprintf(stderr, "connected to [%s:%d]\n", ipstr, port);
  //
  return fd;
}

/* returns fd or -1 */
int sam3tcpConnect(const char *hostname, int port, uint32_t *ip) {
  struct hostent *host = NULL;
  //
  if (hostname == NULL || !hostname[0] || port < 1 || port > 65535)
    return -1;
  //
  host = gethostbyname(hostname);
  if (host == NULL || host->h_name == NULL || !host->h_name[0]) {
    if (libsam3_debug)
      fprintf(stderr, "ERROR: can't resolve '%s'\n", hostname);
    return -1;
  }
  //
  if (libsam3_debug) {
    char ipstr[18];
    //
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((struct in_addr *)host->h_addr);
    //
    if (getnameinfo((struct sockaddr *)&addr, sizeof(struct sockaddr_in), ipstr,
                    sizeof(ipstr), NULL, 0, NI_NUMERICHOST) == 0) {
      fprintf(stderr, "resolving: %s is [%s]...\n", hostname, ipstr);
    }
  }
  //
  if (ip != NULL)
    *ip = ((struct in_addr *)host->h_addr)->s_addr;
  return sam3tcpConnectIP(((struct in_addr *)host->h_addr)->s_addr, port);
}

// <0: error; 0: ok
int sam3tcpDisconnect(int fd) {
  if (fd >= 0) {
    shutdown(fd, SHUT_RDWR);
    return close(fd);
  }
  //
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
// <0: error; 0: ok
int sam3tcpSend(int fd, const void *buf, size_t bufSize) {
  const char *c = (const char *)buf;
  //
  if (fd < 0 || (buf == NULL && bufSize > 0))
    return -1;
  //
  while (bufSize > 0) {
    int wr = send(fd, c, bufSize, MSG_NOSIGNAL);
    //
    if (wr < 0 && errno == EINTR)
      continue; // interrupted by signal
    if (wr <= 0)
      return -1; // either error or
    c += wr;
    bufSize -= wr;
  }
  //
  return 0;
}

/* <0: received (-res) bytes; read error */
/* can return less that requesten bytes even if `allowPartial` is 0 when
 * connection is closed */
ssize_t sam3tcpReceiveEx(int fd, void *buf, size_t bufSize, int allowPartial) {
  char *c = (char *)buf;
  ssize_t total = 0;
  //
  if (fd < 0 || (buf == NULL && bufSize > 0))
    return -1;
  //
  while (bufSize > 0) {
    int rd = recv(fd, c, bufSize, 0);
    //
    if (rd < 0 && errno == EINTR)
      continue; // interrupted by signal
    if (rd == 0)
      return total;
    if (rd < 0)
      return -total;
    c += rd;
    total += rd;
    bufSize -= rd;
    if (allowPartial)
      break;
  }
  //
  return total;
}

ssize_t sam3tcpReceive(int fd, void *buf, size_t bufSize) {
  return sam3tcpReceiveEx(fd, buf, bufSize, 0);
}

////////////////////////////////////////////////////////////////////////////////
__attribute__((format(printf, 2, 3))) int sam3tcpPrintf(int fd, const char *fmt,
                                                        ...) {
  int res;
  char buf[1024], *p = buf;
  int size = sizeof(buf) - 1;
  //
  for (;;) {
    va_list ap;
    char *np;
    int n;
    //
    va_start(ap, fmt);
    n = vsnprintf(p, size, fmt, ap);
    va_end(ap);
    //
    if (n > -1 && n < size)
      break;
    if (n > -1)
      size = n + 1;
    else
      size *= 2;
    if (p == buf) {
      if ((p = malloc(size + 4)) == NULL)
        return -1;
    } else {
      if ((np = realloc(p, size + 4)) == NULL) {
        free(p);
        return -1;
      }
      p = np;
    }
  }
  //
  if (libsam3_debug)
    fprintf(stderr, "SENDING: %s", p);
  res = sam3tcpSend(fd, p, strlen(p));
  if (p != buf)
    free(p);
  return res;
}

int sam3tcpReceiveStr(int fd, char *dest, size_t maxSize) {
  char *d = dest;
  //
  if (maxSize < 1 || fd < 0 || dest == NULL)
    return -1;
  memset(dest, 0, maxSize);
  while (maxSize > 1) {
    char *e;
    int rd = recv(fd, d, maxSize - 1, MSG_PEEK);
    //
    if (rd < 0 && errno == EINTR)
      continue; // interrupted by signal
    if (rd == 0) {
      rd = recv(fd, d, 1, 0);
      if (rd < 0 && errno == EINTR)
        continue; // interrupted by signal
      if (d[0] == '\n') {
        d[0] = 0; // remove '\n'
        return 0;
      }
    } else {
      if (rd < 0)
        return -1; // error or connection closed; alas
    }
    // check for EOL
    d[maxSize - 1] = 0;
    if ((e = strchr(d, '\n')) != NULL) {
      rd = e - d + 1; // bytes to receive
      if (sam3tcpReceive(fd, d, rd) < 0)
        return -1;   // alas
      d[rd - 1] = 0; // remove '\n'
      return 0;      // done
    } else {
      // let's receive this part and go on
      if (sam3tcpReceive(fd, d, rd) < 0)
        return -1; // alas
      maxSize -= rd;
      d += rd;
    }
  }
  // alas, the string is too big
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
int sam3udpSendToIP(uint32_t ip, int port, const void *buf, size_t bufSize) {
  // TODO: ipv6
  struct sockaddr_in addr;
  int fd, res;
  //
  if (buf == NULL || bufSize < 1)
    return -1;
  if (port < 1 || port > 65535)
    port = 7655;
  //
  if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    if (libsam3_debug)
      fprintf(stderr, "ERROR: can't create socket\n");
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
    if (libsam3_debug) {
      res = errno;
      fprintf(stderr, "UDP ERROR (%d): %s\n", res, strerror(res));
    }
    res = -1;
  } else {
    if (libsam3_debug)
      fprintf(stderr, "UDP: %d bytes sent\n", res);
  }
  //
  close(fd);
  //
  return (res >= 0 ? 0 : -1);
}

int sam3udpSendTo(const char *hostname, int port, const void *buf,
                  size_t bufSize, uint32_t *ip) {
  struct hostent *host = NULL;
  // TODO: ipv6
  if (buf == NULL || bufSize < 1)
    return -1;
  if (hostname == NULL || !hostname[0])
    hostname = "localhost";
  if (port < 1 || port > 65535)
    port = 7655;
  //
  host = gethostbyname(hostname);
  if (host == NULL || host->h_name == NULL || !host->h_name[0]) {
    if (libsam3_debug)
      fprintf(stderr, "ERROR: can't resolve '%s'\n", hostname);
    return -1;
  }
  //
  if (ip != NULL)
    *ip = ((struct in_addr *)host->h_addr)->s_addr;
  return sam3udpSendToIP(((struct in_addr *)host->h_addr)->s_addr, port, buf,
                         bufSize);
}

////////////////////////////////////////////////////////////////////////////////
void sam3FreeFieldList(SAMFieldList *list) {
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

void sam3DumpFieldList(const SAMFieldList *list) {
  for (; list != NULL; list = list->next) {
    fprintf(stderr, "%s=[%s]\n", list->name, list->value);
  }
}

const char *sam3FindField(const SAMFieldList *list, const char *field) {
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

SAMFieldList *sam3ParseReply(const char *rep) {
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
    if (libsam3_debug)
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
  if (libsam3_debug)
    sam3DumpFieldList(first);
  //
  return first;
error:
  sam3FreeFieldList(first);
  return NULL;
}

// NULL: error; else: list of fields
// first item is always 2-word reply, with first word in name and second in
// value
SAMFieldList *sam3ReadReply(int fd) {
  char rep[2048]; // should be enough for any reply
  //
  if (sam3tcpReceiveStr(fd, rep, sizeof(rep)) < 0)
    return NULL;
  if (libsam3_debug)
    fprintf(stderr, "SAM REPLY: [%s]\n", rep);
  return sam3ParseReply(rep);
}

// example:
//   r0: 'HELLO'
//   r1: 'REPLY'
//   field: NULL or 'RESULT'
//   VALUE: NULL or 'OK'
// returns bool
int sam3IsGoodReply(const SAMFieldList *list, const char *r0, const char *r1,
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
  return hashint(res);
}

////////////////////////////////////////////////////////////////////////////////
size_t sam3GenChannelName(char *dest, size_t minlen, size_t maxlen) {
  BJRandCtx rc;
  size_t len;
  size_t retlen;
  //
  if (dest == NULL || minlen < 1 || maxlen < minlen || minlen > 65536 ||
      maxlen > 65536)
    return -1;
  bjprngInit(&rc, genSeed());
  len = minlen + (bjprngRand(&rc) % (maxlen - minlen + 1));
  retlen = len;
  while (len--) {
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
  return retlen;
}

////////////////////////////////////////////////////////////////////////////////
static int sam3HandshakeInternal(int fd) {
  SAMFieldList *rep = NULL;
  //
  if (sam3tcpPrintf(fd, "HELLO VERSION MIN=3.0 MAX=3.1\n") < 0)
    goto error;
  rep = sam3ReadReply(fd);
  if (!sam3IsGoodReply(rep, "HELLO", "REPLY", "RESULT", "OK"))
    goto error;
  sam3FreeFieldList(rep);
  return fd;
error:
  sam3tcpDisconnect(fd);
  if (rep != NULL)
    sam3FreeFieldList(rep);
  return -1;
}

int sam3HandshakeIP(uint32_t ip, int port) {
  int fd;
  //
  if ((fd = sam3tcpConnectIP(ip, (port < 1 || port > 65535 ? 7656 : port))) < 0)
    return -1;
  return sam3HandshakeInternal(fd);
}

int sam3Handshake(const char *hostname, int port, uint32_t *ip) {
  int fd;
  //
  if ((fd = sam3tcpConnect(
           (hostname == NULL || !hostname[0] ? "localhost" : hostname),
           (port < 1 || port > 65535 ? 7656 : port), ip)) < 0)
    return -1;
  return sam3HandshakeInternal(fd);
}

////////////////////////////////////////////////////////////////////////////////
static inline void strcpyerr(Sam3Session *ses, const char *errstr) {
  memset(ses->error, 0, sizeof(ses->error));
  if (errstr != NULL)
    strncpy(ses->error, errstr, sizeof(ses->error) - 1);
}

int sam3GenerateKeys(Sam3Session *ses, const char *hostname, int port,
                     int sigType) {
  if (ses != NULL) {
    SAMFieldList *rep = NULL;
    int fd, res = -1;
    static const char *sigtypes[5] = {
        "SIGNATURE_TYPE=DSA_SHA1", "SIGNATURE_TYPE=ECDSA_SHA256_P256",
        "SIGNATURE_TYPE=ECDSA_SHA384_P384", "SIGNATURE_TYPE=ECDSA_SHA512_P521",
        "SIGNATURE_TYPE=EdDSA_SHA512_Ed25519"};
    //
    if ((fd = sam3Handshake(hostname, port, NULL)) < 0) {
      strcpyerr(ses, "I2P_ERROR");
      return -1;
    }
    //
    if (sam3tcpPrintf(fd, "DEST GENERATE %s\n", sigtypes[sigType]) >= 0) {
      if ((rep = sam3ReadReply(fd)) != NULL &&
          sam3IsGoodReply(rep, "DEST", "REPLY", NULL, NULL)) {
        const char *pub = sam3FindField(rep, "PUB"),
                   *priv = sam3FindField(rep, "PRIV");
        //
        if (pub != NULL && sam3CheckValidKeyLength(pub) && priv != NULL &&
            strlen(priv) >= SAM3_PRIVKEY_MIN_SIZE) {
          strcpy(ses->pubkey, pub);
          strcpy(ses->privkey, priv);
          res = 0;
        }
      }
    }
    //
    sam3FreeFieldList(rep);
    sam3tcpDisconnect(fd);
    //
    return res;
  }
  return -1;
}

int sam3NameLookup(Sam3Session *ses, const char *hostname, int port,
                   const char *name) {
  if (ses != NULL && name != NULL && name[0]) {
    SAMFieldList *rep = NULL;
    int fd, res = -1;
    //
    if ((fd = sam3Handshake(hostname, port, NULL)) < 0) {
      strcpyerr(ses, "I2P_ERROR");
      return -1;
    }
    //
    strcpyerr(ses, "I2P_ERROR");
    if (sam3tcpPrintf(fd, "NAMING LOOKUP NAME=%s\n", name) >= 0) {
      if ((rep = sam3ReadReply(fd)) != NULL &&
          sam3IsGoodReply(rep, "NAMING", "REPLY", "RESULT", NULL)) {
        const char *rs = sam3FindField(rep, "RESULT"),
                   *pub = sam3FindField(rep, "VALUE");
        //
        if (strcmp(rs, "OK") == 0) {
          if (pub != NULL && sam3CheckValidKeyLength(pub)) {
            strcpy(ses->destkey, pub);
            strcpyerr(ses, NULL);
            res = 0;
          }
        } else if (rs[0]) {
          strcpyerr(ses, rs);
        }
      }
    }
    //
    sam3FreeFieldList(rep);
    sam3tcpDisconnect(fd);
    //
    return res;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
static int sam3CloseConnectionInternal(Sam3Connection *conn) {
  return (conn->fd >= 0 ? sam3tcpDisconnect(conn->fd) : 0);
}

int sam3CloseConnection(Sam3Connection *conn) {
  if (conn != NULL) {
    int res = sam3CloseConnectionInternal(conn);
    //
    if (conn->ses != NULL) {
      for (Sam3Connection *p = NULL, *c = conn->ses->connlist; c != NULL;
           p = c, c = c->next) {
        if (c == conn) {
          if (p == NULL)
            conn->ses->connlist = c->next;
          else
            p->next = c->next;
          break;
        }
      }
    }
    free(conn);
    //
    return res;
  }
  return -1;
}

int sam3CloseSession(Sam3Session *ses) {
  if (ses != NULL) {
    for (Sam3Connection *n, *c = ses->connlist; c != NULL; c = n) {
      n = c->next;
      sam3CloseConnectionInternal(c);
      free(c);
    }
    if (ses->fwd_fd >= 0)
      sam3tcpDisconnect(ses->fwd_fd);
    if (ses->fd >= 0)
      sam3tcpDisconnect(ses->fd);
    memset(ses, 0, sizeof(Sam3Session));
    ses->fd = -1;
    return 0;
  }
  return -1;
}

int sam3CreateSilentSession(Sam3Session *ses, const char *hostname, int port,
                            const char *privkey, Sam3SessionType type,
                            Sam3SigType sigType, const char *params) {
  int r =
      sam3CreateSession(ses, hostname, port, privkey, type, sigType, params);
  if (r != 0) {
    return r;
  }
  ses->silent = true;
  return 0;
}

int sam3CreateSession(Sam3Session *ses, const char *hostname, int port,
                      const char *privkey, Sam3SessionType type,
                      Sam3SigType sigType, const char *params) {
  if (ses != NULL) {
    static const char *typenames[3] = {"RAW", "DATAGRAM", "STREAM"};
    static const char *sigtypes[5] = {
        "SIGNATURE_TYPE=DSA_SHA1", "SIGNATURE_TYPE=ECDSA_SHA256_P256",
        "SIGNATURE_TYPE=ECDSA_SHA384_P384", "SIGNATURE_TYPE=ECDSA_SHA512_P521",
        "SIGNATURE_TYPE=EdDSA_SHA512_Ed25519"};

    SAMFieldList *rep;
    const char *v = NULL;
    const char *pdel = (params != NULL ? " " : "");
    //
    memset(ses, 0, sizeof(Sam3Session));
    ses->fd = -1;
    ses->fwd_fd = -1;
    ses->silent = false;
    //
    if (privkey != NULL && strlen(privkey) < SAM3_PRIVKEY_MIN_SIZE)
      goto error;
    if ((int)type < 0 || (int)type > 2)
      goto error;
    if (privkey == NULL)
      privkey = "TRANSIENT";
    //
    ses->type = type;
    ses->sigType = sigType;
    ses->port = (type == SAM3_SESSION_STREAM ? (port ? port : 7656) : 7655);
    sam3GenChannelName(ses->channel, 32, 64);
    if (libsam3_debug)
      fprintf(stderr, "sam3CreateSession: channel=[%s]\n", ses->channel);
    //
    if ((ses->fd = sam3Handshake(hostname, port, &ses->ip)) < 0)
      goto error;
    //
    if (libsam3_debug)
      fprintf(stderr, "sam3CreateSession: creating session (%s)...\n",
              typenames[(int)type]);
    if (sam3tcpPrintf(
            ses->fd, "SESSION CREATE STYLE=%s ID=%s DESTINATION=%s %s %s %s\n",
            typenames[(int)type], ses->channel, privkey, sigtypes[(int)sigType],
            pdel, (params != NULL ? params : "")) < 0)
      goto error;
    if ((rep = sam3ReadReply(ses->fd)) == NULL)
      goto error;
    if (!sam3IsGoodReply(rep, "SESSION", "STATUS", "RESULT", "OK") ||
        (v = sam3FindField(rep, "DESTINATION")) == NULL ||
        strlen(v) < SAM3_PRIVKEY_MIN_SIZE) {
      if (libsam3_debug)
        fprintf(stderr, "sam3CreateSession: invalid reply (%ld)...\n",
                (v != NULL ? strlen(v) : -1));
      if (libsam3_debug)
        sam3DumpFieldList(rep);
      sam3FreeFieldList(rep);
      goto error;
    }
    // save our keys
    strcpy(ses->privkey, v);
    sam3FreeFieldList(rep);
    // get public key
    if (sam3tcpPrintf(ses->fd, "NAMING LOOKUP NAME=ME\n") < 0)
      goto error;
    if ((rep = sam3ReadReply(ses->fd)) == NULL)
      goto error;
    v = NULL;
    if (!sam3IsGoodReply(rep, "NAMING", "REPLY", "RESULT", "OK") ||
        (v = sam3FindField(rep, "VALUE")) == NULL ||
        !sam3CheckValidKeyLength(v)) {
      if (libsam3_debug)
        fprintf(stderr, "sam3CreateSession: invalid NAMING reply (%ld)...\n",
                (v != NULL ? strlen(v) : -1));
      if (libsam3_debug)
        sam3DumpFieldList(rep);
      sam3FreeFieldList(rep);
      goto error;
    }
    strcpy(ses->pubkey, v);
    sam3FreeFieldList(rep);
    //
    if (libsam3_debug)
      fprintf(stderr, "sam3CreateSession: complete.\n");
    return 0;
  }
error:
  sam3CloseSession(ses);
  return -1;
}

Sam3Connection *sam3StreamConnect(Sam3Session *ses, const char *destkey) {
  if (ses != NULL) {
    SAMFieldList *rep;
    Sam3Connection *conn;
    //
    if (ses->type != SAM3_SESSION_STREAM) {
      strcpyerr(ses, "INVALID_SESSION_TYPE");
      return NULL;
    }
    if (ses->fd < 0) {
      strcpyerr(ses, "INVALID_SESSION");
      return NULL;
    }
    if (destkey == NULL || !sam3CheckValidKeyLength(destkey)) {
      strcpyerr(ses, "INVALID_KEY");
      return NULL;
    }
    if ((conn = calloc(1, sizeof(Sam3Connection))) == NULL) {
      strcpyerr(ses, "NO_MEMORY");
      return NULL;
    }
    if ((conn->fd = sam3HandshakeIP(ses->ip, ses->port)) < 0) {
      strcpyerr(ses, "IO_ERROR_SK");
      goto error;
    }
    if (sam3tcpPrintf(conn->fd,
                      "STREAM CONNECT ID=%s DESTINATION=%s SILENT=%s\n",
                      ses->channel, destkey, checkIsSilent(ses)) < 0) {
      strcpyerr(ses, "IO_ERROR");
      goto error;
    }
    if ((rep = sam3ReadReply(conn->fd)) == NULL) {
      strcpyerr(ses, "IO_ERROR");
      goto error;
    }
    if (!ses->silent) {
      if (!sam3IsGoodReply(rep, "STREAM", "STATUS", "RESULT", "OK")) {
        const char *v = sam3FindField(rep, "RESULT");
        //
        strcpyerr(ses, (v != NULL && v[0] ? v : "I2P_ERROR"));
        sam3CloseConnectionInternal(conn);
        free(conn);
        conn = NULL;
      } else {
        // no error
        strcpyerr(ses, NULL);
      }
    }
    sam3FreeFieldList(rep);
    if (conn != NULL) {
      strcpy(conn->destkey, destkey);
      conn->ses = ses;
      conn->next = ses->connlist;
      ses->connlist = conn;
    }
    return conn;
  error:
    sam3CloseConnectionInternal(conn);
    free(conn);
    return NULL;
  }
  return NULL;
}

Sam3Connection *sam3StreamAccept(Sam3Session *ses) {
  if (ses != NULL) {
    SAMFieldList *rep = NULL;
    char repstr[1024];
    Sam3Connection *conn;
    //
    if (ses->type != SAM3_SESSION_STREAM) {
      strcpyerr(ses, "INVALID_SESSION_TYPE");
      return NULL;
    }
    if (ses->fd < 0) {
      strcpyerr(ses, "INVALID_SESSION");
      return NULL;
    }
    if ((conn = calloc(1, sizeof(Sam3Connection))) == NULL) {
      strcpyerr(ses, "NO_MEMORY");
      return NULL;
    }
    if ((conn->fd = sam3HandshakeIP(ses->ip, ses->port)) < 0) {
      strcpyerr(ses, "IO_ERROR_SK");
      goto error;
    }
    if (sam3tcpPrintf(conn->fd, "STREAM ACCEPT ID=%s\n", ses->channel) < 0) {
      strcpyerr(ses, "IO_ERROR_PF");
      goto error;
    }
    if ((rep = sam3ReadReply(conn->fd)) == NULL) {
      strcpyerr(ses, "IO_ERROR_RP");
      goto error;
    }
    if (!ses->silent) {
      if (!sam3IsGoodReply(rep, "STREAM", "STATUS", "RESULT", "OK")) {
        const char *v = sam3FindField(rep, "RESULT");
        //
        strcpyerr(ses, (v != NULL && v[0] ? v : "I2P_ERROR_RES"));
        goto error;
      }
    }
    if (sam3tcpReceiveStr(conn->fd, repstr, sizeof(repstr)) < 0) {
      strcpyerr(ses, "IO_ERROR_RP1");
      goto error;
    }
    sam3FreeFieldList(rep);
    if ((rep = sam3ParseReply(repstr)) != NULL) {
      const char *v = sam3FindField(rep, "RESULT");
      //
      strcpyerr(ses, (v != NULL && v[0] ? v : "I2P_ERROR_RES1"));
      goto error;
    }
    if (!sam3CheckValidKeyLength(repstr)) {
      strcpyerr(ses, "INVALID_KEY");
      goto error;
    }
    sam3FreeFieldList(rep);
    strcpy(conn->destkey, repstr);
    conn->ses = ses;
    conn->next = ses->connlist;
    ses->connlist = conn;
    strcpyerr(ses, NULL);
    return conn;
  error:
    if (rep != NULL)
      sam3FreeFieldList(rep);
    sam3CloseConnectionInternal(conn);
    free(conn);
    return NULL;
  }
  return NULL;
}

const char *checkIsSilent(Sam3Session *ses) {
  if (ses->silent == true) {
    return "true";
  } else {
    return "false";
  }
}

int sam3StreamForward(Sam3Session *ses, const char *hostname, int port) {
  if (ses != NULL) {
    SAMFieldList *rep = NULL;
    //
    if (ses->type != SAM3_SESSION_STREAM) {
      strcpyerr(ses, "INVALID_SESSION_TYPE");
      return -1;
    }
    if (ses->fd < 0) {
      strcpyerr(ses, "INVALID_SESSION");
      return -1;
    }
    if (ses->fwd_fd >= 0) {
      strcpyerr(ses, "DUPLICATE_FORWARD");
      return -1;
    }
    if ((ses->fwd_fd = sam3HandshakeIP(ses->ip, ses->port)) < 0) {
      strcpyerr(ses, "IO_ERROR_SK");
      goto error;
    }
    if (sam3tcpPrintf(ses->fwd_fd,
                      "STREAM FORWARD ID=%s PORT=%d HOST=%s SILENT=%s\n",
                      ses->channel, port, hostname, checkIsSilent(ses)) < 0) {
      strcpyerr(ses, "IO_ERROR_PF");
      goto error;
    }
    if ((rep = sam3ReadReply(ses->fwd_fd)) == NULL) {
      strcpyerr(ses, "IO_ERROR_RP");
      goto error;
    }
    if (!sam3IsGoodReply(rep, "STREAM", "STATUS", "RESULT", "OK")) {
      const char *v = sam3FindField(rep, "RESULT");
      //
      strcpyerr(ses, (v != NULL && v[0] ? v : "I2P_ERROR_RES"));
      goto error;
    }
    sam3FreeFieldList(rep);
    strcpyerr(ses, NULL);
    return 0;
  error:
    if (rep != NULL)
      sam3FreeFieldList(rep);
    return -1;
  }
  return -1;
}

int sam3DatagramSend(Sam3Session *ses, const char *destkey, const void *buf,
                     size_t bufsize) {
  if (ses != NULL) {
    char *dbuf;
    int res, dbufsz;
    //
    if (ses->type == SAM3_SESSION_STREAM) {
      strcpyerr(ses, "INVALID_SESSION_TYPE");
      return -1;
    }
    if (ses->fd < 0) {
      strcpyerr(ses, "INVALID_SESSION");
      return -1;
    }
    if (destkey == NULL || !sam3CheckValidKeyLength(destkey)) {
      strcpyerr(ses, "INVALID_KEY");
      return -1;
    }
    if (buf == NULL || bufsize < 1 || bufsize > 31744) {
      strcpyerr(ses, "INVALID_DATA");
      return -1;
    }
    dbufsz = bufsize + 4 + SAM3_PUBKEY_SIZE + 1 + strlen(ses->channel) + 1;
    if ((dbuf = malloc(dbufsz)) == NULL) {
      strcpyerr(ses, "OUT_OF_MEMORY");
      return -1;
    }
    sprintf(dbuf, "3.0 %s %s\n", ses->channel, destkey);
    memcpy(dbuf + strlen(dbuf), buf, bufsize);
    res = sam3udpSendToIP(ses->ip, ses->port, dbuf, dbufsz);
    free(dbuf);
    strcpyerr(ses, (res < 0 ? "IO_ERROR" : NULL));
    return (res < 0 ? -1 : 0);
  }
  return -1;
}

ssize_t sam3DatagramReceive(Sam3Session *ses, void *buf, size_t bufsize) {
  if (ses != NULL) {
    SAMFieldList *rep;
    const char *v;
    ssize_t size = 0;
    //
    if (ses->type == SAM3_SESSION_STREAM) {
      strcpyerr(ses, "INVALID_SESSION_TYPE");
      return -1;
    }
    if (ses->fd < 0) {
      strcpyerr(ses, "INVALID_SESSION");
      return -1;
    }
    if (buf == NULL || bufsize < 1) {
      strcpyerr(ses, "INVALID_BUFFER");
      return -1;
    }
    if ((rep = sam3ReadReply(ses->fd)) == NULL) {
      strcpyerr(ses, "IO_ERROR");
      return -1;
    }
    if (!sam3IsGoodReply(rep, "DATAGRAM", "RECEIVED", "SIZE", NULL)) {
      strcpyerr(ses, "I2P_ERROR");
      sam3FreeFieldList(rep);
      return -1;
    }
    //
    if ((v = sam3FindField(rep, "DESTINATION")) != NULL &&
        sam3CheckValidKeyLength(v))
      strncpy(ses->destkey, v, sizeof(ses->destkey));
    v = sam3FindField(rep, "SIZE"); // we have this field -- for sure
    if (!v[0] || !isdigit(*v)) {
      strcpyerr(ses, "I2P_ERROR_SIZE");
      sam3FreeFieldList(rep);
      return -1;
    }
    //
    while (*v && isdigit(*v)) {
      if ((size = size * 10 + v[0] - '0') > bufsize) {
        strcpyerr(ses, "I2P_ERROR_BUFFER_TOO_SMALL");
        sam3FreeFieldList(rep);
        return -1;
      }
      ++v;
    }
    //
    if (*v) {
      strcpyerr(ses, "I2P_ERROR_SIZE");
      sam3FreeFieldList(rep);
      return -1;
    }
    sam3FreeFieldList(rep);
    //
    if (sam3tcpReceive(ses->fd, buf, size) != size) {
      strcpyerr(ses, "IO_ERROR");
      return -1;
    }
    strcpyerr(ses, NULL);
    return size;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
// output 8 bytes for every 5 input
// return size or <0 on error
ssize_t sam3Base32Encode(char *dest, size_t destsz, const void *srcbuf,
                         size_t srcsize) {
  if (dest != NULL && srcbuf != NULL && srcsize >= 0) {
    static const char *const b32chars = "abcdefghijklmnopqrstuvwxyz234567=";
    const unsigned char *src = (const unsigned char *)srcbuf;
    ssize_t destsize = 0;
    //
    while (srcsize > 0) {
      int blksize = (srcsize < 5 ? srcsize : 5);
      unsigned char n[8];
      //
      memset(n, 0, sizeof(n));
      switch (blksize) {
      case 5:
        n[7] = (src[4] & 0x1f);
        n[6] = ((src[4] & 0xe0) >> 5);
      case 4:
        n[6] |= ((src[3] & 0x03) << 3);
        n[5] = ((src[3] & 0x7c) >> 2);
        n[4] = ((src[3] & 0x80) >> 7);
      case 3:
        n[4] |= ((src[2] & 0x0f) << 1);
        n[3] = ((src[2] & 0xf0) >> 4);
      case 2:
        n[3] |= ((src[1] & 0x01) << 4);
        n[2] = ((src[1] & 0x3e) >> 1);
        n[1] = ((src[1] & 0xc0) >> 6);
      case 1:
        n[1] |= ((src[0] & 0x07) << 2);
        n[0] = ((src[0] & 0xf8) >> 3);
        break;
      }
      src += blksize;
      srcsize -= blksize;
      // pad
      switch (blksize) {
      case 1:
        n[2] = n[3] = 32;
      case 2:
        n[4] = 32;
      case 3:
        n[5] = n[6] = 32;
      case 4:
        n[7] = 32;
      case 5:
        break;
      }
      // output
      if (destsize + 8 <= destsz) {
        for (int f = 0; f < 8; ++f)
          *dest++ = b32chars[n[f]];
      }
      destsize += 8;
    }
    if (destsize <= destsz) {
      *dest++ = 0; // make valid asciiz string
    }
    return destsize;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
// output 8 bytes for every 5 input
// return size or <0 on error
ssize_t sam3Base32Decode(char *dest, size_t destsz, const void *srcbuf,
                         size_t srcsize) {
  if (dest != NULL && srcbuf != NULL && srcsize >= 0) {
    static const char *const b32chars = "abcdefghijklmnopqrstuvwxyz234567=";
    const unsigned char *src = (const unsigned char *)srcbuf;
    int destsize = 0;
    //
    while (srcsize > 0) {
      int blksize = (srcsize < 5 ? srcsize : 5);
      unsigned char n[8];
      //
      memset(n, 0, sizeof(n));
      switch (blksize) {
      case 5:
        n[7] = (src[4] & 0x1f);
        n[6] = ((src[4] & 0xe0) >> 5);
      case 4:
        n[6] |= ((src[3] & 0x03) << 3);
        n[5] = ((src[3] & 0x7c) >> 2);
        n[4] = ((src[3] & 0x80) >> 7);
      case 3:
        n[4] |= ((src[2] & 0x0f) << 1);
        n[3] = ((src[2] & 0xf0) >> 4);
      case 2:
        n[3] |= ((src[1] & 0x01) << 4);
        n[2] = ((src[1] & 0x3e) >> 1);
        n[1] = ((src[1] & 0xc0) >> 6);
      case 1:
        n[1] |= ((src[0] & 0x07) << 2);
        n[0] = ((src[0] & 0xf8) >> 3);
        break;
      }
      src += blksize;
      srcsize -= blksize;
      // pad
      switch (blksize) {
      case 1:
        n[2] = n[3] = 32;
      case 2:
        n[4] = 32;
      case 3:
        n[5] = n[6] = 32;
      case 4:
        n[7] = 32;
      case 5:
        break;
      }
      // output
      if (destsize + 8 <= destsz) {
        for (int f = 0; f < 8; ++f)
          *dest++ = b32chars[n[f]];
      }
      destsize += 8;
    }
    if (destsize <= destsz) {
      *dest++ = 0; // make valid asciiz string
    }
    return destsize;
  }
  return -1;
}

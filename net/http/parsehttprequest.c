/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2020 Justine Alexandra Roberts Tunney                              │
│                                                                              │
│ Permission to use, copy, modify, and/or distribute this software for         │
│ any purpose with or without fee is hereby granted, provided that the         │
│ above copyright notice and this permission notice appear in all copies.      │
│                                                                              │
│ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
│ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
│ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
│ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
│ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
│ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
│ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
│ PERFORMANCE OF THIS SOFTWARE.                                                │
╚─────────────────────────────────────────────────────────────────────────────*/
#include "libc/alg/alg.h"
#include "libc/alg/arraylist.internal.h"
#include "libc/limits.h"
#include "libc/macros.internal.h"
#include "libc/stdio/stdio.h"
#include "libc/str/str.h"
#include "libc/sysv/errfuns.h"
#include "libc/x/x.h"
#include "net/http/http.h"

#define LIMIT (SHRT_MAX - 1)

enum { START, METHOD, URI, VERSION, HKEY, HSEP, HVAL, CR1, LF1, LF2 };

/**
 * Initializes HTTP request parser.
 */
void InitHttpRequest(struct HttpRequest *r) {
  memset(r, 0, sizeof(*r));
}

/**
 * Parses HTTP request.
 */
int ParseHttpRequest(struct HttpRequest *r, const char *p, size_t n) {
  int c;
  for (n = MIN(n, LIMIT); r->i < n; ++r->i) {
    c = p[r->i] & 0xff;
    switch (r->t) {
      case START:
        if (c == '\r' || c == '\n') {
          ++r->a; /* RFC7230 § 3.5 */
          break;
        }
        r->t = METHOD;
        /* fallthrough */
      case METHOD:
        if (c == ' ') {
          if ((r->method = GetHttpMethod(p + r->a, r->i - r->a)) != -1) {
            r->uri.a = r->i + 1;
            r->t = URI;
          } else {
            return ebadmsg();
          }
        }
        break;
      case URI:
        if (c == ' ' || c == '\r' || c == '\n') {
          if (r->i == r->uri.a) return ebadmsg();
          r->uri.b = r->i;
          if (c == ' ') {
            r->version.a = r->i + 1;
            r->t = VERSION;
          } else if (c == '\r') {
            r->t = CR1;
          } else {
            r->t = LF1;
          }
        }
        break;
      case VERSION:
        if (c == '\r' || c == '\n') {
          r->version.b = r->i;
          r->t = c == '\r' ? CR1 : LF1;
        }
        break;
      case CR1:
        if (c != '\n') return ebadmsg();
        r->t = LF1;
        break;
      case LF1:
        if (c == '\r') {
          r->t = LF2;
          break;
        } else if (c == '\n') {
          return ++r->i;
        } else if (c == ':') {
          return ebadmsg();
        } else if (c == ' ' || c == '\t') {
          return ebadmsg(); /* RFC7230 § 3.2.4 */
        }
        r->a = r->i;
        r->t = HKEY;
        break;
      case HKEY:
        if (c == ':') {
          r->h = GetHttpHeader(p + r->a, r->i - r->a);
          r->t = HSEP;
        }
        break;
      case HSEP:
        if (c == ' ' || c == '\t') break;
        r->a = r->i;
        r->t = HVAL;
        /* fallthrough */
      case HVAL:
        if (c == '\r' || c == '\n') {
          if (r->h != -1) {
            r->headers[r->h].a = r->a;
            r->headers[r->h].b = r->i;
          }
          r->t = c == '\r' ? CR1 : LF1;
        }
        break;
      case LF2:
        if (c == '\n') {
          return ++r->i;
        }
        return ebadmsg();
      default:
        unreachable;
    }
  }
  if (r->i < LIMIT) {
    return 0;
  } else {
    return ebadmsg();
  }
}

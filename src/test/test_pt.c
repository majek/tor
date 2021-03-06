/* Copyright (c) 2001-2004, Roger Dingledine.
 * Copyright (c) 2004-2006, Roger Dingledine, Nick Mathewson.
 * Copyright (c) 2007-2013, The Tor Project, Inc. */
/* See LICENSE for licensing information */

#include "orconfig.h"
#define PT_PRIVATE
#include "or.h"
#include "transports.h"
#include "circuitbuild.h"
#include "test.h"

static void
reset_mp(managed_proxy_t *mp)
{
  mp->conf_state = PT_PROTO_LAUNCHED;
  SMARTLIST_FOREACH(mp->transports, transport_t *, t, transport_free(t));
  smartlist_clear(mp->transports);
}

static void
test_pt_parsing(void)
{
  char line[200];

  managed_proxy_t *mp = tor_malloc(sizeof(managed_proxy_t));
  mp->conf_state = PT_PROTO_INFANT;
  mp->transports = smartlist_new();

  /* incomplete cmethod */
  strlcpy(line,"CMETHOD trebuchet",sizeof(line));
  test_assert(parse_cmethod_line(line, mp) < 0);

  reset_mp(mp);

  /* wrong proxy type */
  strlcpy(line,"CMETHOD trebuchet dog 127.0.0.1:1999",sizeof(line));
  test_assert(parse_cmethod_line(line, mp) < 0);

  reset_mp(mp);

  /* wrong addrport */
  strlcpy(line,"CMETHOD trebuchet socks4 abcd",sizeof(line));
  test_assert(parse_cmethod_line(line, mp) < 0);

  reset_mp(mp);

  /* correct line */
  strlcpy(line,"CMETHOD trebuchet socks5 127.0.0.1:1999",sizeof(line));
  test_assert(parse_cmethod_line(line, mp) == 0);
  test_assert(smartlist_len(mp->transports));

  reset_mp(mp);

  /* incomplete smethod */
  strlcpy(line,"SMETHOD trebuchet",sizeof(line));
  test_assert(parse_smethod_line(line, mp) < 0);

  reset_mp(mp);

  /* wrong addr type */
  strlcpy(line,"SMETHOD trebuchet abcd",sizeof(line));
  test_assert(parse_smethod_line(line, mp) < 0);

  reset_mp(mp);

  /* cowwect */
  strlcpy(line,"SMETHOD trebuchy 127.0.0.1:1999",sizeof(line));
  test_assert(parse_smethod_line(line, mp) == 0);

  reset_mp(mp);

  /* unsupported version */
  strlcpy(line,"VERSION 666",sizeof(line));
  test_assert(parse_version(line, mp) < 0);

  /* incomplete VERSION */
  strlcpy(line,"VERSION ",sizeof(line));
  test_assert(parse_version(line, mp) < 0);

  /* correct VERSION */
  strlcpy(line,"VERSION 1",sizeof(line));
  test_assert(parse_version(line, mp) == 0);

 done:
  tor_free(mp);
}

static void
test_pt_protocol(void)
{
  char line[200];

  managed_proxy_t *mp = tor_malloc_zero(sizeof(managed_proxy_t));
  mp->conf_state = PT_PROTO_LAUNCHED;
  mp->transports = smartlist_new();
  mp->argv = tor_malloc_zero(sizeof(char*)*2);
  mp->argv[0] = tor_strdup("<testcase>");

  /* various wrong protocol runs: */

  strlcpy(line,"VERSION 1",sizeof(line));
  handle_proxy_line(line, mp);
  test_assert(mp->conf_state == PT_PROTO_ACCEPTING_METHODS);

  strlcpy(line,"VERSION 1",sizeof(line));
  handle_proxy_line(line, mp);
  test_assert(mp->conf_state == PT_PROTO_BROKEN);

  reset_mp(mp);

  strlcpy(line,"CMETHOD trebuchet socks5 127.0.0.1:1999",sizeof(line));
  handle_proxy_line(line, mp);
  test_assert(mp->conf_state == PT_PROTO_BROKEN);

  reset_mp(mp);

  /* correct protocol run: */
  strlcpy(line,"VERSION 1",sizeof(line));
  handle_proxy_line(line, mp);
  test_assert(mp->conf_state == PT_PROTO_ACCEPTING_METHODS);

  strlcpy(line,"CMETHOD trebuchet socks5 127.0.0.1:1999",sizeof(line));
  handle_proxy_line(line, mp);
  test_assert(mp->conf_state == PT_PROTO_ACCEPTING_METHODS);

  strlcpy(line,"CMETHODS DONE",sizeof(line));
  handle_proxy_line(line, mp);
  test_assert(mp->conf_state == PT_PROTO_CONFIGURED);

 done:
  tor_free(mp);
}

#define PT_LEGACY(name)                                               \
  { #name, legacy_test_helper, 0, &legacy_setup, test_pt_ ## name }

struct testcase_t pt_tests[] = {
  PT_LEGACY(parsing),
  PT_LEGACY(protocol),
  END_OF_TESTCASES
};


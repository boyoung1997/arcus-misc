/*
 * acp-c : Arcus C Client Performance benchmark program
 * Copyright 2013-2014 NAVER Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <netinet/in.h>
#include <assert.h>

#include "libmemcached/memcached.h"
#include "common.h"
#include "keyset.h"
#include "valueset.h"
#include "client_profile.h"
#include "client.h"

static int
do_btree_test(struct client *cli)
{
  memcached_coll_create_attrs_st attr;
  memcached_return rc;
  int ok, keylen, base;
  const char *key;
  uint8_t *val_ptr;
  int val_len;
  uint64_t bkey;

  // Pick a key
  key = keyset_get_key(cli->ks, &base);
  keylen = strlen(key);
  
  // Create a btree item
  if (0 != client_before_request(cli))
    return -1;
  
  memcached_coll_create_attrs_init(&attr, 20 /* flags */, 100 /* exptime */,
    4000 /* maxcount */);
  memcached_coll_create_attrs_set_overflowaction(&attr,
    OVERFLOWACTION_SMALLEST_TRIM);
  rc = memcached_bop_create(cli->next_mc, key, keylen, &attr);
  ok = (rc == MEMCACHED_SUCCESS);
  if (!ok) {
    print_log("bop create failed. id=%d key=%s rc=%d(%s)", cli->id, key,
      rc, memcached_strerror(NULL, rc));
  }
  if (0 != client_after_request(cli, ok))
    return -1;
  
  // Insert 4000 elements
  for (bkey = base; bkey < base + 4000; bkey++) {
    if (0 != client_before_request(cli))
      return -1;

    val_ptr = NULL;
    val_len = valueset_get_value(cli->vs, &val_ptr);
    assert(val_ptr != NULL && val_len > 0 && val_len <= 4096);
    
    rc = memcached_bop_insert(cli->next_mc, key, keylen,
      bkey,
      NULL /* eflag */, 0 /* eflag length */,
      (const char*)val_ptr, (size_t)val_len,
      NULL /* Do not create automatically */);
    valueset_return_value(cli->vs, val_ptr);
    ok = (rc == MEMCACHED_SUCCESS);
    if (!ok) {
      print_log("bop insert failed. id=%d key=%s bkey=%llu rc=%d(%s)",
        cli->id, key, (long long unsigned)bkey,
        rc, memcached_strerror(NULL, rc));
    }
    if (0 != client_after_request(cli, ok))
      return -1;
  }

  // Update elements
  for (bkey = base; bkey < base + 4000; bkey++) {
    if (0 != client_before_request(cli))
      return -1;

    val_ptr = NULL;
    val_len = valueset_get_value(cli->vs, &val_ptr);
    assert(val_ptr != NULL && val_len > 0 && val_len <= 4096);
    
    rc = memcached_bop_update(cli->next_mc, key, keylen,
      bkey,
      NULL /* eflag */,
      (const char*)val_ptr, (size_t)val_len);
    valueset_return_value(cli->vs, val_ptr);
    ok = (rc == MEMCACHED_SUCCESS);
    if (!ok) {
      print_log("bop update failed. id=%d key=%s bkey=%llu rc=%d(%s)",
        cli->id, key, (long long unsigned)bkey,
        rc, memcached_strerror(NULL, rc));
    }
    if (0 != client_after_request(cli, ok))
      return -1;
  }

  // Upsert elements
  for (bkey = base; bkey < base + 4000; bkey++) {
    if (0 != client_before_request(cli))
      return -1;

    val_ptr = NULL;
    val_len = valueset_get_value(cli->vs, &val_ptr);
    assert(val_ptr != NULL && val_len > 0 && val_len <= 4096);

    rc = memcached_bop_upsert(cli->next_mc, key, keylen,
      bkey,
      NULL /* eflag */, 0 /* eflag length */,
      (const char*)val_ptr, (size_t)val_len,
      NULL /* Do not create automatically */);
    valueset_return_value(cli->vs, val_ptr);
    ok = (rc == MEMCACHED_SUCCESS);
    if (!ok) {
      print_log("bop upsert failed. id=%d key=%s bkey=%llu"
        " val_ptr=%p val_len=%d rc=%d(%s)",
        cli->id, key, (long long unsigned)bkey,
        val_ptr, val_len,
        rc, memcached_strerror(NULL, rc));
    }
    if (0 != client_after_request(cli, ok))
      return -1;
  }
  
  return 0;
}

static int
do_test(struct client *cli)
{
  if (0 != do_btree_test(cli))
    return -1; // Stop the test
  
  return 0; // Do another test
}

static struct client_profile default_profile = {
  .do_test = do_test,
};

struct client_profile *
torture_btree_replace_init(void)
{
  return &default_profile;
}

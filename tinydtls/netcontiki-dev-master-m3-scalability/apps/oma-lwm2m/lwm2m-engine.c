/*
 * Copyright (c) 2015, SICS Swedish ICT AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holders nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    This product includes software developed by Yanzi Networks AB.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author: Joakim Eriksson, joakime@sics.se
 *         Niclas Finne, nfi@sics.se
 *
 */

#include "lwm2m-engine.h"
#include "lwm2m-object.h"
#include "lwm2m-plain-text.h"
#include "rest-engine.h"
#include "er-coap-constants.h"
#include "er-coap-engine.h"
#include "er-coap-context.h"
#include "contiki.h"
#include "contiki-net.h"
#include "oma-tlv.h"
#include <stdio.h>
#include <string.h>
#include "tlv-writer.h"
#include "net/ipv6/uip-ds6.h"
#include "net/rpl/rpl.h"

#ifdef WITH_DTLS
#include "tinydtls.h"
#include "dtls.h"
#endif

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#ifndef PRODUCT_STRING
#ifdef BOARD_STRING
#define PRODUCT_STRING BOARD_STRING
#else /* BOARD_STRING */
#define PRODUCT_STRING "Contiki"
#endif /* BOARD_STRING */
#endif /* PRODUCT_STRING */

#define REMOTE_PORT        UIP_HTONS(COAP_DEFAULT_PORT)
#define BS_REMOTE_PORT     UIP_HTONS(5685)

#define MAX_OBJECTS 10

static const lwm2m_object_t *objects[MAX_OBJECTS];
static char endpoint[32];
static char rd_data[128]; /* allocate some data for the RD */

PROCESS(lwm2m_rd_client, "LWM2M Engine");

static uip_ipaddr_t server_ipaddr;
static uint16_t server_port;
static uip_ipaddr_t bs_server_ipaddr;

static uint8_t registered = 0;
static uint8_t bootstrapped = 0; /* bootstrap made... */

static const lwm2m_instance_t *get_first_instance_of_object(uint16_t id, lwm2m_context_t *context);
static const lwm2m_instance_t *get_instance(const lwm2m_object_t *object, lwm2m_context_t *context, int depth);
static const lwm2m_resource_t *get_resource(const lwm2m_instance_t *instance, lwm2m_context_t *context);
/*---------------------------------------------------------------------------*/
#ifdef WITH_DTLS

static uint8_t is_secure = 0;
static int32_t security_mode = LWM2M_SECURITY_MODE_NOSEC; /* No security by default */
static coap_context_t *coap_context;

/* Pre-shared key mode */
static int
get_psk_info(struct dtls_context_t *ctx, const session_t *session,
	     dtls_credentials_type_t type,
	     const unsigned char *id, size_t id_len,
	     unsigned char *result, size_t result_length)
{
  lwm2m_context_t context;
  const uint8_t *sid, *key;
  int sid_len, key_len;
  const lwm2m_instance_t *instance;
  const lwm2m_resource_t *rsc;

  if(security_mode != LWM2M_SECURITY_MODE_PSK) {
    /* Wrong mode */
    return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
  }

  instance = get_first_instance_of_object(LWM2M_OBJECT_SECURITY_ID, &context);
  if(instance == NULL) {
    /* No security object instance found */
    PRINTF("lwm2m.psk: No security instance found\n");
    return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
  }

  context.resource_id = LWM2M_SECURITY_CLIENT_PKI;
  rsc = get_resource(instance, &context);
  sid = lwm2m_object_get_resource_string(rsc, &context);
  sid_len = lwm2m_object_get_resource_strlen(rsc, &context);
  if(sid == NULL || sid_len == 0) {
    PRINTF("lwm2m.psk: no client pki found\n");
    return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
  }

  switch(type) {
  case DTLS_PSK_IDENTITY:
    PRINTF("lwm2m.psk: Found PSK identity: %.*s\n", sid_len, (char *)sid);
    if(result_length < sid_len) {
      PRINTF("lwm2m.psk: cannot set psk_identity -- buffer too small\n");
      return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
    }
    memcpy(result, sid, sid_len);
    return sid_len;
  case DTLS_PSK_KEY:
    if(id_len != sid_len || memcmp(sid, id, id_len) != 0) {
      PRINTF("lwm2m.psk: PSK for unknown id requested\n");
      return dtls_alert_fatal_create(DTLS_ALERT_ILLEGAL_PARAMETER);
    }

    context.resource_id = LWM2M_SECURITY_KEY;
    rsc = get_resource(instance, &context);
    key = lwm2m_object_get_resource_string(rsc, &context);
    key_len = lwm2m_object_get_resource_strlen(rsc, &context);
    if(key == NULL || key_len == 0) {
      PRINTF("lwm2m.psk: No security key found\n");
      return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
    }

    if(result_length < key_len) {
      PRINTF("lwm2m.psk: cannot set psk -- buffer too small\n");
      return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
    }

    memcpy(result, key, key_len);
    return key_len;
  default:
    PRINTF("lwm2m.psk: unsupported request type: %d\n", type);
  }

  return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
}

#ifdef DTLS_ECC

static int
get_ecdsa_key(struct dtls_context_t *ctx,
	      const session_t *session,
	      const dtls_ecdsa_key_t **result)
{
  /* To reduce memory usage, the keys are only cached until the next call to
     get_ecdsa_key or until the LWM2M security object information is changed.
     TinyDTLS only uses the information directly after each call to
     get_ecdsa_key and do not save the pointer.
  */
  static dtls_ecdsa_key_t ecdsa_key;

  lwm2m_context_t context;
  const uint8_t *sid, *key;
  int sid_len, key_len;
  const lwm2m_instance_t *instance;
  const lwm2m_resource_t *rsc;

  if(security_mode != LWM2M_SECURITY_MODE_RPK) {
    /* Wrong mode */
    return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
  }

  instance = get_first_instance_of_object(LWM2M_OBJECT_SECURITY_ID, &context);
  if(instance == NULL) {
    /* No security object instance found */
    PRINTF("lwm2m.rpk: No security instance found\n");
    return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
  }

  context.resource_id = LWM2M_SECURITY_CLIENT_PKI;
  rsc = get_resource(instance, &context);
  sid = lwm2m_object_get_resource_string(rsc, &context);
  sid_len = lwm2m_object_get_resource_strlen(rsc, &context);
  if(sid == NULL || sid_len != DTLS_EC_KEY_SIZE * 2) {
    PRINTF("lwm2m.rpk: no client pki found (len=%d)\n", sid_len);
    return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
  }

  context.resource_id = LWM2M_SECURITY_KEY;
  rsc = get_resource(instance, &context);
  key = lwm2m_object_get_resource_string(rsc, &context);
  key_len = lwm2m_object_get_resource_strlen(rsc, &context);
  if(key == NULL || key_len != DTLS_EC_KEY_SIZE) {
    PRINTF("lwm2m.rpk: No security key found (len=%d)\n", key_len);
    return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
  }

  memset(&ecdsa_key, 0, sizeof(ecdsa_key));
  ecdsa_key.curve = DTLS_ECDH_CURVE_SECP256R1;
  ecdsa_key.priv_key = key;
  ecdsa_key.pub_key_x = &sid[0];
  ecdsa_key.pub_key_y = &sid[DTLS_EC_KEY_SIZE];

  *result = &ecdsa_key;
  return 0;
}

static int
verify_ecdsa_key(struct dtls_context_t *ctx,
		 const session_t *session,
		 const unsigned char *other_pub_x,
		 const unsigned char *other_pub_y,
		 size_t key_size)
{
  return 0;
}
#endif /* DTLS_ECC */

#endif  /* WITH_DTLS */
/*---------------------------------------------------------------------------*/
static void
client_chunk_handler(void *response)
{
  const uint8_t *chunk;

  int len = coap_get_payload(response, &chunk);

  printf("|%.*s\n", len, (char *)chunk);
}
/*---------------------------------------------------------------------------*/
static int
index_of(const uint8_t *data, int offset, int len, uint8_t c)
{
  if(offset < 0) {
    return offset;
  }
  for(;offset < len;offset++) {
    if(data[offset] == c) {
      return offset;
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(lwm2m_rd_client, ev, data)
{
  static coap_packet_t request[1];      /* This way the packet can be treated as pointer as usual. */
  static struct etimer et;

  PROCESS_BEGIN();

  /* hardcoded BS server for now */
  uip_ip6addr(&bs_server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0x1);

#if WITH_DTLS
  coap_context = coap_context_new(uip_htons(COAP_DEFAULT_SECURE_PORT));
  if(coap_context == NULL) {
    printf("lwm2m: failed to setup DTLS\n");
  }
#endif /* WITH_DTLS */

  printf("RD Client started with endpoint '%s'\n", endpoint);

  etimer_set(&et, 15 * CLOCK_SECOND);

  while(1) {
    PROCESS_YIELD();

    if(etimer_expired(&et)) {
      if(UIP_CONF_IPV6_RPL && rpl_get_any_dag() == NULL) {
        /* Wait until for a network to join */

      } else if(bootstrapped == 0) {
	/* prepare request, TID is set by COAP_BLOCKING_REQUEST() */
	coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
	coap_set_header_uri_path(request, "/bs");
	coap_set_header_uri_query(request, endpoint);

	printf("Registering ID with bootstrap server [");
        uip_debug_ipaddr_print(&bs_server_ipaddr);
        printf("]:%u as '%s'\n", uip_ntohs(BS_REMOTE_PORT), endpoint);

	COAP_BLOCKING_REQUEST(&bs_server_ipaddr, BS_REMOTE_PORT, request,
			      client_chunk_handler);
	bootstrapped++;
      } else if(bootstrapped == 1) {
	lwm2m_context_t context;
	const lwm2m_instance_t *instance = NULL;
        const lwm2m_resource_t *rsc;
        const uint8_t *first;
        int len;

	printf("*** Bootstrap - checking for server info...\n");

	/* get the security object */
        instance = get_first_instance_of_object(LWM2M_OBJECT_SECURITY_ID, &context);
	if(instance != NULL) {
          /* get the server URI */
	  context.resource_id = LWM2M_SECURITY_SERVER_URI;
	  rsc = get_resource(instance, &context);
          first = lwm2m_object_get_resource_string(rsc, &context);
	  len = lwm2m_object_get_resource_strlen(rsc, &context);
          if(first != NULL && len > 0) {
            int start, end;
            uip_ipaddr_t addr;
            int32_t port;
            uint8_t secure = 0;

            printf("**** Found security instance using: %.*s\n", len, first);
            /* TODO Should verify it is a URI */

            /* Check if secure */
            secure = strncmp((const char *)first, "coaps:", 6) == 0;

            /* Only IPv6 supported */
            start = index_of(first, 0, len, '[');
            end = index_of(first, start, len, ']');
            if(start > 0 && end > start &&
               uiplib_ipaddrconv((const char *)&first[start], &addr)) {
              if(first[end + 1] == ':' &&
                 lwm2m_plain_text_read_int(first + end + 2, len - end - 2, &port)) {
              } else if(secure) {
                port = COAP_DEFAULT_SECURE_PORT;
              } else {
                port = COAP_DEFAULT_PORT;
              }
              PRINTF("Server address ");
              PRINT6ADDR(&addr);
              PRINTF(" port %ld%s\n", (long)port, secure ? " (secure)" : "");
              uip_ipaddr_copy(&server_ipaddr, &addr);
              server_port = UIP_HTONS((uint16_t) port);

#if WITH_DTLS
              is_secure = secure;
              security_mode = LWM2M_SECURITY_MODE_NOSEC;
              if(!secure) {
                /* No security */
                bootstrapped++;
              } else {
                /* Get the security mode */
                context.resource_id = LWM2M_SECURITY_MODE;
                rsc = get_resource(instance, &context);
                if(rsc == NULL || !lwm2m_object_get_resource_int(rsc, &context, &security_mode)) {
                  PRINTF("lwm2m: No security mode found!\n");
                  security_mode = LWM2M_SECURITY_MODE_NOSEC;
                } else {
                  PRINTF("lwm2m: Security mode: %d\n", (int)security_mode);
                }

                if(coap_context == NULL) {
                  /* No DTLS context available */
                  printf("Secure CoAP requested but no secure CoAP context available\n");
                } else if(security_mode == LWM2M_SECURITY_MODE_PSK) {
                  /* Pre-shared key mode */
                  coap_context->dtls_handler.get_psk_info = get_psk_info;
                  coap_context->dtls_handler.get_ecdsa_key = NULL;
                  coap_context->dtls_handler.verify_ecdsa_key = NULL;
                  if(coap_context_connect(coap_context, &server_ipaddr, server_port) == 0) {
                    printf("lwm2m: Failed to connect DTLS\n");
                  } else {
                    bootstrapped++;
                  }

#ifdef DTLS_ECC
                } else if(security_mode == LWM2M_SECURITY_MODE_RPK) {
                  /* Raw Public Key ECC */
                  coap_context->dtls_handler.get_psk_info = NULL;
                  coap_context->dtls_handler.get_ecdsa_key = get_ecdsa_key;
                  coap_context->dtls_handler.verify_ecdsa_key = verify_ecdsa_key;
                  if(coap_context_connect(coap_context, &server_ipaddr, server_port) == 0) {
                    printf("lwm2m: Failed to connect DTLS\n");
                  } else {
                    bootstrapped++;
                  }
#endif /* DTLS_ECC */

                } else if(security_mode == LWM2M_SECURITY_MODE_NOSEC) {
                  coap_context->dtls_handler.get_psk_info = NULL;
                  coap_context->dtls_handler.get_ecdsa_key = NULL;
                  coap_context->dtls_handler.verify_ecdsa_key = NULL;

                  bootstrapped++;
                } else {
                  printf("Unsupported security mode: %ld\n", (long)security_mode);
                }
              }
#else /* WITH_DTLS */
              if(secure) {
                printf("Secure CoAP requested but not supported - can not bootstrap\n");
              } else {
                bootstrapped++;
              }
#endif /* WITH_DTLS */

            } else {
              printf("** failed to parse URI %.*s\n", len, first);
            }
	  }
	}

        if(bootstrapped == 1) {
          /* Not ready. Lets retry with the bootstrap server again */
          bootstrapped = 0;
        }

#if WITH_DTLS
      } else if(is_secure && coap_context != NULL &&
                coap_context_is_connecting(coap_context)) {
        /* Waiting to connect */
      } else if(is_secure && coap_context != NULL &&
                coap_context_has_errors(coap_context)) {
        /* DTLS connection failed */
        printf("*** DTLS connection failed\n");

        /* Go back and ask the bootstrap server again! */
        bootstrapped = 0;

#endif /* WITH_DTLS */
      } else if(!registered) {
	int pos;
	int len, i, j;
	registered = 1;

	/* prepare request, TID is set by COAP_BLOCKING_REQUEST() */
	coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
	coap_set_header_uri_path(request, "/rd");
	coap_set_header_uri_query(request, endpoint);

	/* generate the rd data */
	pos = 0;
	for(i = 0; i < MAX_OBJECTS; i++) {
	  if(objects[i] != NULL) {
	    for(j = 0; j < objects[i]->count; j++) {
	      if(objects[i]->instances[j].flag & INSTANCE_FLAG_USED) {
                len = snprintf(&rd_data[pos], sizeof(rd_data) - pos,
                               "%s<%d/%d>", pos > 0 ? "," : "",
                               objects[i]->id, objects[i]->instances[j].id);
                if(len > 0 && len < sizeof(rd_data) - pos) {
                  pos += len;
                }
	      }
	    }
	  }
	}

	coap_set_payload(request, (uint8_t *)rd_data, pos);

        printf("Registering lwm2m endpoint '%s': '%.*s'\n", endpoint,
               pos, rd_data);
#if WITH_DTLS
        if(is_secure && coap_context != NULL) {
          COAP_CONTEXT_BLOCKING_REQUEST(coap_context,
                                        &server_ipaddr, server_port,
                                        request, client_chunk_handler);
        } else {
#endif /* WITH_DTLS */
          COAP_BLOCKING_REQUEST(&server_ipaddr, server_port, request,
                                client_chunk_handler);
#if WITH_DTLS
        }
#endif /* WITH_DTLS */
      }
      /* for now only register once...   registered = 0; */
      etimer_set(&et, 15 * CLOCK_SECOND);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
lwm2m_register_with(uip_ipaddr_t *server)
{
  uip_ipaddr_copy(&server_ipaddr, server);
  process_poll(&lwm2m_rd_client);
}
/*---------------------------------------------------------------------------*/
void
lwm2m_engine_init(void)
{
#ifdef LWM2M_ENGINE_CLIENT_ENDPOINT_NAME

  snprintf(endpoint, sizeof(endpoint), "?ep=" LWM2M_ENGINE_CLIENT_ENDPOINT_NAME);

#else /* LWM2M_ENGINE_CLIENT_ENDPOINT_NAME */

  int len, i;
  uint8_t state;
  uip_ipaddr_t *ipaddr;
  char client[sizeof(endpoint)];

  len = strlen(PRODUCT_STRING);
  /* ensure that this fits with the hex-nums */
  if(len > sizeof(client) - 13) {
    len = sizeof(client) - 13;
  }
  memcpy(client, PRODUCT_STRING, len);

  /* pick an IP address that is PREFERRED or TENTATIVE */
  ipaddr = NULL;
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      ipaddr = &(uip_ds6_if.addr_list[i]).ipaddr;
      break;
    }
  }

  if(ipaddr != NULL) {
    for(i = 0; i < 6; i++) {
      /* assume IPv6 for now */
      uint8_t b = ipaddr->u8[10 + i];
      client[len++] = (b >> 4) > 9 ? 'A' - 10 + (b >> 4) : '0' + (b >> 4);
      client[len++] = (b & 0xf) > 9 ? 'A' - 10 + (b & 0xf) : '0' + (b & 0xf);
    }
  }

  /* a zero at end of string */
  client[len] = 0;
  /* create endpoint */
  snprintf(endpoint, sizeof(endpoint), "?ep=%s", client);

#endif /* LWM2M_ENGINE_CLIENT_ENDPOINT_NAME */

  rest_init_engine();
  process_start(&lwm2m_rd_client, NULL);
}
/*---------------------------------------------------------------------------*/
static int
parse_next(const char **path, int *path_len, uint16_t *value)
{
  char c;
  *value = 0;
  /* printf("parse_next: %p %d\n", *path, *path_len); */
  if(*path_len == 0) {
    return 0;
  }
  while(*path_len > 0) {
    c = **path;
    (*path)++;
    *path_len = *path_len - 1;
    if(c >= '0' && c <= '9') {
      *value = *value * 10 + (c - '0');
    } else if(c == '/') {
      return 1;
    } else {
      /* error */
      return -4;
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
int
lwm2m_engine_parse_context(const lwm2m_object_t *object, const char *path, int path_len, lwm2m_context_t *context)
{
  int ret;
  if(context == NULL || object == NULL || path == NULL) {
    return 0;
  }
  memset(context, 0, sizeof(lwm2m_context_t));
  /* get object id */
  ret = 0;
  ret += parse_next(&path, &path_len, &context->object_id);
  ret += parse_next(&path, &path_len, &context->object_instance_id);
  ret += parse_next(&path, &path_len, &context->resource_id);

  /* Set default reader/writer */
  context->reader = &lwm2m_plain_text_reader;
  context->writer = &tlv_writer;

  return ret;
}
/*---------------------------------------------------------------------------*/
const lwm2m_object_t *
lwm2m_engine_get_object(uint16_t id)
{
  int i;
  for(i = 0; i < MAX_OBJECTS; i++) {
    if(objects[i] != NULL && objects[i]->id == id) {
      return objects[i];
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
int
lwm2m_engine_register_object(const lwm2m_object_t *object)
{
  int i;
  int found = 0;
  for(i = 0; i < MAX_OBJECTS; i++) {
    if(objects[i] == NULL) {
      objects[i] = object;
      found = 1;
      break;
    }
  }
  rest_activate_resource(lwm2m_object_get_coap_resource(object),
                         (char *)object->path);
  return found;
}
/*---------------------------------------------------------------------------*/
static const lwm2m_instance_t *
get_first_instance_of_object(uint16_t id, lwm2m_context_t *context)
{
  const lwm2m_object_t *object;
  int i;

  object = lwm2m_engine_get_object(id);
  if(object == NULL) {
    /* No object with the specified id found */
    return NULL;
  }

  /* Initialize the context */
  memset(context, 0, sizeof(lwm2m_context_t));
  context->object_id = id;

  for(i = 0; i < object->count; i++) {
    if(object->instances[i].flag & INSTANCE_FLAG_USED) {
      context->object_instance_id = object->instances[i].id;
      context->object_instance_index = i;
      return &object->instances[i];
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static const lwm2m_instance_t *
get_instance(const lwm2m_object_t *object, lwm2m_context_t *context, int depth)
{
  int i;
  if(depth > 1) {
    PRINTF("lwm2m: searching for instance %u\n", context->object_instance_id);
    for(i = 0; i < object->count; i++) {
      PRINTF("  Instance %d -> %u (used: %d)\n", i, object->instances[i].id,
	     (object->instances[i].flag & INSTANCE_FLAG_USED) != 0);
      if(object->instances[i].id == context->object_instance_id &&
	 object->instances[i].flag & INSTANCE_FLAG_USED) {
	context->object_instance_index = i;
	return &object->instances[i];
      }
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static const lwm2m_resource_t *
get_resource(const lwm2m_instance_t *instance, lwm2m_context_t *context) {
  if(instance != NULL) {
    int i;
    PRINTF("lwm2m: searching for resource %u\n", context->resource_id);
    for(i = 0; i < instance->count; i++) {
      PRINTF("  Resource %d -> %u\n", i, instance->resources[i].id);
      if(instance->resources[i].id == context->resource_id) {
	context->resource_index = i;
	return &instance->resources[i];
      }
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
lwm2m_engine_handler(const lwm2m_object_t *object, void *request,
		     void *response, uint8_t *buffer, uint16_t preferred_size,
		     int32_t *offset)
{
  int len;
  const char *url;
  unsigned int format;
  int depth;
  lwm2m_context_t context;
  rest_resource_flags_t method;
  char *method_str = "UNKNOWN";

  method = REST.get_method_type(request);
  /* for debugging */
  if(method == METHOD_GET) method_str = "GET";
  else if(method == METHOD_POST) method_str = "POST";
  else if(method == METHOD_PUT) method_str = "PUT";
  else if(method == METHOD_DELETE) method_str = "DELETE";

  len = REST.get_url(request, &url);
  if(!REST.get_header_content_type(request, &format)) {
    PRINTF("No format given. Assume text plain...\n");
    format = LWM2M_TEXT_PLAIN;
  } else if(format == TEXT_PLAIN) {
    /* CoAP content format text plain - assume LWM2M text plain */
    format = LWM2M_TEXT_PLAIN;
  }

  depth = lwm2m_engine_parse_context(object, url, len, &context);
  PRINTF("Context: %u/%u/%u  found: %d\n", context.object_id,
	 context.object_instance_id, context.resource_id, depth);

  printf("%s Called Path:%.*s Format:%d ID:%d bsize:%u\n", method_str, len, url, format, object->id, preferred_size);
  if(format == LWM2M_TEXT_PLAIN) {
    /* a string */
    const uint8_t *data;
    int plen = REST.get_request_payload(request, &data);
    printf("Data: '%.*s'\n", plen, data);
  }

  const lwm2m_instance_t *instance = get_instance(object, &context, depth);

  /* from POST */
  if(instance == NULL) {
    if(method != METHOD_PUT && method != METHOD_POST) {
      printf("Error - do not have instance %d\n", context.object_instance_id);
      REST.set_response_status(response, NOT_FOUND_4_04);
      return;
    } else {
      int i, len, pos;
      oma_tlv_t tlv;
      printf(">>> CREATE ? %d/%d\n",
	     context.object_id, context.object_instance_id);

      for(i = 0; i < object->count; i++) {
	if((object->instances[i].flag & INSTANCE_FLAG_USED) == 0) {
	  /* allocate this instance */
	  object->instances[i].flag |= INSTANCE_FLAG_USED;
	  object->instances[i].id = context.object_instance_id;
          context.object_instance_index = i;
	  printf("Created instance: %d\n", context.object_instance_id);
	  REST.set_response_status(response, CREATED_2_01);
	  instance = &object->instances[i];
	  break;
	}
      }

      if(instance == NULL) {
	/* could for some reason not create the instance */
	REST.set_response_status(response, BAD_REQUEST_4_00);
	return;
      }

      const uint8_t *data;
      int plen = REST.get_request_payload(request, &data);
      if(plen == 0) {
	/* do nothing more */
	return;
      }
      PRINTF("Payload: ");
      for(i = 0; i < plen; i++) {
	PRINTF("%02x", data[i]);
      }
      PRINTF("\n");

      pos = 0;
      do {
	len = oma_tlv_read(&tlv, (uint8_t *)&data[pos], plen - pos);
	PRINTF("Found TLV type=%u id=%u len=%lu\n",
	       tlv.type, tlv.id, (unsigned long)tlv.length);
	/* here we need to do callbacks or write value */
	if(tlv.type == OMA_TLV_TYPE_RESOURCE) {
	  context.resource_id = tlv.id;
	  const lwm2m_resource_t *rsc = get_resource(instance, &context);
	  if(rsc != NULL) {
	    /* write the value to the resource */
	    if(lwm2m_object_is_resource_string(rsc)) {
	      PRINTF("  new string value for /%d/%d/%d = %.*s\n",
		     context.object_id, context.object_instance_id,
		     context.resource_id, (int)tlv.length, tlv.value);
              lwm2m_object_set_resource_string(rsc, &context,
                                               tlv.length, tlv.value);
	    } else if(lwm2m_object_is_resource_int(rsc)) {
	      PRINTF("  new int value for /%d/%d/%d = %ld\n",
		     context.object_id, context.object_instance_id,
		     context.resource_id, (long)oma_tlv_get_int32(&tlv));
              lwm2m_object_set_resource_int(rsc, &context,
                                            oma_tlv_get_int32(&tlv));
	    } else if(lwm2m_object_is_resource_floatfix(rsc)) {
              /* TODO floatfix conversion */
	      PRINTF("  new float value for /%d/%d/%d = %ld\n",
		     context.object_id, context.object_instance_id,
		     context.resource_id, (long)oma_tlv_get_int32(&tlv));
              lwm2m_object_set_resource_floatfix(rsc, &context,
                                                 oma_tlv_get_int32(&tlv));
	    } else if(lwm2m_object_is_resource_boolean(rsc)) {
	      PRINTF("  new boolean value for /%d/%d/%d = %ld\n",
		     context.object_id, context.object_instance_id,
		     context.resource_id, (long)oma_tlv_get_int32(&tlv));
              lwm2m_object_set_resource_boolean(rsc, &context,
                                                oma_tlv_get_int32(&tlv) != 0);
	    }
	  }
	}
	pos = pos + len;
      } while(len > 0 && pos < plen);
    }
  }

  if(depth == 3) {
    const lwm2m_resource_t *resource = get_resource(instance, &context);
    size_t tlvlen = 0;
    if(resource == NULL) {
      printf("Error - do not have resource %d\n", context.resource_id);
      REST.set_response_status(response, NOT_FOUND_4_04);
      return;
    }
    /* HANDLE PUT */
    if(method == METHOD_PUT) {
      if(lwm2m_object_is_resource_callback(resource)) {
	if(resource->value.callback.write != NULL) {
	  /* pick a reader ??? */
	  if(format == LWM2M_TEXT_PLAIN) {
	    /* a string */
	    const uint8_t *data;
	    int plen = REST.get_request_payload(request, &data);
	    context.reader = &lwm2m_plain_text_reader;
	    PRINTF("PUT Callback with data: '%.*s'\n", plen, data);
	    /* no specific reader for plain text */
	    tlvlen = resource->value.callback.write(&context, data, plen,
						    buffer, preferred_size);
	    PRINTF("tlvlen:%u\n", (unsigned int) tlvlen);
	    REST.set_response_status(response, CHANGED_2_04);
	  } else {
            PRINTF("PUT callback with format %d\n", format);
            REST.set_response_status(response, INTERNAL_SERVER_ERROR_5_00);
          }
	} else {
          PRINTF("PUT - no write callback\n");
          REST.set_response_status(response, METHOD_NOT_ALLOWED_4_05);
        }
      } else {
        PRINTF("PUT on non-callback resource!\n");
        REST.set_response_status(response, INTERNAL_SERVER_ERROR_5_00);
      }
      /* HANDLE GET */
    } else if(method == METHOD_GET) {
      if(lwm2m_object_is_resource_string(resource)) {
        const uint8_t *value;
        uint16_t len;
        value = lwm2m_object_get_resource_string(resource, &context);
        len = lwm2m_object_get_resource_strlen(resource, &context);
        if(value != NULL) {
          PRINTF("Get string value: %.*s\n", (int)len, (char *)value);
          /* TODO check format */
          REST.set_response_payload(response, value, len);
          REST.set_header_content_type(response, LWM2M_TEXT_PLAIN);
          /* Done */
          return;
        }
      } else if(lwm2m_object_is_resource_int(resource)) {
        int32_t value;
        if(lwm2m_object_get_resource_int(resource, &context, &value)) {
          /* export INT as TLV */
          tlvlen = oma_tlv_write_int32(resource->id, value, buffer, preferred_size);
          PRINTF("Exporting int as TLV: %ld, len: %u\n", (long)value, (unsigned int) tlvlen);
        }
      } else if(lwm2m_object_is_resource_floatfix(resource)) {
        int32_t value;
        if(lwm2m_object_get_resource_floatfix(resource, &context, &value)) {
          /* export FLOATFIX 10 bits as TLV */
          PRINTF("Exporting 10-bit fix as float: %ld\n", (long)value);
          tlvlen = oma_tlv_write_float32(resource->id, value, 10, buffer, preferred_size);
          PRINTF("Exporting as TLV: len:%u\n", (unsigned int) tlvlen);
        }
      } else if(resource->type == LWM2M_RESOURCE_TYPE_CALLBACK) {
	if(resource->value.callback.read != NULL) {
	  tlvlen = resource->value.callback.read(&context,
						 buffer, preferred_size);
	}
      }
      if(tlvlen > 0) {
	REST.set_response_payload(response, buffer, tlvlen);
	REST.set_header_content_type(response, LWM2M_TLV);
      } else {
	/* failed to produce output - it is an internal error */
	REST.set_response_status(response, INTERNAL_SERVER_ERROR_5_00);
      }
      /* Handle POST */
    } else if(method == METHOD_POST) {
      if(resource->type == LWM2M_RESOURCE_TYPE_CALLBACK) {
	if(resource->value.callback.exec != NULL) {
	  const uint8_t *data;
	  int plen = REST.get_request_payload(request, &data);
	  PRINTF("Execute Callback with data: '%.*s'\n", plen, data);
	  tlvlen = resource->value.callback.exec(&context,
						 data, plen,
						 buffer, preferred_size);
	  REST.set_response_status(response, CHANGED_2_04);
	} else {
          printf("Execute callback - no exec callback\n");
	  REST.set_response_status(response, FORBIDDEN_4_03);
        }
      } else {
        printf("Resource post but no callback resource\n");
      }
    }
  } else if(depth == 2) {
    /* produce an instance response */
    if(method == METHOD_GET) {
      if(instance != NULL) {
	int i;
	char *s = "";
	const lwm2m_resource_t *resource = NULL;
	if(format == APPLICATION_LINK_FORMAT) {
	  printf("<%d/%d>", object->id, instance->id);
	} else {
	  printf("{\"e\":[\n");
	}
	for(i = 0; i < instance->count; i++) {
	  resource = &instance->resources[i];
	  if(format == APPLICATION_LINK_FORMAT) {
	    printf(",<%d/%d/%d>", object->id, instance->id, resource->id);
	  } else {
	    if(lwm2m_object_is_resource_string(resource)) {
              const uint8_t *value;
              uint16_t len;
              value = lwm2m_object_get_resource_string(resource, &context);
              len = lwm2m_object_get_resource_strlen(resource, &context);
              if(value != NULL) {
                printf("%s{\"n\":\"%u\",\"vs\":\"%.*s\"}\n", s, resource->id, len, value);
                s = ",";
              }
	    } else if(lwm2m_object_is_resource_int(resource)) {
              int32_t value;
              if(lwm2m_object_get_resource_int(resource, &context, &value)) {
                printf("%s{\"n\":\"%u\",\"v\":%ld}\n", s, resource->id, (long)value);
                s = ",";
              }
	    } else if(lwm2m_object_is_resource_floatfix(resource)) {
              int32_t value;
              if(lwm2m_object_get_resource_floatfix(resource, &context, &value)) {
                /* TODO floatfix conversion */
                printf("%s{\"n\":\"%u\",\"v\":%ld}\n", s, resource->id, (long)value);
                s = ",";
              }
	    } else if(lwm2m_object_is_resource_boolean(resource)) {
              int value;
              if(lwm2m_object_get_resource_boolean(resource, &context, &value)) {
                printf("%s{\"n\":\"%u\",\"v\":%s}\n", s, resource->id,
                       value ? "true" : "false");
                s = ",";
              }
	    }
	  }
	}
	if(format != APPLICATION_LINK_FORMAT) {
	  printf("]}\n");
	  REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
	} else {
	  REST.set_header_content_type(response, REST.type.APPLICATION_LINK_FORMAT);
	}
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
void
lwm2m_engine_delete_handler(const lwm2m_object_t *object, void *request,
			 void *response, uint8_t *buffer, uint16_t preferred_size,
			 int32_t *offset)
{
  int len, depth;
  const char *url;
  lwm2m_context_t context;

  len = REST.get_url(request, &url);
  printf("*** DELETE URI:'%.*s' called... - responding with DELETED.\n", len, url);
  depth = lwm2m_engine_parse_context(object, url, len, &context);
  printf("Context: %u/%u/%u  found: %d\n", context.object_id,
	 context.object_instance_id, context.resource_id, depth);

  REST.set_response_status(response, DELETED_2_02);
}
/*---------------------------------------------------------------------------*/

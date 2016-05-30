/*
 * Copyright (c) 2013 Joakim Eriksson, Niclas Finne
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "contiki.h"
#include "contiki-lib.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>
#include <string.h>

#include "contiki-net.h"

#include "border-router.h"
#include "border-router-cmds.h"
#include "br-config.h"
#include "vargen.h"
#include "encap.h"
#include "oam.h"
#include "tlv.h"
#include "udp-cmd.h"
#include "net/rpl/rpl.h"

#define YLOG_LEVEL YLOG_LEVEL_INFO
#define YLOG_NAME  "udp-cmd"
#include "lib/ylog.h"

#define PORT 49111
#define PORT_MAX 65000

#define PSP_PORTAL_STATUS_UNIT_ON_ACCEPT_LIST      0
#define PSP_PORTAL_STATUS_UNIT_CONTROLLER_WAITING  1

extern uint8_t radio_info;

static unsigned char buf[PDU_MAX_LEN];
static int udp_fd = -1;
static struct sockaddr_in server;
static struct sockaddr_in client;
static struct sockaddr_in unit_controller;

#define TLV_QUEUE_LEN 10
tlv_in_progress tlv_queue[TLV_QUEUE_LEN];

process_event_t tlv_queue_event;
void send_single_tlv(TLV *t);
#define SINGLE_TLV_BUF_LEN 1280
volatile uint8_t single_tlv_buf[SINGLE_TLV_BUF_LEN];
static volatile uint16_t single_tlv_length;
static instanceControl *localIC = NULL;

static size_t (*standard_0_process_request)(TLV *request, uint8_t *reply, size_t len, oam_end_processing *endProcessing) = NULL;
static size_t (*standard_0_poll)(uint8_t instance, uint8_t *reply, size_t len, oam_poll_type poll_type) = NULL;

extern uint8_t wait_for_address;
static uint8_t have_address;
static uint8_t have_prefix;
extern const char *slip_config_ipaddr;
extern uint16_t slip_config_unit_controller_port;
static char router_address_string[64]; // 0000:1111:2222:3333:4444:5555:6666:7777/128
static uint8_t router_address[16];
static uint32_t router_prefix_len;

PROCESS(tlv_queue_proc, "process TLV queue");

/*---------------------------------------------------------------------------*/
static int
set_fd(fd_set *rset, fd_set *wset)
{
  if(udp_fd >= 0) {
    FD_SET(udp_fd, rset);	/* Read from udp ASAP! */
  }
  return 1;
}
/*---------------------------------------------------------------------------*/

tlv_in_progress *
next_free_tlv_queue()
{
  int i;
  // serialize
  for (i = 0 ; i < TLV_QUEUE_LEN; i++) {
    if (tlv_queue[i].fd == -1) {
      return &tlv_queue[i];
    }
  }
  return (tlv_in_progress *)NULL;
}
/*---------------------------------------------------------------------------*/
void
handle_tlv_request(tlv_in_progress *t)
{
  int retval;
  retval = process_post(&tlv_queue_proc, tlv_queue_event, (void *)t);
  if (retval != PROCESS_ERR_OK) {
    if (retval == PROCESS_ERR_FULL) {
      YLOG_ERROR("Event queue full for process tlv_queue_proc, dropping request\n");
      t->fd = -1;
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
handle_portal_selection(const uint8_t *request, size_t len, const pdu_info *pinfo)
{
#if BORDER_ROUTER_SET_RADIO_WATCHDOG > 0
  static uint8_t info_set = 0;
#endif /* BORDER_ROUTER_SET_RADIO_WATCHDOG > 0 */

#ifdef BORDER_ROUTER_CONF_SET_RADIO_FRONTPANEL_OK
#ifdef BORDER_ROUTER_CONF_SET_RADIO_FRONTPANEL_PENDING
  static uint16_t last_set = 0xffff;
  uint32_t local32;
  uint16_t fp;
  int reqpos;
  int thislen;
  TLV T;
  TLV *t =  &T;

  for(reqpos = 0; (thislen = tlv_from_bytes(t, request + reqpos)) > 0; reqpos += thislen) {
    if((thislen + reqpos) > len) {
      break;
    }
    if(t->opcode == TLV_OPCODE_GET_RESPONSE) {
      if(t->instance == 0 && t->variable == 0x10d) { /* VARIABLE_PSP_PORTAL_STATUS */
        local32 = get_int32_from_data(t->data);
        YLOG_DEBUG("Portal status: %lu\n", (unsigned long)local32);

        if(local32 == PSP_PORTAL_STATUS_UNIT_CONTROLLER_WAITING) {
          /* Unit controller is unpeered and ready to be peered. This
             means it is running and connected to the portal so
             everything seems to be ok. */
          YLOG_DEBUG("unit controller is unpeered and waiting to be peered\n");
          fp = BORDER_ROUTER_CONF_SET_RADIO_FRONTPANEL_OK;
        } else {
          /* If the unit controller is peered, it should soon take
             control of the front panel. If it is not peered,
             something prevents it from becoming peered. */
          fp = BORDER_ROUTER_CONF_SET_RADIO_FRONTPANEL_PENDING;
        }
        if(fp != last_set) {
          last_set = fp;
          border_router_set_frontpanel_info(fp);
        }
      }
      break;
    }
  }
#endif /* BORDER_ROUTER_CONF_SET_RADIO_FRONTPANEL_OK */
#endif /* BORDER_ROUTER_CONF_SET_RADIO_FRONTPANEL_PENDING */

  /* Native border router should manage the serial radio watchdog until
     the unit controller has been peered and is fully up and running. */
#if BORDER_ROUTER_SET_RADIO_WATCHDOG > 0
  if((info_set & 2) == 0) {
    info_set |= 2;
    YLOG_INFO("Setting radio watchdog to %d\n", BORDER_ROUTER_SET_RADIO_WATCHDOG);
  }
  border_router_set_radio_watchdog(BORDER_ROUTER_SET_RADIO_WATCHDOG);
#endif /* BORDER_ROUTER_SET_RADIO_WATCHDOG > 0 */
}

/*---------------------------------------------------------------------------*/
static void
handle_fd(fd_set *rset, fd_set *wset)
{
  if(udp_fd >= 0 && FD_ISSET(udp_fd, rset)) {
    int l;
    unsigned int a=sizeof(client);
    int32_t encap_header_length;
    uint8_t txbuf[512];
    uint8_t *did = getDid();
    pdu_info pinfo;

    if ((l=recvfrom(udp_fd, buf, sizeof(buf), 0, (struct sockaddr*)&client, &a)) == -1) {
      if(errno == EAGAIN) {
        return;
      }
      err(1, "udp-cmd: recv");
      return;
    }

    memset((void *) &txbuf, 0, sizeof (txbuf));

    encap_header_length = verify_and_decrypt_message(buf, l, &pinfo);
    if (encap_header_length < 0) {
      YLOG_ERROR("decode message: error %d\n", encap_header_length);
    }
    /* Always device ID finger print. No encryption for local access. */
    pinfo.fpmode = ENC_FINGERPRINT_MODE_DEVID;
    pinfo.fp = &did[8];
    pinfo.fplen = 8;

    if (pinfo.payload_type == ENC_PAYLOAD_TLV) {
      tlv_in_progress *t = next_free_tlv_queue();
      if (t != NULL) {
        t->txlen = write_encap_header(t->txbuf, sizeof (t->txbuf), &pinfo, ENC_ERROR_OK);
        memcpy(t->rxbuf, &buf[encap_header_length], pinfo.payload_len);
        memcpy(&t->sin, &client, a);
        t->payload_len = pinfo.payload_len;
        t->fd = udp_fd;
        handle_tlv_request(t);
      }
      l = 0;
    } else if (pinfo.payload_type == ENC_PAYLOAD_PORTAL_SELECTION_PROTO) {
      /* Receiving portal selection protocol payload means the unit controller
         is running but is not yet peered. Silently drop the message. */
      l = 0;

      handle_portal_selection(&buf[encap_header_length], pinfo.payload_len, &pinfo);

    } else if (pinfo.payload_type == ENC_PAYLOAD_ECHO_REQUEST) {
      pinfo.payload_type =  ENC_PAYLOAD_ECHO_REPLY;
      l = write_encap_header(txbuf, sizeof (txbuf), &pinfo, ENC_ERROR_OK);
      memcpy(txbuf + l, buf + encap_header_length, 4);
      l += 4;
    } else if (pinfo.payload_type == ENC_PAYLOAD_ECHO_REPLY) {
      l = 0; // silent drop
    } else {
      l = write_encap_header(txbuf, sizeof (txbuf), &pinfo, ENC_ERROR_BAD_PAYLOAD_TYPE);
    }

     if (l > 0) {
         sendto(udp_fd, txbuf, l, 0, (struct sockaddr *)&client, a);
     }
     memset((void *) &buf, 0, sizeof(buf));
  }
}
/*---------------------------------------------------------------------------*/
static const struct select_callback udp_callback = { set_fd, handle_fd };
/*---------------------------------------------------------------------------*/
void
udp_cmd_init(void)
{
  int i;
  for (i = 0; i < TLV_QUEUE_LEN; i++) {
    tlv_queue[i].fd = -1;
  }
  tlv_queue_event = process_alloc_event();
  process_start(&tlv_queue_proc, NULL);
}
/*---------------------------------------------------------------------------*/
void
udp_cmd_start(void)
{
  uint16_t port = PORT;
  /* create a UDP socket */
  if((udp_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    YLOG_ERROR("Error creating socket...\n");
    return;
  }

  memset((void *) &server, 0, sizeof(server));

  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  /* bind socket to port */

  while (bind(udp_fd, (struct sockaddr*)&server, sizeof(server) ) == -1) {
    port++;
    server.sin_port = htons(port);
    if (port > PORT_MAX) {
      YLOG_ERROR("Error binding UDP port\n");
      return;
    }
  }

  YLOG_INFO("Started UDP cmd on port %d\n", port);
  select_set_callback(udp_fd, &udp_callback);
}
/*---------------------------------------------------------------------------*/
void
send_poll_response() {
  int l;
  uint8_t *did;
  pdu_info pinfo;

  if(udp_fd < 0) {
    /* Server has not yet been started */
    return;
  }

  did = getDid();
  memset((void *) &buf, 0, sizeof(buf));

  memset(&pinfo, 0, sizeof (pinfo));
  pinfo.fp = &did[8];
  pinfo.fpmode = ENC_FINGERPRINT_MODE_DEVID;
  pinfo.fplen = 8;
  pinfo.payload_type = ENC_PAYLOAD_TLV;

  l = write_encap_header(buf, sizeof (buf), &pinfo, ENC_ERROR_OK);
  if (l < 4) {
      YLOG_ERROR("failed to write encap header\n");
  } else {
      l += localIC[0].poll(0, buf + l, sizeof (buf) - l, OAM_POLL_TYPE_POLL);
      // null TLV
      buf[l++] = 0;
      buf[l++] = 0;

      unit_controller.sin_family = AF_INET;
      unit_controller.sin_port = htons(slip_config_unit_controller_port);
      unit_controller.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // localhost 127.0.0.1

      sendto(udp_fd,buf,l,0,(struct sockaddr *)&unit_controller, sizeof(unit_controller));
  }
}

/*---------------------------------------------------------------------------*/
void
send_single_tlv(TLV *t)
{
  int len = 0;
  uint8_t localbuf[1280];

  len += tlv_to_bytes(t, &localbuf[len], sizeof (localbuf) - len);
  localbuf[len++] = 0;
  localbuf[len++] = 0;
  write_to_slip_payload_type(localbuf, len, ENC_PAYLOAD_TLV);
}

/*---------------------------------------------------------------------------*/
void
get_radio_info()
{
  TLV T;
  int len = 0;

  memset((void *) &buf, 0, sizeof(buf));

  T.version = TLV_VERSION;
  T.instance = 0;
  T.opcode = TLV_OPCODE_GET_REQUEST;
  T.error = TLV_ERROR_NO_ERROR;

  if ((radio_info & RADIO_INFO_PRODUCT_TYPE) == 0) {
    T.variable = VARIABLE_PRODUCT_TYPE;
    T.elementSize = 8;
    len += tlv_to_bytes(&T, &buf[len], sizeof (buf) - len);
  }

  if ((radio_info & RADIO_INFO_PRODUCT_ID) == 0) {
    T.variable = VARIABLE_PRODUCT_ID;
    T.elementSize = 16;
    len += tlv_to_bytes(&T, &buf[len], sizeof (buf) - len);
  }

  if ((radio_info & RADIO_INFO_SW_REVISION) == 0) {
    T.variable = VARIABLE_SW_REVISION;
    T.elementSize = 16;
    len += tlv_to_bytes(&T, &buf[len], sizeof (buf) - len);
  }

  if ((radio_info & RADIO_INFO_BL_VERSION) == 0) {
    T.variable = 0x0e1; /* VARIABLE_CHASSIS_BOOTLOADER_VERSION */
    T.elementSize = 4;
    len += tlv_to_bytes(&T, &buf[len], sizeof (buf) - len);
  }

  if ((radio_info & RADIO_INFO_CAPABILITIES) == 0) {
    T.variable = 0x0e0; /* VARIABLE_CHASSIS_CAPABILITIES */
    T.elementSize = 8;
    len += tlv_to_bytes(&T, &buf[len], sizeof (buf) - len);
  }

  if ((radio_info & RADIO_INFO_INSTANCES) == 0) {
    T.variable = VARIABLE_NUMBER_OF_INSTANCES;
    T.elementSize = 4;
    len += tlv_to_bytes(&T, &buf[len], sizeof (buf) - len);
  }
  /* null TLV to terminate stack */
  buf[len++] = 0;
  buf[len++] = 0;

  write_to_slip_payload_type(buf, len, ENC_PAYLOAD_TLV);
}

/*---------------------------------------------------------------------------*/

void
process_tlv_from_radio(uint8_t *stack, int stacklen)
{
  uint32_t local32;
  int stackpos;
  int thislen = 0;
  TLV T;
  TLV *t =  &T;

#if 0
  YLOG_DEBUG("process_tlv_from_radio: stacklen = %d\n", stacklen);
  for (stackpos = 0 ; stackpos < stacklen; stackpos++) {
    printf("%02x ", stack[stackpos]);
    if ((stackpos + 1) % 16 == 0) {
      printf("\n");
    } else if ((stackpos + 1) % 8 == 0) {
      printf("  ");
    }
  }
  printf("\n");
#endif /* 0 */

  for (stackpos = 0; (thislen = tlv_from_bytes(t, &stack[stackpos])) > 0; stackpos += thislen) {
    /*
     * If parsing this TLV would read outside buffer, abort stack parsing.
     */
    if ((thislen + stackpos) >  stacklen) {
      DEBUG_PRINT_I("TLV @ %d parsing beyond buffer, aborting\n", stackpos);
      return;
    }
    /*
     * This is a tunneled instance
     */
    if (localIC != NULL) {
      if (localIC[t->instance].process == INSTANCE_PROCESS_EXTERNAL) {
        if (single_tlv_length != 0) {
          YLOG_ERROR("process_tlv_from_radio: no free buffer; dropping TLV instance %d variable 0x%03x\n",
                 t->instance, t->variable);
          continue;
        }
        if (thislen > SINGLE_TLV_BUF_LEN) {
          YLOG_ERROR("process_tlv_from_radio: TLV too long (%d bytes); dropping TLV instance %d variable 0x%03x\n",
                 thislen, t->instance, t->variable);
          continue;
        }
        memcpy((void *)single_tlv_buf, &stack[stackpos], thislen);
        single_tlv_length = thislen;
        continue;
      }
    } else {
      // Not initialized yet.
    }

    /*
     * Process Event request
     */
    if ((t->opcode == TLV_OPCODE_EVENT_REQUEST) || (t->opcode == TLV_OPCODE_VECTOR_EVENT_REQUEST)) {
      YLOG_DEBUG("Got event from radio, now what?\n");
      continue;
    }

    if (t->variable == VARIABLE_PRODUCT_TYPE) {
      uint64_t product = 0;
      int i;
      for (i = 0 ; i < 8; i++) {
        product <<= 8;
        product += t->data[i];
      }
      if (product == SUPPORTED_RADIO_TYPE) {
        radio_info |= RADIO_INFO_PRODUCT_TYPE;
      } else {
        YLOG_ERROR("From radio: Unsupported product type %02x%02x%02x%02x%02x%02x%02x%02x\n",
               t->data[0], t->data[1], t->data[2], t->data[3],
               t->data[4], t->data[5], t->data[6], t->data[7]);
      }
    }

    if (t->variable == VARIABLE_PRODUCT_ID) {
      if ((t->data[0] == 0) &&
          (t->data[1] == 0) &&
          (t->data[2] == 0) &&
          (t->data[3] == 0) &&
          (t->data[4] == 0) &&
          (t->data[5] == 0) &&
          (t->data[6] == 0) &&
          (t->data[7] == 0)) {
        memcpy(uip_lladdr.addr,&t->data[8], sizeof(uip_lladdr.addr));
        memcpy(wsn_mac_addr.addr, uip_lladdr.addr, sizeof(uip_lladdr.addr));
        linkaddr_set_node_addr((linkaddr_t *)uip_lladdr.addr);
        radio_mac_addr_ready = 1;
        radio_info |= RADIO_INFO_PRODUCT_ID;
#if 0
        YLOG_INFO("did is set to\n");
        {
          int i;
          for (i = 0; i < 8; i++) {
            printf("%02x", uip_lladdr.addr[i]);
          }
          printf("\n");
        }
#endif /* 0 */
      } else {
        YLOG_ERROR("Got bad DID from radio\n");
      }
    }

    if (t->variable == VARIABLE_SW_REVISION) {
      if (t->error == TLV_ERROR_NO_ERROR && t->data) {
        instance_brm_set_radio_sw_revision((char *)t->data);
      }
      radio_info |= RADIO_INFO_SW_REVISION;
    }

    if (t->variable == 0x0e1) { /* VARIABLE_CHASSIS_BOOTLOADER_VERSION */
      if (t->error == TLV_ERROR_NO_ERROR && t->data) {
        local32 = get_int32_from_data(t->data);
        instance_brm_set_radio_bootloader_version(local32);
      } else {
        YLOG_ERROR("radio did not give any bootloader version (%u)\n", t->error);
      }
      radio_info |= RADIO_INFO_BL_VERSION;
    }

    if (t->variable == 0x0e0) { /* VARIABLE_CHASSIS_CAPABILITIES */
      if (t->error == TLV_ERROR_NO_ERROR && t->data) {
        uint64_t data = get_int64_from_data(t->data);
        instance_brm_set_radio_capabilities(data);
      } else {
        YLOG_ERROR("radio did not give any chassis capabilities (%u)\n", t->error);
      }
      radio_info |= RADIO_INFO_CAPABILITIES;
    }

    if (t->variable == VARIABLE_NUMBER_OF_INSTANCES) {
      radio_info |= RADIO_INFO_INSTANCES;
    }
  }
}

/*---------------------------------------------------------------------------*/

/*
 * Dummy only, to be able to compare pointer.
 */
size_t
slip_radio_process_request(TLV *request, uint8_t *reply, size_t len, oam_end_processing *endProcessing)
{
  return 0;
}
/*---------------------------------------------------------------------------*/


/*
 * On new PDU; clear use count.
 * Keep track of use, and return true for first use each PDU.
 * This is the same for all tunneled instances.
 */
uint8_t
radio_instances_new_tlv_pdu(uint8_t operation) {
  static int use = 0;
  if (operation == NEW_TLV_PDU_OPERATION_NEW) {
    use = 0;
  } else if (operation == NEW_TLV_PDU_OPERATION_USE) {
    use++;
    if (use == 1) {
      return TRUE;
    }
  }
  return FALSE;
}
/*----------------------------------------------------------------*/

uint8_t
oam_do_action(oam_action o, uint8_t parameter) {
  switch (o) {
  case OAM_ACTION_NEW_PDU:
    return radio_instances_new_tlv_pdu(parameter);
    break;
  default:
    return 0;
    break;
  }
  return 0;
}
/*----------------------------------------------------------------*/

/*
 * Initialize the instances accessed via slip tunnel.
 */
void
radio_instances_init(instanceControl IC[])
{
  localIC = IC;
  int i;

  for (i = 1; i <= RADIO_INSTANCES; i++) {
    memset(&IC[i], 0, sizeof (instanceControl));
    IC[i].processRequest = slip_radio_process_request;
    IC[i].process = INSTANCE_PROCESS_EXTERNAL;
    IC[i].oam_do_action = oam_do_action;
  }
}
/*---------------------------------------------------------------------------*/

void
check_complete_address()
{
  if (have_address && have_prefix) {
    int i;
    memset(router_address_string, 0, sizeof (router_address_string));
    for (i = 0; i < 8; i++) {
      snprintf(router_address_string + strlen(router_address_string) , sizeof (router_address_string) - strlen(router_address_string),
               "%02x%02x%c", router_address[i * 2], router_address[i * 2 + 1], (i == 7) ? '/' : ':');
    }
    snprintf(router_address_string + strlen(router_address_string) , sizeof (router_address_string) - strlen(router_address_string),
             "%d", router_prefix_len);

    slip_config_ipaddr = router_address_string;
    wait_for_address = FALSE;
  }
}
/*---------------------------------------------------------------------------*/

size_t
process_instance_0_variables(TLV *request, uint8_t *reply, size_t len, oam_end_processing *endProcessing)
{

  /*
   * Handle all discovery and instance 0 variables in default handler.
   * Even sw revision and boot timer, even if they are defined as
   * higher than 0x100. Also keep reset hardware here, so we can send
   * it to slip radio, because we did not really mean to reset the
   * border router.
   */
  if ((request->variable != 0xca) &&
      ((request->variable < 0x100)
#ifdef VARIABLE_OLD_UNIT_BOOT_TIMER
       || (request->variable == VARIABLE_OLD_UNIT_BOOT_TIMER)
#endif
#ifdef VARIABLE_OLD_SW_REVISION
       || (request->variable == VARIABLE_OLD_SW_REVISION)
#endif
       )) {
    return standard_0_process_request(request, reply, len, endProcessing);
  }

  if ((request->opcode == TLV_OPCODE_SET_REQUEST) || (request->opcode == TLV_OPCODE_VECTOR_SET_REQUEST)) {
    /* write requests */
#ifdef VARIABLE_ROUTER_ADDRESS
    if (request->variable == VARIABLE_ROUTER_ADDRESS) {
      memcpy(router_address, request->data, 16);
      have_address = TRUE;
      check_complete_address();
      return writeReply128Tlv(request, reply, len, 0);
    }
#endif /* VARIABLE_ROUTER_ADDRESS */

#ifdef VARIABLE_ROUTER_ADDRESS_PREFIX_LEN
    if (request->variable == VARIABLE_ROUTER_ADDRESS_PREFIX_LEN) {
      if ((request->data[0] != 0) || (request->data[1] != 0) ||
          (request->data[2] != 0) || (request->data[3] > 128)) {
        return writeErrorTlv(request, TLV_ERROR_INVALID_ARGUMENT, reply, len);
      }
      router_prefix_len = request->data[3];
      have_prefix = TRUE;
      check_complete_address();
      return writeReply32Tlv(request, reply, len, 0);
    }
#endif /* VARIABLE_ROUTER_ADDRESS_PREFIX_LEN */

#ifdef VARIABLE_HARDWARE_RESET
    if (request->variable == VARIABLE_HARDWARE_RESET) {
     /*
      * Send to slip radio, don't bother with reply.
      */
      send_single_tlv(request);
      return 0;
    }
#endif /* VARIABLE_HARDWARE_RESET */

    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_VARIABLE, reply, len);

  } else if ((request->opcode == TLV_OPCODE_GET_REQUEST) || (request->opcode == TLV_OPCODE_VECTOR_GET_REQUEST)) {


#ifdef VARIABLE_ROUTER_READY
    if (request->variable == VARIABLE_ROUTER_READY) {
      uint32_t data = BORDER_ROUTER_STATUS_CONFIGURATION_HAS_STATUS_BITS;
      if(rpl_get_any_dag() != (rpl_dag_t *)NULL) {
	data |= BORDER_ROUTER_STATUS_CONFIGURATION_READY;
      }
      data |= border_router_get_status();
      printf("Router ready: %08X\n", data);
      return writeReply32intTlv(request, reply, len, data);
    }
#endif /* VARIABLE_ROUTER_READY */

#ifdef VARIABLE_ROUTER_ADDRESS
    if (request->variable == VARIABLE_ROUTER_ADDRESS) {
      return writeReply128Tlv(request, reply, len, router_address);
    }
#endif /* VARIABLE_ROUTER_ADDRESS */

#ifdef VARIABLE_ROUTER_ADDRESS_PREFIX_LEN
    if (request->variable == VARIABLE_ROUTER_ADDRESS_PREFIX_LEN) {
      return writeReply32intTlv(request, reply, len, router_prefix_len);
    }
#endif /* VARIABLE_ROUTER_ADDRESS_PREFIX_LEN */

    return writeErrorTlv(request, TLV_ERROR_UNKNOWN_VARIABLE, reply, len);
  }
  return writeErrorTlv(request, TLV_ERROR_UNKNOWN_OP_CODE, reply, len);
}

/*---------------------------------------------------------------------------*/
static size_t
process_instance_0_poll(uint8_t instance, uint8_t *reply, size_t len, oam_poll_type poll_type)
{
  int replyLen = 0;
  TLV T;

  if(standard_0_poll) {
    replyLen += standard_0_poll(instance, &reply[replyLen], len - replyLen, poll_type);
  }

  if(rpl_get_any_dag() != NULL) {
    T.version = TLV_VERSION;
    T.instance = instance;
    T.opcode = TLV_OPCODE_GET_REQUEST;
    T.error = TLV_ERROR_NO_ERROR;
    T.variable = VARIABLE_ROUTER_READY;
    T.elementSize = 4;

    replyLen += writeReply32intTlv(&T, &reply[replyLen], len - replyLen, 1);
  }

  return replyLen;
}

/*---------------------------------------------------------------------------*/
void
br_init(instanceControl IC[])
{
  /*
    insert out own instance 0 tlv processor first. And call the
    standard for variables < 0x100.
  */
  standard_0_process_request = IC[0].processRequest;
  IC[0].processRequest = process_instance_0_variables;
  standard_0_poll = IC[0].poll;
  IC[0].poll = process_instance_0_poll;
}

/*---------------------------------------------------------------------------*/


/*
 * Process TLV PDU from unit controller.
 *
 */
PROCESS_THREAD(tlv_queue_proc, ev, data) {
  static struct etimer response_timer;
  static tlv_in_progress *tip;
  static int reqpos;
  static int replyLen = 0;
  static int thislen = 0;
  static TLV T;
  static TLV *t =  &T;
  static oam_end_processing endProcessing = OAM_END_PROCESSING_NEW;
  static uint8_t error;
  static size_t replymax;
  static int sleep_count;

  PROCESS_BEGIN();

  while (TRUE) {
    PROCESS_WAIT_EVENT_UNTIL(ev == tlv_queue_event);
    if (data == NULL) {
      continue;
    }
    if (localIC == NULL) {
      continue;
    }
    tip = (tlv_in_progress *)data;

    /* The "-2" is to leave room for the null tlv added after tlv stack. */
    replymax = sizeof (tip->txbuf) - tip->txlen -2;

    /* Drop request without request or with too small reply buffer */
    if ((tip->payload_len <= 0) || (replymax < 8)) {
      tip-> fd = -1;
      continue;
    }

    stackBegin();

    for (reqpos = 0; (thislen = tlv_from_bytes(t, tip->rxbuf + reqpos)) > 0; reqpos += thislen) {
      /*
       * Check if any of the TLVs will write to the radio, if so ,
       * send grouping config.
       */
      if ((t->instance >= 1) && (t->instance <= RADIO_INSTANCES)) {
        t->version = TLV_VERSION;
        t->instance = 0;
        t->opcode = TLV_OPCODE_SET_REQUEST;
        t->error = TLV_ERROR_NO_ERROR;
        t->variable = 0xcd;
        t->elementSize = 4;
        t->data = data;
        send_single_tlv(t);
        /* once is enough */
        break;
      }
    }

    for (replyLen = 0, reqpos = 0; (thislen = tlv_from_bytes(t, tip->rxbuf + reqpos)) > 0; reqpos += thislen) {
      /*
       * If parsing this TLV would read outside buffer, abort stack parsing.
       */
      if ((thislen + reqpos) >  tip->payload_len) {
        DEBUG_PRINT_I("TLV @ %d parsing beyond buffer, aborting\n", reqpos);
        break;
      }

      /*
       * Send hardware reset direct to the radio
       */
      if ((t->instance == 0) && (t->variable == 0xca)) {
        send_single_tlv(t);
        continue;
      }

      /*
       * If this instance is handled elsewhere, give it there, without any checks
       */
      if (localIC[t->instance].process == INSTANCE_PROCESS_EXTERNAL) {
        /* Only handle requests to slip_radio_process_request inline. */
        if (localIC[t->instance].processRequest != slip_radio_process_request) {
          replyLen += localIC[t->instance].processRequest(t, tip->txbuf + tip->txlen + replyLen,
                                                          replymax - replyLen, &endProcessing);
          continue;
        }

        single_tlv_length = 0;
        send_single_tlv(t);

        /* wait for reply, and on timeout return 0 */
        for (sleep_count = 0; (sleep_count < 256) && (single_tlv_length == 0); sleep_count++) {
          etimer_set(&response_timer, CLOCK_SECOND / 64);
          PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&response_timer));
        }

        if (single_tlv_length > 0) {
          //printf("Received %d bytes from radio after %d sleeps\n", single_tlv_length, sleep_count);
          memcpy(tip->txbuf + tip->txlen + replyLen, (void *)single_tlv_buf, single_tlv_length);
          replyLen += single_tlv_length;
          single_tlv_length = 0;
        }
        continue;
      }

      /*
       * Reflector for any instance is handled here.
       */
      if ((t->opcode == TLV_OPCODE_REFLECT_REQUEST) || (t->opcode == TLV_OPCODE_VECTOR_REFLECT_REQUEST)) {
        replyify(t);
        replyLen += tlv_to_bytes(t, tip->txbuf + tip->txlen + replyLen, replymax - replyLen);
        continue;
      }

      /*
       * Process Event request
       */
      if ((t->opcode == TLV_OPCODE_EVENT_REQUEST) || (t->opcode == TLV_OPCODE_VECTOR_EVENT_REQUEST)) {
        replyify(t);
        replyLen += tlv_to_bytes(t, tip->txbuf + tip->txlen + replyLen, replymax - replyLen);
        //respond_to_uc = TRUE;
        continue;
      }

      error = checkTLV(t, INSTANCES, localIC, FALSE);
      //DEBUG_PRINT_II("@%04d: Checking TLV error: %d\n", reqpos, error);
      if (error == TLV_ERROR_UNPROCESSED_TLV) {
        // Silently skip it.
        continue;
      }
      if (error != TLV_ERROR_NO_ERROR) {
        //t->error = error;
        //replyLen += tlv_to_bytes(t, tip->txbuf + tip->txlen + replyLen, replymax - replyLen);
        replyLen += writeErrorTlv(t, error, tip->txbuf + tip->txlen + replyLen, replymax - replyLen);
        continue;
      }

      /*
       * Process request
       */
      //DEBUG_PRINT_II("@%04d: replyLen before: %d\n", reqpos, replyLen);
      if (is_discovery_variable(t)) {
        replyLen += processDiscoveryRequest(t, tip->txbuf + tip->txlen + replyLen,
                                            replymax - replyLen, &endProcessing, localIC);
      } else {
        replyLen += localIC[t->instance].processRequest(t, tip->txbuf + tip->txlen + replyLen,
                                                        replymax - replyLen, &endProcessing);
      }
      //DEBUG_PRINT_II("@%04d: replyLen after : %d\n", reqpos, replyLen);

      if (endProcessing & OAM_END_PROCESSING_ABORT_TLV_STACK) {
        break;
      }
    } /* for (reqpos = 0; (thislen = tlv_from_bytes(t, tip->rxbuf + reqpos)) > 0; reqpos += thislen) { */

    stackEnd();
    tip->txlen += replyLen;

    tip->txbuf[tip->txlen++] = 0;
    tip->txbuf[tip->txlen++] = 0;

    if (tip->txlen > 2) {
      sendto(tip->fd, tip->txbuf, tip->txlen, 0, (struct sockaddr *)&tip->sin, sizeof (struct sockaddr_in));
    }

    /* set it "free" */
    tip->fd = -1;
  } /* while (TRUE) { */

  PROCESS_END();
}

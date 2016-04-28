#include "contiki.h"
#include "tinydtls.h"
#include "dtls.h"
#include <inttypes.h>

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[UIP_LLIPH_LEN])
int send_to_peer(struct dtls_context_t *, session_t *, uint8 *, size_t);
static struct uip_udp_conn *server_conn;
static dtls_context_t *dtls_context;
static dtls_handler_t cb = {
  .write = send_to_peer,
  .read  = read_from_peer,
  .event = NULL,
  .get_psk_key = get_psk_key
};
PROCESS(server_process, "DTLS server process");
AUTOSTART_PROCESSES(&server_process);
PROCESS_THREAD(server_process, ev, data)
{
  PROCESS_BEGIN();
  dtls_init();
  server_conn = udp_new(NULL, 0, NULL);
  udp_bind(server_conn, UIP_HTONS(5684));
  dtls_context = dtls_new_context(server_conn);
  if (!dtls_context) {
    dtls_emerg("cannot create context\n");
    PROCESS_EXIT();
  }
  dtls_set_handler(dtls_context, &cb);
  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == tcpip_event && uip_newdata()) {
      session_t session;
      uip_ipaddr_copy(&session.addr, &UIP_IP_BUF->srcipaddr);
      session.port = UIP_UDP_BUF->srcport;
      session.size = sizeof(session.addr) + sizeof(session.port);
    
      dtls_handle_message(ctx, &session, uip_appdata, uip_datalen());
    }
  }
  PROCESS_END();
}
int send_to_peer(struct dtls_context_t *ctx, session_t *session, uint8 *data, size_t len) {
  struct uip_udp_conn *conn = (struct uip_udp_conn *)dtls_get_app_data(ctx);
  uip_ipaddr_copy(&conn->ripaddr, &session->addr);
  conn->rport = session->port;
  uip_udp_packet_send(conn, data, len);
  memset(&conn->ripaddr, 0, sizeof(server_conn->ripaddr));
  memset(&conn->rport, 0, sizeof(conn->rport));
  return len;
}

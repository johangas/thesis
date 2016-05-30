/*
 * Copyright (c) 2001, Adam Dunkels.
 * Copyright (c) 2009, 2010 Joakim Eriksson, Niclas Finne, Dogan Yazar.
 * Copyright (c) 2013-2015 Yanzi Networks AB.
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

 /* Below define allows importing saved output into Wireshark as "Raw IP" packet type */
#define WIRESHARK_IMPORT_FORMAT 1
#include "contiki.h"

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <err.h>

#include "lib/list.h"
#include "lib/memb.h"
#include "lib/ringbuf.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "cmd.h"
#include "border-router.h"
#include "border-router-cmds.h"
#include "udp-cmd.h"
#include "brm-stats.h"
#include "crc.h"
#include "encap.h"
#include "enc-dev.h"

#ifndef WITH_SPI
#define WITH_SPI 0
#endif

#if WITH_SPI
#include <linux/spi/spidev.h>
#endif

#define YLOG_LEVEL YLOG_LEVEL_DEBUG
#define YLOG_NAME  "enc"
#include "lib/ylog.h"

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

extern int slip_config_verbose;
extern int slip_config_flowcontrol;
extern const char *slip_config_siodev;
extern const char *slip_config_host;
extern const char *slip_config_port;
extern speed_t slip_config_b_rate;

PROCESS_NAME(serial_input_process);

static void stty_telos(int fd);

#define DEBUG_LINE_MARKER '\r'

#ifndef HAVE_OAM
uint8_t did[] = {0,1,2,3,4,5,6,7,8};
uint8_t *getDid() {
  return did;
}
#endif /* HAVE_OAM */

#ifdef SLIP_DEV_CONF_SEND_DELAY
#define SEND_DELAY_DEFAULT SLIP_DEV_CONF_SEND_DELAY
#else
#define SEND_DELAY_DEFAULT 0
#endif

#ifdef SLIP_DEV_CONF_BUFFER_SIZE
#define SLIP_DEV_BUFFER_SIZE SLIP_DEV_CONF_BUFFER_SIZE
#else
#define SLIP_DEV_BUFFER_SIZE 4096
#endif

#define LOG_LIMIT_ERROR(...)                                            \
  do {                                                                  \
    static clock_time_t _log_error_last;                                \
    static uint8_t _log_error_count = 0;                                \
    if(_log_error_count >= 5) {                                         \
      if(clock_time() - _log_error_last > 5 * 60UL * CLOCK_SECOND) {    \
        _log_error_count = 0;                                           \
      }                                                                 \
    } else {                                                            \
      _log_error_count++;                                               \
    }                                                                   \
    if(_log_error_count < 5) {                                          \
      YLOG_ERROR(__VA_ARGS__);                                          \
      _log_error_last = clock_time();                                   \
    }                                                                   \
  } while(0)

int devopen(const char *dev, int flags);

static const struct enc_dev_tunnel *tunnel = NULL;

/* delay between slip packets */
static struct ctimer send_delay_timer;
static uint32_t send_delay = 0;
uint8_t verbose_output = 1;

/* for statistics */
unsigned long slip_sent = 0;
unsigned long slip_sent_to_fd = 0;

static int slipfd = -1;

#define PACKET_MAX_COUNT 64
#define PACKET_MAX_SIZE 1280

typedef struct {
  void *next;
  uint16_t len;
  uint8_t data[PACKET_MAX_SIZE];
} packet_t;

MEMB(packet_memb, packet_t, PACKET_MAX_COUNT);
LIST(pending_packets);
static packet_t *active_packet = NULL;

//#define PROGRESS(s) fprintf(stderr, s)
#define PROGRESS(s) do { } while(0)

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335


/*---------------------------------------------------------------------------*/
#if WITH_SPI
#define GPIOS_NUM 8
#define GPIO_RADIO_INT  14
typedef struct gpio_s
{
  int number;
  char* direction;
  int value;
  char* drive;
} gpio_t;
static struct ringbuf spi_rx_buf;
static uint8_t rxbuf_data[1024];
int spi_reset_request = 0;
gpio_t gpios[GPIOS_NUM] = {
  {4, "out", 1, ""},
  {42, "out", 0, "strong"},
  {43, "out", 0, "strong"},
  {54, "out", 0, "strong"},
  {55, "out", 0, "strong"},
  {31, "out", 0, ""}, /* required by gpio14 */
  {0, "out", 0, ""}, /* required by gpio14 */
  {14, "in", 0, ""}, /* IO2 on Galileo */
};
#endif /* WITH_SPI */
/*---------------------------------------------------------------------------*/
/* Read thread */

static pthread_t thread;

#define SLIP_BUF_COUNT 512
static uint8_t inputbuf[SLIP_BUF_COUNT][160];
static int bufsize[SLIP_BUF_COUNT];
static volatile uint16_t write_buf = 0;
static volatile uint16_t read_buf = 0;

void *read_code(void *argument)
{
   int i;

   YLOG_DEBUG("Serial Reader started!\n");
   if (slipfd > 0) {
     ssize_t size;
     while(1) {
       size = read(slipfd, inputbuf[write_buf], sizeof(inputbuf[write_buf]));
       if(size <= 0) {

         if(size == 0) {
           /* Serial connection has closed */
           YLOG_ERROR("*** serial connection closed.\n");
           for(i = 5; i > 0; i--) {
             YLOG_ERROR("*** exit in %d seconds\n", i);
             sleep(1);
           }
           exit(EXIT_FAILURE);
         }

         if(errno != EINTR && errno!= EAGAIN) {
           err(1, "enc-dev: read serial");
         }
         continue;
       }

       bufsize[write_buf] = size;
       write_buf = (write_buf + 1) % SLIP_BUF_COUNT;
       process_poll(&serial_input_process);

       if(write_buf == read_buf) {
         BRM_STATS_DEBUG_INC(BRM_STATS_DEBUG_SLIP_DROPPED);
         LOG_LIMIT_ERROR("*** reader has not read... overwriting read buffer! (%u)\n",
                         BRM_STATS_DEBUG_GET(BRM_STATS_DEBUG_SLIP_DROPPED));
       }
       /* printf(" read %d on serial wb:%d rb:%d\n", size, write_buf, read_buf); */
     }
   } else {
     YLOG_ERROR("**** reader thread exiting - slipfd not initialized... \n");
   }
   return NULL;
}

/*---------------------------------------------------------------------------*/
unsigned
serial_get_baudrate(void)
{
  return slip_config_b_rate;
}
/*---------------------------------------------------------------------------*/
void
serial_set_baudrate(unsigned speed)
{
  slip_config_b_rate = speed;
  if(slip_config_host == NULL && slipfd > 0) {
    stty_telos(slipfd);
  }
}
/*---------------------------------------------------------------------------*/
int
serial_get_ctsrts(void)
{
  return slip_config_flowcontrol;
}
/*---------------------------------------------------------------------------*/
void
serial_set_ctsrts(int ctsrts)
{
  slip_config_flowcontrol = ctsrts;
  if(slip_config_host == NULL && slipfd > 0) {
    stty_telos(slipfd);
  } else if(ctsrts) {
    YLOG_ERROR("can not set cts/rts - no serial connection\n");
  }
}
/*---------------------------------------------------------------------------*/
uint32_t
serial_get_mode(void)
{
  uint32_t v = 0;
  return v;
}
/*---------------------------------------------------------------------------*/
void
serial_set_mode(uint32_t mode)
{
}
/*---------------------------------------------------------------------------*/
uint32_t
serial_get_send_delay(void)
{
  return send_delay;
}
/*---------------------------------------------------------------------------*/
void
serial_set_send_delay(uint32_t delayms)
{
  if(send_delay != delayms) {
    send_delay = delayms;

#if CLOCK_SECOND != 1000
#warning "send delay is assuming CLOCK_SECOND is milliseconds on native platform"
#endif

    if(send_delay > 0) {
      /* No callback function is needed here - the callback timer is just
         to make sure that the application is not sleeping when it is time
         to continue sending data. set_fd()/handle_fd() will be called
         anyway when the application is awake.
      */
      ctimer_set(&send_delay_timer, send_delay, NULL, NULL);
    }
    /* Make sure the send delay timer is expired from start */
    ctimer_stop(&send_delay_timer);
  }
}
/*---------------------------------------------------------------------------*/
static packet_t *
alloc_packet(void)
{
  packet_t *p;
  p = memb_alloc(&packet_memb);

  if(p == NULL) {
    BRM_STATS_DEBUG_INC(BRM_STATS_DEBUG_SLIP_OVERFLOWS);
    LOG_LIMIT_ERROR("*** dropping pending packet (%u)\n",
                    BRM_STATS_DEBUG_GET(BRM_STATS_DEBUG_SLIP_OVERFLOWS));

    /* Drop latest pending packet */
    p = list_chop(pending_packets);
  }

  if(p) {
    memset(p, 0, sizeof(packet_t));
  }
  return p;
}
/*---------------------------------------------------------------------------*/
static void
free_packet(packet_t *p)
{
  memb_free(&packet_memb, p);
}
/*---------------------------------------------------------------------------*/
#if 0
static void *
get_in_addr(struct sockaddr *sa)
{
  if(sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
#endif /* 0 */
/*---------------------------------------------------------------------------*/
void
enc_dev_set_tunnel(const struct enc_dev_tunnel *t)
{
  tunnel = t;
}
/*---------------------------------------------------------------------------*/
static int
connect_to_server(const char *host, const char *port)
{
  /* Setup TCP connection */
  struct addrinfo hints, *servinfo, *p;
  int rv, fd;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
    err(1, "getaddrinfo: %s", gai_strerror(rv));
    return -1;
  }

  /* loop through all the results and connect to the first we can */
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    if(connect(fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(fd);
      perror("client: connect");
      continue;
    }
    break;
  }

  if(p == NULL) {
    err(1, "can't connect to ``%s:%s''", host, port);

    /* all done with this structure */
    freeaddrinfo(servinfo);
    return -1;
  }

  /* Not need for nonblocking when p-thread reader */
  /*  fcntl(fd, F_SETFL, O_NONBLOCK); */

  /* all done with this structure */
  freeaddrinfo(servinfo);
  return fd;
}
/*---------------------------------------------------------------------------*/
int
is_sensible_string(const unsigned char *s, int len)
{
  int i;
  for(i = 1; i < len; i++) {
    if(s[i] == 0 || s[i] == '\r' || s[i] == '\n' || s[i] == '\t') {
      continue;
    } else if(s[i] < ' ' || '~' < s[i]) {
      return 0;
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/

int
check_encap(unsigned char *data, int len)
{
  int enclen;
  pdu_info pinfo;
 
  enclen = verify_and_decrypt_message(data, len, &pinfo);
  if(enclen < 0) {
    if (enclen == ENC_ERROR_BAD_CHECKSUM) {
      YLOG_ERROR("Packet input failed Bad CRC, len:%d, enclen:%d\n",
             len, enclen);
      PRINTF("Should have been: %lx\n", (unsigned long) crcFast(data, len - 4));
    }
    if(slip_config_verbose > 0) {
      YLOG_ERROR("Packet input failed over SLIP-enc: %d\n => error:%d\n",
	     len, enclen);
    }
    return 0;
  }

  return enclen; /* number of bytes to remove excl. 4 bytes CRC */
}

void
slip_packet_input(unsigned char *data, int len)
{
  packetbuf_copyfrom(data, len);
  if(slip_config_verbose > 0) {
    YLOG_DEBUG("Packet input over SLIP: %d\n", len);
  }
  NETSTACK_RDC.input();
}

/*---------------------------------------------------------------------------*/
/*
 * Read from serial, when we have a packet call slip_packet_input. No output
 * buffering, input buffered by the reader thread...
 */
static void
serial_input()
{
  /* the unslipped input buffer */
  static unsigned char inbuf[SLIP_DEV_BUFFER_SIZE];
  static int inbufptr = 0;
  static int state = 0;
  /* the read buffer */
  unsigned char slipbuf[SLIP_DEV_BUFFER_SIZE];
  int ret, i, enclen, j;
  pdu_info pinfo;

  if(write_buf == read_buf) {
    /* nothing to read */
    return;
  }

  if(inbufptr + bufsize[read_buf] >= SLIP_DEV_BUFFER_SIZE) {
    /* slip buffer is full, drop everything */
    BRM_STATS_DEBUG_INC(BRM_STATS_DEBUG_SLIP_OVERFLOWS);
    LOG_LIMIT_ERROR("*** serial input buffer overflow\n");
    inbufptr = 0;
    state = 0;
  }

  BRM_STATS_DEBUG_ADD(BRM_STATS_DEBUG_SLIP_RECV, bufsize[read_buf]);

  if(tunnel != NULL) {
    if(tunnel->input) {
      tunnel->input(inputbuf[read_buf], bufsize[read_buf]);
    }
    bufsize[read_buf] = 0;
    read_buf = (read_buf + 1) % SLIP_BUF_COUNT;
    return;
  }

  memcpy(slipbuf, inputbuf[read_buf], bufsize[read_buf]);
  ret = bufsize[read_buf];
  bufsize[read_buf] = 0;
  read_buf = (read_buf + 1) % SLIP_BUF_COUNT;
  /* printf("We read:%d bytes -  wr:%d rd:%d\n", ret, write_buf, read_buf); */

  /* handle the data */
  for (j = 0; j < ret; j++) {
    /* step one step forward */
    unsigned char c = slipbuf[j];
    switch(c) {
    case SLIP_END:
      state = 0;
      if(inbufptr > 0) {
        BRM_STATS_DEBUG_INC(BRM_STATS_DEBUG_SLIP_FRAMES);
	/* debug line marker is the only one that goes without encap... */
	if(inbuf[0] == DEBUG_LINE_MARKER) {
          YLOG_INFO("SR: ");
          fwrite(inbuf + 1, inbufptr - 1, 1, stdout);
          if(inbuf[inbufptr - 1] != '\n') {
            printf("\n");
          }
        } else {
	  /* this should come in encap frames */
	  //enclen = check_encap(inbuf, inbufptr);
	  if(verbose_output > 2) {
	    printf("IN(%03u): ", inbufptr);
	    for(i = 0; i < inbufptr; i++) printf("%02x", inbuf[i]);
	    printf("\n");
	  }

          BRM_STATS_ADD(BRM_STATS_ENCAP_RECV, inbufptr);

	  enclen = verify_and_decrypt_message(inbuf, inbufptr, &pinfo);
	  if(enclen <  0) {
            BRM_STATS_INC(BRM_STATS_ENCAP_ERRORS);
            if(verbose_output) {
              YLOG_ERROR("Error - failed enc check (enc:%d len:%d)\n", enclen, inbufptr);
	      if(verbose_output < 3 && verbose_output > 1) {
		for(i = 0; i < inbufptr; i++) printf("%02x", inbuf[i]);
		printf("\n");
	      }
            }
	    /* pinfo.payload_len = inbufptr; */
	    /* pinfo.payload_type = ENC_PAYLOAD_SERIAL; // XXX: to get old style parser of bad packet. remove. */

            /* empty the input buffer and continue */
            inbufptr = 0;
            continue;
	  }

          if(pinfo.fpmode == ENC_FINGERPRINT_MODE_LENOPT
             && pinfo.fplen == 4 && pinfo.fp
             && pinfo.fp[1] == ENC_FINGERPRINT_LENOPT_OPTION_SEQNO_CRC) {
            /* Packet includes sequence number */
            /* uint32_t seqno; */
            /* seqno = inbuf[enclen + 0] << 24; */
            /* seqno |= inbuf[enclen + 1] << 16; */
            /* seqno |= inbuf[enclen + 2] << 8; */
            /* seqno |= inbuf[enclen + 3]; */
            enclen += 4;
          }

	  if (pinfo.payload_type == ENC_PAYLOAD_SERIAL) {
            BRM_STATS_INC(BRM_STATS_ENCAP_SERIAL);
	    if(inbuf[enclen] == '!') {
              command_context = CMD_CONTEXT_RADIO;
              cmd_input(&inbuf[enclen], pinfo.payload_len);
	    } else if(inbuf[enclen] == '?') {
	      /* no queries expected over slip? */
	    } else {
	      if(slip_config_verbose > 2) {
		YLOG_DEBUG("Packet from SLIP of length %d - write TUN\n", inbufptr);
		if(slip_config_verbose > 4) {
#if WIRESHARK_IMPORT_FORMAT
		  printf("0000");
		  for(i = 0; i < inbufptr; i++) printf(" %02x", inbuf[i]);
#else
		  printf("         ");
		  for(i = 0; i < inbufptr; i++) {
		    printf("%02x", inbuf[i]);
		    if((i & 3) == 3) printf(" ");
		    if((i & 15) == 15) printf("\n         ");
		  }
#endif
		  printf("\n");
		}
	      }
	      slip_packet_input(&inbuf[enclen], pinfo.payload_len);
	    }
	  } else if (pinfo.payload_type == ENC_PAYLOAD_TLV) {
            BRM_STATS_INC(BRM_STATS_ENCAP_TLV);
	    process_tlv_from_radio(&inbuf[enclen], pinfo.payload_len);

	  } else {
            BRM_STATS_INC(BRM_STATS_ENCAP_UNPROCESSED);
          }
	}
	/* empty the input buffer and continue */
	inbufptr = 0;
      }
      break;

    case SLIP_ESC:
      state = SLIP_ESC;
      break;
    case SLIP_ESC_END:
      if(state == SLIP_ESC) {
	inbuf[inbufptr++] =  SLIP_END;
	state = 0;
      } else {
	inbuf[inbufptr++] =  SLIP_ESC_END;
      }
      break;
    case SLIP_ESC_ESC:
      if(state == SLIP_ESC) {
	inbuf[inbufptr++] = SLIP_ESC;
	state = 0;
      } else {
	inbuf[inbufptr++] =  SLIP_ESC_ESC;
      }
      break;

    default:
      inbuf[inbufptr++] = c;
      state = 0;
      /* Echo lines as they are received for verbose=2,3,5+ */
    /* Echo all printable characters for verbose==4 */
      if(slip_config_verbose == 4) {
	if(c == 0 || c == '\r' || c == '\n' || c == '\t' || (c >= ' ' && c <= '~')) {
	  fwrite(&c, 1, 1, stdout);
	}
      } else if(slip_config_verbose >= 2) {
	if(c == '\n' && is_sensible_string(inbuf, inbufptr)) {
	  fwrite(inbuf, inbufptr, 1, stdout);
	  inbufptr = 0;
	}
      }
      break;
    }
  }
}
/*---------------------------------------------------------------------------*/
long
slip_buffered(void)
{
  return list_length(pending_packets);
}
/*---------------------------------------------------------------------------*/
int
slip_empty()
{
  return active_packet == NULL && list_head(pending_packets) == NULL;
}
/*---------------------------------------------------------------------------*/
#if WITH_SPI
int
gpio_get_value(unsigned int gpio)
{
  int fd;
  char buf[64];
  char ch;

  sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);

  fd = open(buf, O_RDONLY);
  read(fd, &ch, 1);
  close(fd);

  if (ch != '0') {
    return 1;
  } else {
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
void
gpio_init(void)
{
  int fd;
  int i;
  char buf[64];
  
  fd = open("/sys/class/gpio/export", O_WRONLY);
  for(i=0; i<GPIOS_NUM; i++) {
    sprintf(buf, "%d", gpios[i].number);
    write(fd, buf, strlen(buf));
  }
  close(fd);

  for(i=0; i<GPIOS_NUM; i++) {
    sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpios[i].number);
    fd = open(buf, O_WRONLY);
    write(fd, gpios[i].direction, strlen(gpios[i].direction));
    close(fd);

    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpios[i].number);
    fd = open(buf, O_WRONLY);
    sprintf(buf, "%d", gpios[i].value);
    write(fd, buf, strlen(buf));
    close(fd);

    if(strlen(gpios[i].drive)) {
      sprintf(buf, "/sys/class/gpio/gpio%d/drive", gpios[i].number);
      fd = open(buf, O_WRONLY);
      write(fd, gpios[i].drive, strlen(gpios[i].drive));
      close(fd);
    }
  }
}
/*---------------------------------------------------------------------------*/
int
spi_init(const char* device)
{
  int fd;
  char dvnm[255];
  uint8_t mode = 3;

  sprintf(dvnm, "/dev/%s", device);

  fd = open(dvnm, O_RDWR);
  if(fd < 0) {
    YLOG_ERROR("Failed to open device %s. Reason: %s\n", device, strerror(errno));
    return fd;
  }

  /* Try to set SPI in mode 3 */
  if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
    YLOG_ERROR("Failed to set SPI mode 3. Reason: %s\n", strerror(errno));
    return -1;
  }

  ringbuf_init(&spi_rx_buf, rxbuf_data, sizeof(rxbuf_data));

  return fd;
}
/*---------------------------------------------------------------------------*/
int
spi_transfer(int device, unsigned char* rxbuf, unsigned char *txbuf, int len)
{
  int status;
  struct spi_ioc_transfer xfer;
  
  xfer.cs_change = 1; /* keep CS active between consecutive transfers */
  xfer.delay_usecs = 0; /* delay between CS and transfer, in us */
  xfer.speed_hz = 2000000;
  xfer.bits_per_word = 8;

  if(spi_reset_request) {
    YLOG_ERROR("Resetting SPI...\n");
    /* magic packet to reset SPI on serial-radio node */
    unsigned char rst_msg[] = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
    xfer.tx_buf = (unsigned long)rst_msg;
    xfer.rx_buf = (unsigned long)NULL;
    xfer.len = 8;
    status = ioctl(device, SPI_IOC_MESSAGE(1), &xfer);
    if(status < 0) {
      YLOG_ERROR("Error reading SPI message. Reason: %s\n", strerror(errno));
      return -1;
    }
    /* wait until reset finish */
    usleep(20000);
    spi_reset_request = 0;
  }

  xfer.tx_buf = (unsigned long)txbuf;
  xfer.rx_buf = (unsigned long)rxbuf;
  xfer.len = len;
  status = ioctl(device, SPI_IOC_MESSAGE(1), &xfer);
  if(status < 0) {
    YLOG_ERROR("Error reading SPI message. Reason: %s\n", strerror(errno));
    return -1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
int
spi_read(int fd, unsigned char *buf,  int len)
{
  unsigned char txbuf[8] = {0x50, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  unsigned char rxbuf[8];
  int i, real_len, d;

  /* TODO: sequence of operation
   * 1. read from spi_ringbuf and put data into buf (if space left)
   *    (it is important, because we put data into spi_ringbuf during spi_write)
   *    (we need also to protect operations on ringbuf by mutexes)
   * 2. read from spi_device and put data into buf (if space left)
   * 3. if no space in buf, put rest of data from spi_device into spi_ringbuf
   */

  /* check if serial-radio requested data ready */
  if(gpio_get_value(GPIO_RADIO_INT)) {
    while(1) {
      /* Get data from radio */
      if(spi_transfer(fd, rxbuf, txbuf, 8) < 0) {
        YLOG_ERROR("SPI read failed!\n");
        return -1;
      }

      real_len = rxbuf[0] & 0x0f;

      /* Check if received data are correct */
      if(((rxbuf[0] & 0xf0) != 0x50) || (real_len > 7)) {
        spi_reset_request = 1;
        return 0;
      }

      /* Check if we received something */
      if(!real_len) {
        break;
      }

      /* Store received data */
      for (i = 0; i < real_len; i++) {
        ringbuf_put(&spi_rx_buf, rxbuf[i+1]); /* +1 because we omit first (header) byte */
      }
    }
  }

  i = 0;
  /* copy received data from ring buffer into upper level buffer */
  while(ringbuf_elements(&spi_rx_buf)) {
    if(!len) {
      break;
    }
    d = ringbuf_get(&spi_rx_buf);
    buf[i] = (unsigned char)d;
    i++;
    len--;
  }

  return i;
}
/*---------------------------------------------------------------------------*/
void *spi_read_code(void *argument)
{
  int i;

  YLOG_DEBUG("SPI Reader started!\n");
  if (slipfd > 0) {
    ssize_t size;
    while(1) {
      size = spi_read(slipfd, inputbuf[write_buf], sizeof(inputbuf[write_buf]));
      /* spi_read can return 0 and it is not error */
      /* TODO: we need to check if SPI interface is available and didn't return error */
      if(size < 0) {
        /* SPI connection has closed */
        YLOG_ERROR("*** SPI connection closed.\n");
        for(i = 5; i > 0; i--) {
          YLOG_ERROR("*** exit in %d seconds\n", i);
          sleep(1);
        }
        exit(EXIT_FAILURE);
      }
      if(size) {
        bufsize[write_buf] = size;
        write_buf = (write_buf + 1) % SLIP_BUF_COUNT;
        process_poll(&serial_input_process);

        if(write_buf == read_buf) {
          BRM_STATS_DEBUG_INC(BRM_STATS_DEBUG_SLIP_DROPPED);
          LOG_LIMIT_ERROR("*** reader has not read... overwriting read buffer! (%u)\n",
                          BRM_STATS_DEBUG_GET(BRM_STATS_DEBUG_SLIP_DROPPED));
        }
        /* printf(" read %d on serial wb:%d rb:%d\n", size, write_buf, read_buf); */
      }
    }
  } else {
    YLOG_ERROR("**** reader thread exiting - slipfd not initialized... \n");
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
spi_process_rx_buf(unsigned char *buf, int len)
{
  int real_len = buf[0] & 0x0f;
  int i;

  /* check if received frame is correct */
  if(((buf[0] & 0xf0) != 0x50) || (real_len > 7)) {
    spi_reset_request = 1;
    return;
  }

  /* put recived data into ringbuf */
  for (i = 0; i < real_len; i++) {
    ringbuf_put(&spi_rx_buf, buf[i+1]);
  }
}
/*---------------------------------------------------------------------------*/
int
spi_write(int fd, unsigned char *buf,  int len)
{
  int len_to_go = len;
  int i = 0, j = 0;
  unsigned char txbuf[8];  
  unsigned char rxbuf[8];

  while (len_to_go > 0) {
    /* if data to send is longer than SPI buffer, we have to split data into 7 byte long chunks */
    txbuf[0] = len_to_go > 7 ? 7 : len_to_go;
    txbuf[0] |= 0x50;

    /* tx_buf[0] is the frame header, so we iterate from 1 */
    for(i = 1; i <= 7; i++) {
      txbuf[i] = buf[j++];
    }
    len_to_go -= 7;

    if(spi_transfer(fd, rxbuf, txbuf, 8) < 0)
      return -1;
    /* during the SPI send, we also receive data from radio */
    spi_process_rx_buf(rxbuf, 8);
  }

  return len;
}
/*---------------------------------------------------------------------------*/
void
spi_flushbuf(int fd)
{
  /* Ensure the slip buffer will be large enough for worst case encoding */
  static uint8_t slip_buf[PACKET_MAX_SIZE * 2];
  static uint16_t slip_begin, slip_end = 0;

  int i, n;

  if(active_packet == NULL) {

    if(slip_empty()) {
      /* Nothing to send */
      return;
    }

    active_packet = list_pop(pending_packets);
    if(active_packet == NULL) {
      /* Nothing to send */
      return;
    }

    slip_begin = slip_end = 0;

    slip_buf[slip_end++] = SLIP_END;
    for(i = 0; i < active_packet->len; i++) {
      switch(active_packet->data[i]) {
      case SLIP_END:
        slip_buf[slip_end++] = SLIP_ESC;
        slip_buf[slip_end++] = SLIP_ESC_END;
        break;
      case SLIP_ESC:
        slip_buf[slip_end++] = SLIP_ESC;
        slip_buf[slip_end++] = SLIP_ESC_ESC;
        break;
      default:
        slip_buf[slip_end++] = active_packet->data[i];
        break;
      }
    }
    slip_buf[slip_end++] = SLIP_END;

    if(verbose_output) {
      PRINTF("send %u/%u\n", slip_end, active_packet->len);
    }
  }

  n = spi_write(fd, slip_buf + slip_begin, slip_end - slip_begin);

  if(n == -1) {
    if(errno == EAGAIN) {
      PROGRESS("Q");    /* Outqueue is full! */
    } else {
      err(1, "slip_flushbuf write failed");
    }
  } else {

    slip_sent_to_fd += n;
    slip_begin += n;
    if(slip_begin == slip_end) {
      slip_begin = slip_end = 0;
      slip_sent++;

      if(active_packet != NULL) {
        /* This packet is no longer needed */
        free_packet(active_packet);

        /* a delay between non acked slip packets to avoid losing data */
        if(send_delay != 0) {
          ctimer_restart(&send_delay_timer);
        }

        active_packet = NULL;
      }
    }
  }
}
#endif /* WITH_SPI */
/*---------------------------------------------------------------------------*/
void
slip_flushbuf(int fd)
{
  /* Ensure the slip buffer will be large enough for worst case encoding */
  static uint8_t slip_buf[PACKET_MAX_SIZE * 2];
  static uint16_t slip_begin, slip_end = 0;

  int i, n;

  if(active_packet == NULL) {

    if(slip_empty()) {
      /* Nothing to send */
      return;
    }

    active_packet = list_pop(pending_packets);
    if(active_packet == NULL) {
      /* Nothing to send */
      return;
    }

    slip_begin = slip_end = 0;

    slip_buf[slip_end++] = SLIP_END;
    for(i = 0; i < active_packet->len; i++) {
      switch(active_packet->data[i]) {
      case SLIP_END:
        slip_buf[slip_end++] = SLIP_ESC;
        slip_buf[slip_end++] = SLIP_ESC_END;
        break;
      case SLIP_ESC:
        slip_buf[slip_end++] = SLIP_ESC;
        slip_buf[slip_end++] = SLIP_ESC_ESC;
        break;
      default:
        slip_buf[slip_end++] = active_packet->data[i];
        break;
      }
    }
    slip_buf[slip_end++] = SLIP_END;

    if(verbose_output) {
      PRINTF("send %u/%u\n", slip_end, active_packet->len);
    }
  }

  n = write(fd, slip_buf + slip_begin, slip_end - slip_begin);

  if(n == -1) {
    if(errno == EAGAIN) {
      PROGRESS("Q");		/* Outqueue is full! */
    } else {
      err(1, "slip_flushbuf write failed");
    }
  } else {

    slip_sent_to_fd += n;
    slip_begin += n;
    if(slip_begin == slip_end) {
      slip_begin = slip_end = 0;
      slip_sent++;

      if(active_packet != NULL) {
        /* This packet is no longer needed */
        free_packet(active_packet);

        /* a delay between non acked slip packets to avoid losing data */
        if(send_delay != 0) {
          ctimer_restart(&send_delay_timer);
        }

        active_packet = NULL;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
write_to_serial(int outfd, const uint8_t *inbuf, int len, uint8_t payload_type)
{
  const uint8_t *p = inbuf;
  uint8_t *buffer;
  uint8_t finger[4];
  int enc_res;
  int i;
  crc crc_value;
  pdu_info pinfo;
  packet_t *packet;

  if(tunnel) {
    /* In tunnel mode - only the tunnel can write to serial */
    return;
  }

  if(slip_config_verbose > 2) {
#ifdef __CYGWIN__
    printf("Packet from WPCAP of length %d - write SLIP\n", len);
#else
    printf("Packet from TUN of length %d - write SLIP\n", len);
#endif
    if(slip_config_verbose > 4) {
#if WIRESHARK_IMPORT_FORMAT
      printf("0000");
      for(i = 0; i < len; i++) printf(" %02x", p[i]);
#else
      printf("         ");
      for(i = 0; i < len; i++) {
        printf("%02x", p[i]);
        if((i & 3) == 3) printf(" ");
        if((i & 15) == 15) printf("\n         ");
      }
#endif
      printf("\n");
    }
  }

  packet = alloc_packet();
  if(packet == NULL) {
    /* alloc_packet will log the overflow */
    /* fprintf(stderr, "*** packets overflow\n"); */
    /* BRM_STATS_DEBUG_INC(BRM_STATS_DEBUG_SLIP_OVERFLOWS); */
    return;
  }

  buffer = packet->data;

  /* do encap header + CRC32 here!!! */
  
  finger[0] = 0;
  finger[1] = ENC_FINGERPRINT_LENOPT_OPTION_CRC;
  finger[2] = (len >> 8);
  finger[3] = len & 0xff;

  pinfo.version = ENC_VERSION1;
  pinfo.fp = finger;
  pinfo.iv = NULL;
  pinfo.payload_len = len;
  pinfo.payload_type = payload_type;
  pinfo.fpmode = ENC_FINGERPRINT_MODE_LENOPT;
  pinfo.fplen = 4;
  pinfo.ivmode = ENC_IVMODE_NONE;
  pinfo.ivlen = 0;
  pinfo.encrypt = FALSE;

  enc_res = write_encap_header(buffer, PACKET_MAX_SIZE, &pinfo, ENC_ERROR_OK);

  if(enc_res) {

    /* copy the data into the buffer */
    memcpy(buffer + enc_res, p, len);

    /* do a crc32 calculation of the whole message */
    crc_value = crcFast(buffer, len + enc_res);

    buffer[len + enc_res + 0] = (crc_value >> 0L) & 0xff;
    buffer[len + enc_res + 1] = (crc_value >> 8L) & 0xff;
    buffer[len + enc_res + 2] = (crc_value >> 16L) & 0xff;
    buffer[len + enc_res + 3] = (crc_value >> 24L) & 0xff;

    packet->len = len + enc_res + 4;

    list_add(pending_packets, packet);
  } else {
    free_packet(packet);
  }
  PROGRESS("t");
}
/*---------------------------------------------------------------------------*/
/* writes an 802.15.4 packet to slip-radio */
void
write_to_slip(const uint8_t *buf, int len)
{
  if(slipfd > 0) {
    write_to_serial(slipfd, buf, len, ENC_PAYLOAD_SERIAL);
  }
}
/*---------------------------------------------------------------------------*/
void
write_to_slip_payload_type(const uint8_t *buf, int len, uint8_t payload_type)
{
  if(slipfd > 0) {
    write_to_serial(slipfd, buf, len, payload_type);
  }
}
/*---------------------------------------------------------------------------*/
static void
stty_telos(int fd)
{
  struct termios tty;
  speed_t speed = slip_config_b_rate;
  int i;

  if(tcflush(fd, TCIOFLUSH) == -1) err(1, "tcflush");

  if(fcntl(fd, F_SETFL, 0) == -1) err(1, "fcntl");

  if(tcgetattr(fd, &tty) == -1) err(1, "tcgetattr");

  cfmakeraw(&tty);

  /* Nonblocking read. */
  /* tty.c_cc[VTIME] = 0; */
  /* tty.c_cc[VMIN] = 0; */
  if(slip_config_flowcontrol) {
    tty.c_cflag |= CRTSCTS;
  } else {
    tty.c_cflag &= ~CRTSCTS;
  }
  tty.c_cflag &= ~HUPCL;
  tty.c_cflag &= ~CLOCAL;

  cfsetispeed(&tty, speed);
  cfsetospeed(&tty, speed);

  if(tcsetattr(fd, TCSAFLUSH, &tty) == -1) err(1, "tcsetattr");

#if 1
  /* Nonblocking read and write. */
  /* if(fcntl(fd, F_SETFL, O_NONBLOCK) == -1) err(1, "fcntl"); */

  tty.c_cflag |= CLOCAL;
  if(tcsetattr(fd, TCSAFLUSH, &tty) == -1) err(1, "tcsetattr");

  i = TIOCM_DTR;
  if(ioctl(fd, TIOCMBIS, &i) == -1) err(1, "ioctl");
#endif

  usleep(10*1000);		/* Wait for hardware 10ms. */

  /* Flush input and output buffers. */
  if(tcflush(fd, TCIOFLUSH) == -1) err(1, "tcflush");
}
/*---------------------------------------------------------------------------*/
static int
set_fd(fd_set *rset, fd_set *wset)
{
  if(tunnel != NULL) {
    if(tunnel->has_output && tunnel->has_output()) {
      FD_SET(slipfd, wset);
    }
    return 1;
  }

  /* Anything to flush? */
  if(!slip_empty()
     && (send_delay == 0 || ctimer_expired(&send_delay_timer))) {
    FD_SET(slipfd, wset);
  }

  //FD_SET(slipfd, rset);	/* Read from slip ASAP! */
  return 1;
}
/*---------------------------------------------------------------------------*/
#if WITH_SPI
static void
handle_spi_fd(fd_set *rset, fd_set *wset)
{
  int ret, i;

  if(slipfd > 0 && FD_ISSET(slipfd, rset)) {
  }

  if(slipfd > 0 && FD_ISSET(slipfd, wset)) {
    if(tunnel != NULL) {
      /* TODO: tunnel functionality not tested with SPI */
      if(tunnel->output) {
        ret = tunnel->output(slipfd);
        if(ret == 0) {
          /* Serial connection has closed */
          YLOG_ERROR("*** SPI connection closed.\n");
          for(i = 5; i > 0; i--) {
            YLOG_ERROR("*** exit in %d seconds\n", i);
            sleep(1);
          }
          exit(EXIT_FAILURE);
        } else if(ret < 0) {
          if(errno != EINTR && errno!= EAGAIN) {
            err(1, "enc-dev: write tunnel");
          }
        }
      }
    } else {
      spi_flushbuf(slipfd);
    }
  }
}
#endif /* WITH_SPI */
/*---------------------------------------------------------------------------*/
static void
handle_fd(fd_set *rset, fd_set *wset)
{
  int ret, i;

  if(slipfd > 0 && FD_ISSET(slipfd, rset)) {
  }

  if(slipfd > 0 && FD_ISSET(slipfd, wset)) {
    if(tunnel != NULL) {
      if(tunnel->output) {
        ret = tunnel->output(slipfd);
        if(ret == 0) {
          /* Serial connection has closed */
          YLOG_ERROR("*** serial connection closed.\n");
          for(i = 5; i > 0; i--) {
            YLOG_ERROR("*** exit in %d seconds\n", i);
            sleep(1);
          }
          exit(EXIT_FAILURE);
        } else if(ret < 0) {
          if(errno != EINTR && errno!= EAGAIN) {
            err(1, "enc-dev: write tunnel");
          }
        }
      }
    } else {
      slip_flushbuf(slipfd);
    }
  }

  //  serial_input();
}

/*---------------------------------------------------------------------------*/
#if WITH_SPI
static const struct select_callback slip_spi_callback = { set_fd, handle_spi_fd };
#endif /* WITH_SPI */
static const struct select_callback slip_callback = { set_fd, handle_fd };
/*---------------------------------------------------------------------------*/
void
slip_init(void)
{
  static uint8_t is_initialized = 0;
  int rc;

  if(is_initialized) {
    return;
  }
  is_initialized = 1;

  memb_init(&packet_memb);
  list_init(pending_packets);

  setvbuf(stdout, NULL, _IOLBF, 0); /* Line buffered output. */

  if(slip_config_host != NULL) {
    if(slip_config_port == NULL) {
      slip_config_port = "60001";
    }
    slipfd = connect_to_server(slip_config_host, slip_config_port);
    if(slipfd == -1) {
      err(1, "can't connect to ``%s:%s''", slip_config_host, slip_config_port);
    }

  } else if(slip_config_siodev != NULL) {
    if(strcmp(slip_config_siodev, "null") == 0) {
      /* Disable slip */
      return;
    }
#if WITH_SPI
    if (strncmp(slip_config_siodev, "spidev", 6) == 0) {
      gpio_init();
      slipfd = spi_init(slip_config_siodev);
    } else
#endif /* WITH_SPI */
    {
      slipfd = devopen(slip_config_siodev, O_RDWR | O_NDELAY);
    }
    if(slipfd == -1) {
      err(1, "can't open siodev ``/dev/%s''", slip_config_siodev);
    }

  } else {
    static const char *siodevs[] = {
      "ttyUSB0", "ttyACM0", "cuaU0", "ucom0" /* linux, fbsd6, fbsd5 */
    };
    int i;
    for(i = 0; i < 3; i++) {
      slip_config_siodev = siodevs[i];
      slipfd = devopen(slip_config_siodev, O_RDWR | O_NDELAY);
      if(slipfd != -1) {
	break;
      }
    }
    if(slipfd == -1) {
      err(1, "can't open siodev");
    }
  }

  /* will only be for the write */
#if WITH_SPI
  if (strncmp(slip_config_siodev, "spidev", 6) == 0) {
    select_set_callback(slipfd, &slip_spi_callback);
  } else
#endif /* WITH_SPI */
  {
    select_set_callback(slipfd, &slip_callback);
  }

  if(slip_config_host != NULL) {
    YLOG_INFO("********SLIP opened to ``%s:%s''\n", slip_config_host,
	    slip_config_port);
  } else {
    YLOG_INFO("********SLIP started on ``/dev/%s''\n", slip_config_siodev);
#if WITH_SPI
    /* SPI doesn't support telos */
    if (strncmp(slip_config_siodev, "spidev", 6) != 0) {
      stty_telos(slipfd);
    }
#else /* WITH_SPI */
    stty_telos(slipfd);
#endif /* WITH_SPI */
  }

  serial_set_send_delay(SEND_DELAY_DEFAULT);

#if WITH_SPI
  if (strncmp(slip_config_siodev, "spidev", 6) == 0) {
    rc = pthread_create(&thread, NULL, spi_read_code, NULL);
  } else
#endif /* WITH_SPI */
  {
    rc = pthread_create(&thread, NULL, read_code, NULL);
  }
  if(rc) {
    YLOG_ERROR("failed to start the serial reader thread: %d\n", rc);
    exit(EXIT_FAILURE);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS(serial_input_process, "Input Process");

PROCESS_THREAD(serial_input_process, ev, data)
{
  PROCESS_BEGIN();
  while(1) {
    PROCESS_WAIT_UNTIL(write_buf != read_buf);
    serial_input();
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*
 * Copyright (c) 2001, Adam Dunkels.
 * Copyright (c) 2009, 2010 Joakim Eriksson, Niclas Finne, Dogan Yazar.
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

#include "net/netstack.h"
#include "net/packetbuf.h"
#include "cmd.h"
#include "border-router-cmds.h"

#include <linux/spi/spidev.h>
#include "lib/ringbuf.h"

extern int slip_config_verbose;
extern int slip_config_flowcontrol;
extern const char *slip_config_siodev;
extern const char *slip_config_host;
extern const char *slip_config_port;
extern uint16_t slip_config_basedelay;
extern speed_t slip_config_b_rate;

#ifdef SLIP_DEV_CONF_SEND_DELAY
#define SEND_DELAY SLIP_DEV_CONF_SEND_DELAY
#else
#define SEND_DELAY 0
#endif

int devopen(const char *dev, int flags);

static FILE *inslip;

/* for statistics */
long slip_sent = 0;
long slip_received = 0;

int slipfd = 0;

//#define PROGRESS(s) fprintf(stderr, s)
#define PROGRESS(s) do { } while(0)

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

/*---------------------------------------------------------------------------*/
static struct ringbuf spi_rx_buf;
static uint8_t rxbuf_data[128];

int spi_init(const char* device)
{
        int fd;
        uint8_t mode, lsb, bits;
        uint32_t speed;


    char dvnm[255];
    sprintf(dvnm, "/dev/%s", device);
    fd = open(dvnm, O_RDWR);
    if(fd < 0)
    {
        printf("Failed to open device %s. Reason: %s\n", device, strerror(errno));
        return 0;
    }

    /* Get current config */
    if (ioctl(fd, SPI_IOC_RD_MODE, &mode) < 0)
    {
        printf("Failed to get SPI mode. Reason: %s\n", strerror(errno));
        return 0;
    }
    mode = 3;
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0)
    {
        printf("Failed to get SPI mode. Reason: %s\n", strerror(errno));
        return 0;
    }
    if (ioctl(fd, SPI_IOC_RD_LSB_FIRST, &lsb) < 0)
    {
        printf("Failed to get SPI endianness. Reason: %s\n", strerror(errno));
        return 0;
    }
    if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0)
    {
        printf("Failed to get SPI bits number. Reason: %s\n", strerror(errno));
        return 0;
    }
    if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0)
    {
        printf("Failed to get maximal bus speed. Reason: %s\n", strerror(errno));
        return 0;
    }

    ringbuf_init(&spi_rx_buf, rxbuf_data, sizeof(rxbuf_data));

    return fd;
}

int spi_transfer(int device, char* rxbuf, char *txbuf, int len)
{
    int status;

    struct spi_ioc_transfer xfer[1] = {0};

    xfer[0].cs_change = 0; /* Keep CS activated */
    xfer[0].delay_usecs = 0; /* delay in us */
    xfer[0].speed_hz = 1000000; /* speed */
    xfer[0].bits_per_word = 8; /* bites per word 8 */
    xfer[0].tx_buf = (unsigned long)txbuf;
    xfer[0].rx_buf = (unsigned long)rxbuf;
    xfer[0].len = len;

    status = ioctl(device, SPI_IOC_MESSAGE(1), xfer);
    if(status < 0)
    {
        printf("Error reading SPI message. Reason: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static void *
get_in_addr(struct sockaddr *sa)
{
  if(sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
/*---------------------------------------------------------------------------*/
static int
connect_to_server(const char *host, const char *port)
{
  /* Setup TCP connection */
  struct addrinfo hints, *servinfo, *p;
  char s[INET6_ADDRSTRLEN];
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
    return -1;
  }

  fcntl(fd, F_SETFL, O_NONBLOCK);

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
	    s, sizeof(s));

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
void
slip_packet_input(unsigned char *data, int len)
{
  packetbuf_copyfrom(data, len);
  if(slip_config_verbose > 0) {
    printf("Packet input over SLIP: %d\n", len);
  }
  NETSTACK_RDC.input();
}

void spi_input(FILE *inslip) {
	while (spi_schedule_read(inslip, 8) > 0) {
		// nothing
	}
	serial_input(NULL);
}



/*---------------------------------------------------------------------------*/
/*
 * Read from serial, when we have a packet call slip_packet_input. No output
 * buffering, input buffered by stdio.
 */
void
serial_input(FILE *inslip)
{
  static unsigned char inbuf[2048];
  static int inbufptr = 0;
  int ret,i;
  unsigned char c;
#ifdef linux
  if (inslip == NULL) {
	ret = ringbuf_get(&spi_rx_buf); if (ret >= 0) c = ret;
	if (ret < 0) return;
        ret = 1;
  } else {
   ret = fread(&c, 1, 1, inslip);
}
  if(ret == -1 || ret == 0) err(1, "serial_input: read");
  goto after_fread;
#endif

 read_more:
  if(inbufptr >= sizeof(inbuf)) {
     fprintf(stderr, "*** dropping large %d byte packet\n", inbufptr);
     inbufptr = 0;
  }
if (inslip == NULL) {
    ret = ringbuf_get(&spi_rx_buf); if (ret >= 0) c = ret;
    if (ret < 0) {
	// TODO
	//ringbuf_put(&spi_rx_buf, c);
	return;
    }
    ret = 1;
} else {
  ret = fread(&c, 1, 1, inslip);
}
#ifdef linux
 after_fread:
#endif
  if(ret == -1) {
   if (inslip == NULL) {
	return;
   } else {
    err(1, "serial_input: read");
 }
  }
  if(ret == 0) {
	if (inslip != NULL)
	    clearerr(inslip);
    return;
  }
  slip_received++;
  switch(c) {
  case SLIP_END:
    if(inbufptr > 0) {
      if(inbuf[0] == '!') {
	command_context = CMD_CONTEXT_RADIO;
	cmd_input(inbuf, inbufptr);
      } else if(inbuf[0] == '?') {
#define DEBUG_LINE_MARKER '\r'
      } else if(inbuf[0] == DEBUG_LINE_MARKER) {
	fwrite(inbuf + 1, inbufptr - 1, 1, stdout);
      } else if(is_sensible_string(inbuf, inbufptr)) {
        if(slip_config_verbose == 1) {   /* strings already echoed below for verbose>1 */
          fwrite(inbuf, inbufptr, 1, stdout);
        }
      } else {
        if(slip_config_verbose > 2) {
          printf("Packet from SLIP of length %d - write TUN\n", inbufptr);
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
	slip_packet_input(inbuf, inbufptr);
      }
      inbufptr = 0;
    }
    break;

  case SLIP_ESC:
    if (inslip != NULL) {
    if(fread(&c, 1, 1, inslip) != 1) {
      clearerr(inslip);
      /* Put ESC back and give up! */
      ungetc(SLIP_ESC, inslip);
      return;
    }
} else {
	printf("hmm, some kind of escape ?\n");
	// TODO:
	return;
}

    switch(c) {
    case SLIP_ESC_END:
      c = SLIP_END;
      break;
    case SLIP_ESC_ESC:
      c = SLIP_ESC;
      break;
    }
    /* FALLTHROUGH */
  default:
    inbuf[inbufptr++] = c;

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

  goto read_more;
}

unsigned char slip_buf[2048];
int slip_end, slip_begin, slip_packet_end, slip_packet_count;
static struct timer send_delay_timer;
/* delay between slip packets */
static clock_time_t send_delay = SEND_DELAY;
/*---------------------------------------------------------------------------*/
static void
slip_send(int fd, unsigned char c)
{
  if(slip_end >= sizeof(slip_buf)) {
    err(1, "slip_send overflow");
  }
  slip_buf[slip_end] = c;
  slip_end++;
  slip_sent++;
  if(c == SLIP_END) {
    /* Full packet received. */
    slip_packet_count++;
    if(slip_packet_end == 0) {
      slip_packet_end = slip_end;
    }
  }
}
/*---------------------------------------------------------------------------*/
int
slip_empty()
{
  return slip_packet_end == 0;
}
/*---------------------------------------------------------------------------*/

#define GPIOS_NUM 5

typedef struct gpio_s
{
    int number;
    char* direction;
    int value;
    char* drive;
} gpio_t;

gpio_t gpios[GPIOS_NUM] = {
    {4, "out", 1, ""},
    {42, "out", 0, "strong"},
    {43, "out", 0, "strong"},
    {54, "out", 0, "strong"},
    {55, "out", 0, "strong"},
};

void gpio_init(void)
{
    int fd;
    int i;
    char buf[64];

    fd = open("/sys/class/gpio/export", O_WRONLY);
    for(i=0; i<GPIOS_NUM; i++)
    {
        sprintf(buf, "%d", gpios[i].number);
        write(fd, buf, strlen(buf));
    }
    close(fd);

    for(i=0; i<GPIOS_NUM; i++)
    {
        sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpios[i].number);
        fd = open(buf, O_WRONLY);
        write(fd, gpios[i].direction, strlen(gpios[i].direction));
        close(fd);

        sprintf(buf, "/sys/class/gpio/gpio%d/value", gpios[i].number);
        fd = open(buf, O_WRONLY);
        sprintf(buf, "%d", gpios[i].value);
        write(fd, buf, strlen(buf));
        close(fd);

        if(strlen(gpios[i].drive))
        {
            sprintf(buf, "/sys/class/gpio/gpio%d/drive", gpios[i].number);
            fd = open(buf, O_WRONLY);
            write(fd, gpios[i].drive, strlen(gpios[i].drive));
            close(fd);
        }
    }
}

void feed_spi(unsigned char *buf, int len) {
        if (buf[3] != 0xAA) return;
	if (buf[4] == 0) return;
	int real_len = buf[4];
//	printf("received some data len=%d: ",real_len);
	int i;
	for (i = 5; i < 5+real_len; i++) {
		ringbuf_put(&spi_rx_buf, buf[i]);
//		printf("%02X ", buf[i]);
	}
//	printf("\n");
}

void spi_write(int fd, char *buf,  int len) {
    int len_to_go = len;
    int i= 0;
    char txbuf[8];
    char rxbuf[8];
    while (len_to_go > 0) {
	    txbuf[0] = 0x55;
	    txbuf[1] = 0xab;
	    txbuf[2] = len_to_go > 3 ? 3 : len_to_go;
	    txbuf[3] = 0;
	    txbuf[4] = 0;
	    txbuf[5] = buf[i++];
	    txbuf[6] = buf[i++];
	    txbuf[7] = buf[i++];
	    len_to_go -= 3;
	    spi_transfer(fd, rxbuf, txbuf, 8);
	    feed_spi(rxbuf, 8);
	    usleep(10000);
    }
}


int spi_schedule_read(int fd, int max_len) {
    int len_to_go = max_len;
    int i= 0;
    char txbuf[8];
    char rxbuf[8];
    while (len_to_go > 0) {
            txbuf[0] = 0x55;
            txbuf[1] = 0xaa;
            txbuf[2] = 3;
            txbuf[3] = 0;
            txbuf[4] = 0;
            txbuf[5] = 0xFF;
            txbuf[6] = 0xFF;
            txbuf[7] = 0xFF;
            len_to_go -= 3;
            spi_transfer(fd, rxbuf, txbuf, 8);
            feed_spi(rxbuf, 8);
            usleep(10000);
    }
    return rxbuf[3];
}


void spi_flushbuf(int fd) {
  int n;

  if(slip_empty()) {
    return;
  }

  spi_write(fd, slip_buf + slip_begin, slip_packet_end - slip_begin);
  n = slip_packet_end - slip_begin;

  slip_begin += n;
  if(slip_begin == slip_packet_end) {
      slip_packet_count--;
      if(slip_end > slip_packet_end) {
        memcpy(slip_buf, slip_buf + slip_packet_end,
               slip_end - slip_packet_end);
      }
      slip_end -= slip_packet_end;
      slip_begin = slip_packet_end = 0;
      if(slip_end > 0) {
        /* Find end of next slip packet */
        for(n = 1; n < slip_end; n++) {
          if(slip_buf[n] == SLIP_END) {
            slip_packet_end = n + 1;
            break;
          }
        }
        /* a delay between slip packets to avoid losing data */
        if(send_delay > 0) {
          timer_set(&send_delay_timer, send_delay);
        }
      }
    }
}

void
slip_flushbuf(int fd)
{
  int n;

  if(slip_empty()) {
    return;
  }

  n = write(fd, slip_buf + slip_begin, slip_packet_end - slip_begin);

  if(n == -1 && errno != EAGAIN) {
    err(1, "slip_flushbuf write failed");
  } else if(n == -1) {
    PROGRESS("Q");		/* Outqueue is full! */
  } else {
    slip_begin += n;
    if(slip_begin == slip_packet_end) {
      slip_packet_count--;
      if(slip_end > slip_packet_end) {
        memcpy(slip_buf, slip_buf + slip_packet_end,
               slip_end - slip_packet_end);
      }
      slip_end -= slip_packet_end;
      slip_begin = slip_packet_end = 0;
      if(slip_end > 0) {
        /* Find end of next slip packet */
        for(n = 1; n < slip_end; n++) {
          if(slip_buf[n] == SLIP_END) {
            slip_packet_end = n + 1;
            break;
          }
        }
        /* a delay between slip packets to avoid losing data */
        if(send_delay > 0) {
          timer_set(&send_delay_timer, send_delay);
        }
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
write_to_serial(int outfd, const uint8_t *inbuf, int len)
{
  const uint8_t *p = inbuf;
  int i;
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

  /* It would be ``nice'' to send a SLIP_END here but it's not
   * really necessary.
   */
  /* slip_send(outfd, SLIP_END); */

  for(i = 0; i < len; i++) {
    switch(p[i]) {
    case SLIP_END:
      slip_send(outfd, SLIP_ESC);
      slip_send(outfd, SLIP_ESC_END);
      break;
    case SLIP_ESC:
      slip_send(outfd, SLIP_ESC);
      slip_send(outfd, SLIP_ESC_ESC);
      break;
    default:
      slip_send(outfd, p[i]);
      break;
    }
  }
  slip_send(outfd, SLIP_END);
  PROGRESS("t");
}
/*---------------------------------------------------------------------------*/
/* writes an 802.15.4 packet to slip-radio */
void
write_to_slip(const uint8_t *buf, int len)
{
printf("WRITE TO SLIP, slipfd = %d len=%d\n", slipfd,len);
  if(slipfd > 0) {
    write_to_serial(slipfd, buf, len);
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

  if(tcgetattr(fd, &tty) == -1) err(1, "tcgetattr");

  cfmakeraw(&tty);

  /* Nonblocking read. */
  tty.c_cc[VTIME] = 0;
  tty.c_cc[VMIN] = 0;
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

//  i = TIOCM_DTR;
//  if(ioctl(fd, TIOCMBIS, &i) == -1) err(1, "ioctl");
#endif

  usleep(10*1000);		/* Wait for hardware 10ms. */

  /* Flush input and output buffers. */
  if(tcflush(fd, TCIOFLUSH) == -1) err(1, "tcflush");
}
/*---------------------------------------------------------------------------*/
static int
set_fd(fd_set *rset, fd_set *wset)
{
  /* Anything to flush? */
  if(!slip_empty() && (send_delay == 0 || timer_expired(&send_delay_timer))) {
    FD_SET(slipfd, wset);
  }

  FD_SET(slipfd, rset);	/* Read from slip ASAP! */
  return 1;
}

static handle_spi_fd(fd_set *rset, fd_set *wset) {
  if(FD_ISSET(slipfd, rset)) {
    spi_input(slipfd);
  }

  if(FD_ISSET(slipfd, wset)) {
    spi_flushbuf(slipfd);
  }
}




/*---------------------------------------------------------------------------*/
static void
handle_fd(fd_set *rset, fd_set *wset)
{
  if(FD_ISSET(slipfd, rset)) {
    serial_input(inslip);
  }

  if(FD_ISSET(slipfd, wset)) {
    slip_flushbuf(slipfd);
  }
}
/*---------------------------------------------------------------------------*/
static const struct select_callback slip_spi_callback = { set_fd, handle_spi_fd };
static const struct select_callback slip_callback = { set_fd, handle_fd };
/*---------------------------------------------------------------------------*/
void
slip_init(void)
{
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
    if (strncmp(slip_config_siodev, "spidev", 6) == 0) {
      // spi
      fprintf(stderr, "****** SLIP SPIDEV %s\n", slip_config_siodev);
      //fprintf(stderr, "Setting up GPIO...\n");
      //gpio_init();
      //fprintf(stderr, "Ready!\n");
      slipfd = spi_init(slip_config_siodev);
    }
    if(slipfd == -1) {
      err(1, "can't open siodev ``/dev/%s''", slip_config_siodev);
    }
  } else {
    static const char *siodevs[] = {
      "ttyUSB0", "cuaU0", "ucom0" /* linux, fbsd6, fbsd5 */
    };
    int i;
    for(i = 0; i < 3; i++) {
      slip_config_siodev = siodevs[i];
      slipfd = devopen(slip_config_siodev, O_RDWR | O_NONBLOCK);
      if(slipfd != -1) {
	break;
      }
    }
    if(slipfd == -1) {
      err(1, "can't open siodev");
    }
  }
 if (strncmp(slip_config_siodev, "spidev", 6) == 0) {
  select_set_callback(slipfd, &slip_spi_callback);
} else
  select_set_callback(slipfd, &slip_callback);

  if(slip_config_host != NULL) {
    fprintf(stderr, "********SLIP opened to ``%s:%s''\n", slip_config_host,
	    slip_config_port);
  } else {
    fprintf(stderr, "********SLIP started on ``/dev/%s''\n", slip_config_siodev);
    if (strncmp(slip_config_siodev, "spidev", 6) != 0)
	    stty_telos(slipfd);
  }

  timer_set(&send_delay_timer, 0);
  slip_send(slipfd, SLIP_END);
  inslip = fdopen(slipfd, "r");
  if(inslip == NULL) {
    err(1, "main: fdopen");
  }
}
/*---------------------------------------------------------------------------*/

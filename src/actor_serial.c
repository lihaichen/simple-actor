#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include "actor_common.h"

int config_serial(actor_serial_t* serial,
                  int baudrate,
                  char bits,
                  char parity,
                  char stop) {
  ACTOR_ASSERT(serial != NULL);
  actor_io_t* io = serial->io;
  ACTOR_ASSERT(serial->io != NULL);
  struct termios options;
  if (tcgetattr(io->fd, &options) < 0) {
    ACTOR_PRINT("tcgetattr error %d\n", errno);
    return -1;
  }
  switch (baudrate) {
    case 1200:
      cfsetispeed(&options, B1200);
      cfsetospeed(&options, B1200);
      break;
    case 2400:
      cfsetispeed(&options, B1200);
      cfsetospeed(&options, B1200);
      break;
    case 4800:
      cfsetispeed(&options, B1200);
      cfsetospeed(&options, B1200);
      break;
    case 9600:
      cfsetispeed(&options, B9600);
      cfsetospeed(&options, B9600);
      break;
    case 19200:
      cfsetispeed(&options, B19200);
      cfsetospeed(&options, B19200);
      break;
    case 38400:
      cfsetispeed(&options, B38400);
      cfsetospeed(&options, B38400);
      break;
    case 115200:
      cfsetispeed(&options, B115200);
      cfsetospeed(&options, B115200);
      break;
    default:
      ACTOR_PRINT("baudrate not support\n");
      return -1;
  }
  options.c_cflag |= (CLOCAL | CREAD);
  options.c_cflag &= ~CSIZE;
  switch (bits) {
    case 5:
      options.c_cflag |= CS5;
      break;
    case 6:
      options.c_cflag |= CS6;
      break;
    case 7:
      options.c_cflag |= CS7;
      break;
    case 8:
      options.c_cflag |= CS8;
      break;
    default:
      ACTOR_PRINT("bits not support\n");
      return -1;
  }
  switch (parity) {
    case 'n':
    case 'N':
      options.c_cflag &= ~PARENB;
      break;
    case 'o':
    case 'O':
      options.c_cflag |= PARENB;
      options.c_cflag |= PARODD;
      break;
    case 'e':
    case 'E':
      options.c_cflag |= PARENB;
      options.c_cflag &= ~PARODD;
      break;
    default:
      ACTOR_PRINT("parity not support\n");
      return -1;
  }
  switch (stop) {
    case 1:
      options.c_cflag &= ~CSTOPB;
      break;
    case 2:
      options.c_cflag |= CSTOPB;
      break;
    default:
      ACTOR_PRINT("stop not support\n");
      return -1;
  }
  options.c_oflag = 0;
  options.c_lflag = 0;
  options.c_oflag &=~OPOST;

  options.c_cc[VTIME] = 0;
  options.c_cc[VMIN] = 1;
  tcflush(io->fd, TCIFLUSH);
  return tcsetattr(io->fd, TCSANOW, &options);
}

actor_serial_t* open_serial(char* name,
                            struct actor_context* context,
                            int send_buf_len,
                            int recv_buf_len,
                            int timeout) {
  actor_io_t* io = NULL;
  actor_serial_t* serial = ACTOR_MALLOC(sizeof(*serial));
  ACTOR_ASSERT(serial != NULL);
  memset(serial, 0, sizeof(*serial));

  io = create_io(send_buf_len, recv_buf_len);
  if (io == NULL) {
    ACTOR_FREE(serial);
    return NULL;
  }
  io->timeout = timeout;
  io->context = context;
  io->type = ACTOR_IO_SERIAL;
  serial->io = io;

  io->fd = open(name, O_RDWR | O_NOCTTY);
  if (io->fd < 0) {
    ACTOR_PRINT("open serial error %s %d\n", name, errno);
    delete_io(io);
    ACTOR_FREE(serial);
    return NULL;
  }
  actor_io_fd_add(io);
  return serial;
}

int close_serial(actor_serial_t* serial) {
  ACTOR_ASSERT(serial != NULL);
  actor_io_t* io = serial->io;
  ACTOR_ASSERT(io != NULL);
  actor_io_fd_delete(io);
  if (io->fd > 0) {
    close(io->fd);
  }
  delete_io(io);
  ACTOR_FREE(serial);
  return 0;
}
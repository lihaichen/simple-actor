#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "actor_common.h"

Test(actor_io, serial) {
  int len = 0;
  actor_start(2);
  actor_serial_t* serial = open_serial("/dev/ttyUSB0", NULL, 512, 512, 20);
  cr_assert_not_null(serial, "open_serial null");
  cr_expect_eq(config_serial(serial, 115200, 8, 'n', 1), 0,
               "config serial error");
  const char* s = "This is serial test code";
  len = write(serial->io->fd, s, strlen(s));
  printf("send len %d\n", len);
  ACTOR_MSLEEP(50);
  len = write(serial->io->fd, s, strlen(s));
  printf("send len %d\n", len);
  sleep(10);
  close_serial(serial);
  actor_stop();
  return;
}
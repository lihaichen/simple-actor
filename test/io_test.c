#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "actor_common.h"

Test(actor_io, serial) {
  actor_start(2);
  actor_serial_t* serial = open_serial("/dev/ttyUSB0", NULL, 512, 512, 20);
  cr_assert_not_null(serial, "open_serial null");
  cr_expect_eq(config_serial(serial, 115200, 8, 'n', 1), 0,
               "config serial error");
  const char* s = "This is serial test code";
  write(serial->io->fd,s, strlen(s));
  ACTOR_MSLEEP(30);
  write(serial->io->fd,s, strlen(s));
  sleep(10);
  close_serial(serial);
  actor_stop();
  return;
}
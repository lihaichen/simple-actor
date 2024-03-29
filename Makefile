BUILD ?= build

C_SOURCES += src/actor_mq.c \
src/actor_server.c \
src/actor_start.c \
src/actor_timer.c \
src/actor_io.c \
src/actor_serial.c \
src/actor_pipe.c \
src/actor_dict.c 

TEST_C_SOURCES += test/main.c

UNIT_C_SOURCES += test/dict_test.c
#test/io_test.c \
#test/server_test.c 
#test/mq_test.c  
#test/heap_test.c 


CFLAGS = -g -O2 -Wall

INC += -Iinc

OBJS := ${patsubst %.c, %.o, $(C_SOURCES)}

TEST_OBJS := ${patsubst %.c, %.o, $(TEST_C_SOURCES)}


SLIB = $(BUILD)/libsimple_akka.a
DLIB = $(BUILD)/libsimple_akka.so

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

unit:
	$(CC) $(INC) $(CFLAGS) $(UNIT_C_SOURCES) $(C_SOURCES) -lcriterion -lpthread -o $(BUILD)/$@

test: $(SLIB)
	$(CC) $(INC) $(CFLAGS) $(TEST_C_SOURCES) $(C_SOURCES)  -lpthread -o $(BUILD)/$@

$(DLIB): $(OBJS)
	$(CC) $(INC) $(CFLAGS)  -fPIC -shared  $^ -o $@

$(SLIB): $(OBJS)
	ar -rc $@ $^

lib: $(SLIB) $(DLIB)
	$(MKDIR) $(BUILD)
	
all: lib test
	@(MKDIR) build

clean:
	$(RM) $(OBJS) $(TEST_OBJS)  $(DLIB) $(SLIB) $(BUILD)/test $(BUILD)/unit


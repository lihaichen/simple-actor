BUILD ?= build

C_SOURCES += src/actor_mq.c \
src/actor_server.c \

TEST_C_SOURCES += test/main.c 

CFLAGS = -g -O2 -Wall  

INC += -Iinc

OBJS := ${patsubst %.c, %.o, $(C_SOURCES)}

TEST_OBJS := ${patsubst %.c, %.o, $(TEST_C_SOURCES)}


SLIB = $(BUILD)/simple_akka.a
DLIB = $(BUILD)/simple_akka.so

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

test: $(TEST_OBJS) $(OBJS) 
	$(CC) $(INC) $(CFLAGS) -o $@ 

$(DLIB): $(OBJS)
	$(CC) $(INC) $(CFLAGS)  -fPIC -shared  $^ -o $@ 

$(SLIB): $(OBJS)
	ar -rc $@ $^

lib: $(SLIB) $(DLIB)

all: lib test

clean:
	$(RM) $(OBJS) $(TEST_OBJS)  $(DLIB) $(SLIB)


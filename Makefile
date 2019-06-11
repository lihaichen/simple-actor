BUILD ?= build

C_SOURCES += src/actor_mq.c \
src/actor_server.c \
src/actor_start.c

TEST_C_SOURCES += test/main.c

CFLAGS = -g -O2 -Wall

INC += -Iinc

OBJS := ${patsubst %.c, %.o, $(C_SOURCES)}

TEST_OBJS := ${patsubst %.c, %.o, $(TEST_C_SOURCES)}


SLIB = $(BUILD)/libsimple_akka.a
DLIB = $(BUILD)/libsimple_akka.so

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

test: $(SLIB)
	$(CC) $(INC) $(CFLAGS) $(TEST_C_SOURCES) $(C_SOURCES)  -lpthread -o $(BUILD)/$@
#-lcriterion
$(DLIB): $(OBJS)
	$(CC) $(INC) $(CFLAGS)  -fPIC -shared  $^ -o $@

$(SLIB): $(OBJS)
	ar -rc $@ $^

lib: $(SLIB) $(DLIB)
	$(MKDIR) $(BUILD)
	
all: lib test
	

clean:
	$(RM) $(OBJS) $(TEST_OBJS)  $(DLIB) $(SLIB) $(BUILD)/test


CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS =

EXE = gts

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXE)

.PHONY: all clean

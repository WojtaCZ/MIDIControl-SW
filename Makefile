# Project: MIDI controll
# Author: Vojtěch Vosáhlo
# Date: 30. 7. 2019

name = midicontroll
src = $(wildcard lib/*.c)
obj = $(src:.c=.o)

CC=gcc
CFLAGS= -std=c99 -lreadline -D_SVID_SOURCE

$(name): src/main.c $(obj)
		$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
		rm -f $(obj) $(name)
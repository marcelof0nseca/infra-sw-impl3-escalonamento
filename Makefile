CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LOGIN  = maf
TARGET = scheduler
OBJS   = main.o parser.o scheduler.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o:      main.c task.h parser.h scheduler.h
parser.o:    parser.c parser.h task.h
scheduler.o: scheduler.c scheduler.h task.h

clean:
	rm -f $(OBJS) $(TARGET) rate_$(LOGIN).out edf_$(LOGIN).out

.PHONY: all clean

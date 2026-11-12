CC = x86_64-w64-mingw32-gcc
WINDRES = x86_64-w64-mingw32-windres
CFLAGS = -Os -s -Wall -mwindows
LDFLAGS = -lgdi32 -lwinspool -lcomctl32 -lmsimg32 -limm32 -lmsimg32
SRCDIR = src
OBJDIR = obj
RESDIR = res
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
RESDIR = res
RCOBJ = $(OBJDIR)/buttons.o
TARGET = totem.exe

all: $(TARGET)

$(TARGET): $(OBJS) $(RCOBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(RCOBJ): $(RESDIR)/buttons.rc | $(OBJDIR)
	$(WINDRES) -i $< -o $@ --input-format=rc --output-format=coff

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean

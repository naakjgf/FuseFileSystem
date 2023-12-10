SRCS := $(filter-out bitmap_test.c blocks_test.c slist_test.c, $(wildcard *.c))
OBJS := $(SRCS:.c=.o)
HDRS := $(wildcard *.h)

CFLAGS := -g `pkg-config fuse --cflags`
LDLIBS := `pkg-config fuse --libs`

nufs: $(OBJS)
	gcc $(CFLAGS) -o $@ $^ $(LDLIBS)

bitmap_test: bitmap.o bitmap_test.o
	gcc $(CFLAGS) -o $@ bitmap.o bitmap_test.o $(LDLIBS)

blocks_test: blocks.o blocks_test.o
	gcc $(CFLAGS) -o $@ blocks.o blocks_test.o $(LDLIBS)

slist_test: slist.o slist_test.o
	gcc $(CFLAGS) -o $@ slist.o slist_test.o $(LDLIBS)

%.o: %.c $(HDRS)
	gcc $(CFLAGS) -c -o $@ $<

clean: unmount
	rm -f nufs *.o test.log data.nufs
	rmdir mnt || true

mount: nufs
	mkdir -p mnt || true
	./nufs -s -f mnt data.nufs

unmount:
	fusermount -u mnt || true

test: nufs
	perl test.pl

gdb: nufs
	mkdir -p mnt || true
	gdb --args ./nufs -s -f mnt data.nufs

.PHONY: clean mount unmount gdb


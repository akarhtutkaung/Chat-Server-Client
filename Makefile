CFLAGS 	= -Wall -g
CC		= gcc $(CFLAGS)

PROGRAMS = bl_client bl_server
TEST_PROGRAMS = test_blather.sh test_blather_data.sh test_normalize.awk test_cat_sig.sh test_filter_semopen_bug.awk 

all : $(PROGRAMS)

bl_server : bl_server.c server_funcs.c
	$(CC) -o $@ $^
 
bl_client : bl_client.c simpio.c -lpthread
	$(CC) -o $@ $^ 

test : bl_client bl_server
	chmod u+rx test_*
	./test_blather.sh $(testnum)

clean-tests :
	cd test-data && \
	rm -f *.* 

clean :
	rm -f *.o *.fifo $(PROGRAMS)
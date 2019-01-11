/* file : gbd-text-display.c
 * desc : illustrates GBD Linux POSIX SHM IPC for beat counts
 *
 * build: gcc -Wall -O2 gbd-text-display.c -o gbd-text-display -lrt
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "gbd.h"

static void *shm_init(const char *filename)
{
	int fd, ret = -1;
	void *lmap = NULL;
	size_t shm_filesize;

	fd = shm_open(filename, O_RDWR | O_CREAT, (mode_t) 0666);
	if(fd < 0){
     fprintf(stderr, "shm_open(3): %s\n", strerror(errno));
     goto exit;
	}

	shm_filesize = sysconf(_SC_PAGE_SIZE);
	ret = ftruncate(fd, shm_filesize);
	if (ret < 0) {
		fprintf(stderr, "ftruncate(2): %sn", strerror(errno));
		goto exit;
	}	
  
	lmap = mmap(0, shm_filesize, PROT_READ | PROT_WRITE, 
			       MAP_SHARED, fd, 0);
	if(lmap	== MAP_FAILED) {
  	fprintf(stderr, "mmap(2): %s\n", strerror(errno));
		goto exit;
	}

	return lmap;
exit:
	close(fd);
	return NULL;
}


int main(void)
{
	int *beat_cnt_map;
	beat_cnt_map =  (int *)shm_init(GBD_BEAT_COUNT_FILE);
	if (!beat_cnt_map) {
		fprintf(stderr, "Could not open GBD IPC file!\n");
		return -1;
	}
	
	for (;;) {

		static int bcnt, scnt, ccnt, __attribute__((unused)) blcnt;
		static int prevcnt[GBD_BEAT_COUNT_BUF_SIZE];

		usleep(10000); /* 10ms or 100Hz */
	
		if (beat_cnt_map[KICKDRUM] != prevcnt[KICKDRUM]) {
			prevcnt[KICKDRUM] = beat_cnt_map[KICKDRUM];
			printf("BassBeat (%i)\n", bcnt++);
		}
		
		if (beat_cnt_map[SNARE] != prevcnt[SNARE]) {
			prevcnt[SNARE] = beat_cnt_map[SNARE];
			printf("\tSnareHit (%i)\n", scnt++);
		}
 
		if (beat_cnt_map[CYMBALS] != prevcnt[CYMBALS]) {
			prevcnt[CYMBALS] = beat_cnt_map[CYMBALS];
			printf("\t\tTweeters (%i)\n", ccnt++);
		}
		
		if (beat_cnt_map[BASSLINE] != prevcnt[BASSLINE]) {
			prevcnt[BASSLINE] = beat_cnt_map[BASSLINE];
//		printf("\t\t\tBassline action %i\n", blcnt++);
		}
	}
	return 0;
}

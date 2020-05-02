/*
 * newtest.c
 *
 * Copyright (c) 2014 Jeremy Garff <jer @ jers.net>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 *     1.  Redistributions of source code must retain the above copyright notice, this list of
 *         conditions and the following disclaimer.
 *     2.  Redistributions in binary form must reproduce the above copyright notice, this list
 *         of conditions and the following disclaimer in the documentation and/or other materials
 *         provided with the distribution.
 *     3.  Neither the name of the owner nor the names of its contributors may be used to endorse
 *         or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

static char VERSION[] = "XX.YY.ZZ";

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdarg.h>
#include <getopt.h>

#include <errno.h>

#include "clk.h"
#include "gpio.h"
#include "dma.h"
#include "pwm.h"
#include "version.h"

#include "ws2811.h"

#define ARRAY_SIZE(stuff)       (sizeof(stuff) / sizeof(stuff[0]))

// defaults for cmdline options
#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                18
#define DMA                     10
//#define STRIP_TYPE            WS2811_STRIP_RGB                // WS2812/SK6812RGB integrated chip+leds
#define STRIP_TYPE              WS2811_STRIP_GBR	// WS2812/SK6812RGB integrated chip+leds
//#define STRIP_TYPE            SK6812_STRIP_RGBW               // SK6812RGBW (NOT SK6812RGB)

#define WIDTH                   125
#define HEIGHT                  1
#define LED_COUNT               (WIDTH * HEIGHT)

int width = WIDTH;
int height = HEIGHT;
int led_count = LED_COUNT;

int clear_on_exit = 0;

ws2811_t ledstring = {
	.freq = TARGET_FREQ,
	.dmanum = DMA,
	.channel = {
		    [0] = {
			   .gpionum = GPIO_PIN,
			   .count = LED_COUNT,
			   .invert = 0,
			   .brightness = 255,
			   .strip_type = STRIP_TYPE,
			   },
		    [1] = {
			   .gpionum = 0,
			   .count = 0,
			   .invert = 0,
			   .brightness = 0,
			   },
		    },
};

ws2811_led_t *matrix;

static uint8_t running = 1;

void matrix_render(int color)
{
	int x, y;

	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			ledstring.channel[0].leds[(y * width) + x] = color;
		}
	}
}

void matrix_clear(void)
{
	int x, y;

	for (y = 0; y < (height); y++) {
		for (x = 0; x < width; x++) {
			matrix[y * width + x] = 0;
		}
	}
}

static void ctrl_c_handler(int signum)
{
	(void)(signum);
	running = 0;
}

static void setup_handlers(void)
{
	struct sigaction sa = {
		.sa_handler = ctrl_c_handler,
	};

	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
}

void parseargs(int argc, char **argv, ws2811_t * ws2811)
{
	int index;
	int c;

	static struct option longopts[] = {
		{"help", no_argument, 0, 'h'},
		{"dma", required_argument, 0, 'd'},
		{"gpio", required_argument, 0, 'g'},
		{"invert", no_argument, 0, 'i'},
		{"clear", no_argument, 0, 'c'},
		{"strip", required_argument, 0, 's'},
		{"height", required_argument, 0, 'y'},
		{"width", required_argument, 0, 'x'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	while (1) {

		index = 0;
		c = getopt_long(argc, argv, "cd:g:his:vx:y:", longopts, &index);

		if (c == -1)
			break;

		switch (c) {
		case 0:
			/* handle flag options (array's 3rd field non-0) */
			break;

		case 'h':
			fprintf(stderr, "%s version %s\n", argv[0], VERSION);
			fprintf(stderr, "Usage: %s \n"
				"-h (--help)    - this information\n"
				"-s (--strip)   - strip type - rgb, grb, gbr, rgbw\n"
				"-x (--width)   - matrix width (default 8)\n"
				"-y (--height)  - matrix height (default 8)\n"
				"-d (--dma)     - dma channel to use (default 10)\n"
				"-g (--gpio)    - GPIO to use\n"
				"                 If omitted, default is 18 (PWM0)\n"
				"-i (--invert)  - invert pin output (pulse LOW)\n"
				"-c (--clear)   - clear matrix on exit.\n"
				"-v (--version) - version information\n",
				argv[0]);
			exit(-1);

		case 'D':
			break;

		case 'g':
			if (optarg) {
				int gpio = atoi(optarg);
/*
	PWM0, which can be set to use GPIOs 12, 18, 40, and 52.
	Only 12 (pin 32) and 18 (pin 12) are available on the B+/2B/3B
	PWM1 which can be set to use GPIOs 13, 19, 41, 45 and 53.
	Only 13 is available on the B+/2B/PiZero/3B, on pin 33
	PCM_DOUT, which can be set to use GPIOs 21 and 31.
	Only 21 is available on the B+/2B/PiZero/3B, on pin 40.
	SPI0-MOSI is available on GPIOs 10 and 38.
	Only GPIO 10 is available on all models.

	The library checks if the specified gpio is available
	on the specific model (from model B rev 1 till 3B)

*/
				ws2811->channel[0].gpionum = gpio;
			}
			break;

		case 'i':
			ws2811->channel[0].invert = 1;
			break;

		case 'c':
			clear_on_exit = 1;
			break;

		case 'd':
			if (optarg) {
				int dma = atoi(optarg);
				if (dma < 14) {
					ws2811->dmanum = dma;
				} else {
					printf("invalid dma %d\n", dma);
					exit(-1);
				}
			}
			break;

		case 'y':
			if (optarg) {
				height = atoi(optarg);
				if (height > 0) {
					ws2811->channel[0].count =
					    height * width;
				} else {
					printf("invalid height %d\n", height);
					exit(-1);
				}
			}
			break;

		case 'x':
			if (optarg) {
				width = atoi(optarg);
				if (width > 0) {
					ws2811->channel[0].count =
					    height * width;
				} else {
					printf("invalid width %d\n", width);
					exit(-1);
				}
			}
			break;

		case 's':
			if (optarg) {
				if (!strncasecmp("rgb", optarg, 4)) {
					ws2811->channel[0].strip_type =
					    WS2811_STRIP_RGB;
				} else if (!strncasecmp("rbg", optarg, 4)) {
					ws2811->channel[0].strip_type =
					    WS2811_STRIP_RBG;
				} else if (!strncasecmp("grb", optarg, 4)) {
					ws2811->channel[0].strip_type =
					    WS2811_STRIP_GRB;
				} else if (!strncasecmp("gbr", optarg, 4)) {
					ws2811->channel[0].strip_type =
					    WS2811_STRIP_GBR;
				} else if (!strncasecmp("brg", optarg, 4)) {
					ws2811->channel[0].strip_type =
					    WS2811_STRIP_BRG;
				} else if (!strncasecmp("bgr", optarg, 4)) {
					ws2811->channel[0].strip_type =
					    WS2811_STRIP_BGR;
				} else if (!strncasecmp("rgbw", optarg, 4)) {
					ws2811->channel[0].strip_type =
					    SK6812_STRIP_RGBW;
				} else if (!strncasecmp("grbw", optarg, 4)) {
					ws2811->channel[0].strip_type =
					    SK6812_STRIP_GRBW;
				} else {
					printf("invalid strip %s\n", optarg);
					exit(-1);
				}
			}
			break;

		case 'v':
			fprintf(stderr, "%s version %s\n", argv[0], VERSION);
			exit(-1);

		case '?':
			/* getopt_long already reported error? */
			exit(-1);

		default:
			exit(-1);
		}
	}
}

#include "gbd.h"

static void *shm_init(const char *filename)
{
	int fd, ret = -1;
	void *lmap = NULL;
	size_t shm_filesize;

	fd = shm_open(filename, O_RDWR | O_CREAT, (mode_t) 0666);
	if (fd < 0) {
		fprintf(stderr, "shm_open(3): %s\n", strerror(errno));
		goto exit;
	}

	shm_filesize = sysconf(_SC_PAGE_SIZE);
	ret = ftruncate(fd, shm_filesize);
	if (ret < 0) {
		fprintf(stderr, "ftruncate(2): %sn", strerror(errno));
		goto exit;
	}

	lmap = mmap(0, shm_filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (lmap == MAP_FAILED) {
		fprintf(stderr, "mmap(2): %s\n", strerror(errno));
		goto exit;
	}

	return lmap;
exit:
	close(fd);
	return NULL;
}

int main(int argc, char *argv[])
{
	ws2811_return_t ret;

	sprintf(VERSION, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR,
		VERSION_MICRO);

	parseargs(argc, argv, &ledstring);

	matrix = malloc(sizeof(ws2811_led_t) * width * height);

	setup_handlers();

	if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS) {
		fprintf(stderr, "ws2811_init failed: %s\n",
			ws2811_get_return_t_str(ret));
		return ret;
	}

	int *beat_cnt_map;
	beat_cnt_map = (int *)shm_init(GBD_BEAT_COUNT_FILE);
	if (!beat_cnt_map) {
		fprintf(stderr, "Could not open GBD IPC file!\n");
		return -1;
	}

	while (running) {
		static int tmp_cnt, bcnt;
		static int prevcnt[GBD_BEAT_COUNT_BUF_SIZE];

		usleep(10000);	/* 100ms */

		if (beat_cnt_map[KICKDRUM] != prevcnt[KICKDRUM]) {
			prevcnt[KICKDRUM] = beat_cnt_map[KICKDRUM];
			printf("BassBeat (%i)\n", bcnt++);

			if (tmp_cnt == 0) {
				matrix_render(0x00200000);
			} else if (tmp_cnt == 1) {
				matrix_render(0x00002000);
			} else {
				matrix_render(0x00000020);
			}
			tmp_cnt = ++tmp_cnt > 2 ? 0 : tmp_cnt;

			if ((ret = ws2811_render(&ledstring)) != WS2811_SUCCESS) {
				fprintf(stderr, "ws2811_render failed: %s\n",
					ws2811_get_return_t_str(ret));
				break;
			}
		}		/* if (beat_cnt_map[KICKDRUM]) */
	}

	if (clear_on_exit) {
		matrix_clear();
		matrix_render(0);
		ws2811_render(&ledstring);
	}

	ws2811_fini(&ledstring);

	printf("\n");
	return ret;
}

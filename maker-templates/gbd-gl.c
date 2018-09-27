/*
 * prog : gbd-gl.c
 * desc : demo opengl (glut) program for the gbd framework
 * 
 * NOTE: This is borrowed code. Sub-optimal for RPi. Use
 *       OpenGLES instead.
 *
 * Compile with:
 *
 * 	"gcc -Wall -O2 gbd-gl.c -o gbd-gl -lglut -lGLU -lrt -lm"
 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
  */
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include "gbd.h"
static int *beat_cnt_map;

#define BARWIDTH 30
#define BARSPACING 7
#define X_BAROFFSET (BARSPACING + BARWIDTH)
#define Y_BAROFFSET 50

#define WINWIDTH BARSPACING+100+100+100+100
#define WINHEIGHT 150
static int win_x = WINWIDTH;
static int win_y = WINHEIGHT;
static int win_id;

#define BOX_PER_FBAND 6
static float g_rgb[][3] = {
	{1.0f, 0.01f, 0.01f},
	{0.01f, 0.01f, 1.0f},
	{0.01f, 1.0f, 0.01f},
	{0.0f, 0.0f, 0.0f}
};

#define LFE KICKDRUM
#define MID SNARE
#define TWT CYMBALS
static void render_pattern(void);
static void render_cymbals(void);
static void render_snare(void);

static int prevcnt[GBD_BEAT_COUNT_BUF_SIZE];
static int step_cnt = 1, prev_step_cnt = 0;
static int c_step_cnt = 1, prev_c_step_cnt = 0;

#define W_INTVL 20
static void draw_bands(void)
{
	static int i;
	static float r = 1.0f, g, b;
	static float w_val = 1.0f;
#define C_CNT 12
#define W_CNT 12
	static int w_intvl = W_INTVL, snare_hit_on;
	
	if (!snare_hit_on) {
		if (beat_cnt_map[MID] != prevcnt[MID]) {
			prevcnt[MID] = beat_cnt_map[MID];
			snare_hit_on = 1;
			w_val = 1.0f;
		}
	}
 
	if (snare_hit_on) {
		glColor3f(w_val, w_val, w_val);
		w_val -= 1.0f/W_INTVL;
		if (w_intvl-- <= 0) {
			w_intvl = W_INTVL;
			snare_hit_on = 0;
		}
		render_snare();		
	}
	
	if (beat_cnt_map[BASSLINE] != prevcnt[BASSLINE]) {
		prevcnt[BASSLINE] = beat_cnt_map[BASSLINE];
	
		if (g_rgb[i][0] == 0.0f) i = 0;

		r = g_rgb[i][0]; g = g_rgb[i][1]; b = g_rgb[i][2];
		i++;
	}

	glColor3f(r, g, b);	
	if (beat_cnt_map[LFE] != prevcnt[LFE]) {
		prevcnt[LFE] = beat_cnt_map[LFE];
		step_cnt++;
	}
	render_pattern();

	if (beat_cnt_map[TWT] != prevcnt[TWT]) {
		prevcnt[TWT] = beat_cnt_map[TWT];
		c_step_cnt++;
	}

	render_cymbals();
	return;
}

#define SNARE_LED_HGHT 20
#define CYMBALS_LED_HGHT 4
#define CYMBALS_LED_WDTH 200
#define BASE_LED_HGHT 100
#define BASE_LED_WDTH 100
static void __attribute__((unused)) render_snare(void)
{

#define Y_SBASE BARSPACING+CYMBALS_LED_HGHT+BARSPACING+BASE_LED_HGHT+BARSPACING
	glRectf(BARSPACING, Y_SBASE, 200, Y_SBASE+20);	
	glRectf(BARSPACING + 200, Y_SBASE, 400, Y_SBASE+20);	
	return;
}

static void __attribute__((unused)) render_cymbals(void)
{
	int xidx = 0;
#define CPATTERN_WDTH 9
#define CPATTERN_LEN 17
	float cpattern[CPATTERN_WDTH][CPATTERN_LEN] = {
	{1.0f, .9f, .8f, .7f, .6f, .5f, .4f, .3f,   .3f, .4f, .5f, .6f, .7f, .8f, .9f, 1.0f},
	{0.0f, .9f, .8f, .7f, .6f, .5f, .4f, .3f,   .3f, .4f, .5f, .6f, .7f, .8f, .9f, 0.0f},
	{0.0f, .0f, .8f, .7f, .6f, .5f, .4f, .3f,   .3f, .4f, .5f, .6f, .7f, .8f, .0f, 0.0f},
	{0.0f, .0f, .0f, .7f, .6f, .5f, .4f, .3f,   .3f, .4f, .5f, .6f, .7f, .0f, .0f, 0.0f},
	{0.0f, .0f, .0f, .0f, .6f, .5f, .4f, .3f,   .3f, .4f, .5f, .6f, .0f, .0f, .0f, 0.0f},
	{0.0f, .0f, .0f, .0f, .0f, .5f, .4f, .3f,   .3f, .4f, .5f, .0f, .0f, .0f, .0f, 0.0f},
	{0.0f, .0f, .0f, .0f, .0f, .0f, .4f, .3f,   .3f, .4f, .0f, .0f, .0f, .0f, .0f, 0.0f},
	{0.0f, .0f, .0f, .0f, .0f, .0f, .0f, .3f,   .3f, .0f, .0f, .0f, .0f, .0f, .0f, 0.0f},
	{0.0f, .0f, .0f, .0f, .0f, .0f, .0f, .0f,   .0f, .0f, .0f, .0f, .0f, .0f, .0f, 0.0f},
	};
	static float *pattern;
	static int i = 0;
	
	static int once = 0;

	if (!once) {
		pattern = cpattern[0];
		once = 1;
	}

	if (c_step_cnt != prev_c_step_cnt) { /* beat detected? */
		pattern = cpattern[0];
		i = 1;
	}
	
#define Y_CBASE BARSPACING
	for (xidx = 0; xidx < CPATTERN_LEN; xidx++) {
		if (pattern[xidx] != 0.0f) {
			glColor3f(pattern[xidx], pattern[xidx], pattern[xidx]);
			glRectf(xidx * 25 + BARSPACING, Y_CBASE, xidx * 25+25, Y_CBASE+4); 
		}
	}

	if (i < 9)
		pattern = cpattern[i++];
	
	prev_c_step_cnt = c_step_cnt;

	return;
}
static void __attribute__((unused)) render_pattern(void)
{
	int xidx = 0;
	static int pattern = 0;

	if (step_cnt != prev_step_cnt) { /* beat detected? */
			pattern++;
	}
	/* select display pattern */
	pattern %= 2;

#define Y_BBASE BARSPACING+CYMBALS_LED_HGHT+BARSPACING	
	/* render display pattern */
	if (pattern == 0) {
		xidx = 0;
		glRectf((xidx*(100))+BARSPACING, Y_BBASE, 
				(xidx*(100))+ (100), Y_BBASE+100);	
		xidx = 3;
		glRectf((xidx*(100))+BARSPACING, Y_BBASE, 
				(xidx*(100))+ (100), Y_BBASE + 100);	
	}
	if (pattern == 1) {
		xidx = 1;
		glRectf((xidx*(100))+BARSPACING, Y_BBASE, 
				(xidx*(100))+ (100), Y_BBASE + 100);	
		xidx = 2;
		glRectf((xidx*(100))+BARSPACING, Y_BBASE, 
				(xidx*(100))+ (100), Y_BBASE + 100);	
	}

	prev_step_cnt = step_cnt;
}

static void pre_display ( void )
{
	glViewport ( 0, 0, win_x, win_y );
	glMatrixMode ( GL_PROJECTION );
	glLoadIdentity (); 
	gluOrtho2D ( 0.0, win_x, 0.0, win_y ); 
	glClearColor ( 0.0f, 0.0f, 0.0f, 1.0f );
	glClear(GL_COLOR_BUFFER_BIT);
}

static void post_display ( void )
{
	glutSwapBuffers ();
}

static void display_func ( void )
{
	pre_display ();
	draw_bands();
	post_display ();
}

static void idle_func ( void )
{
	glutSetWindow ( win_id );
	glutPostRedisplay ();
}

static void key_func ( unsigned char key, int x, int y )
{
	switch ( key )
	{
		case 27 : /* ESC */
		case 'q':
		case 'Q':
			exit (EXIT_SUCCESS);
			break;			
	}
}

static void reshape_func ( int width, int height )
{
	glutSetWindow ( win_id );
	glutReshapeWindow ( width, height );

	win_x = width;
	win_y = height;
}

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
		abort();
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
 
static void open_glut_window ( void )
{
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition ( 0, 0 );
	glutInitWindowSize ( win_x, win_y );
	win_id = glutCreateWindow ( "GBD OpenGL Demo" );
	glClearColor ( 0.0f, 0.0f, 0.0f, 1.0f );
	glClear ( GL_COLOR_BUFFER_BIT );
	glutSwapBuffers ();
	glClear ( GL_COLOR_BUFFER_BIT );
	glutSwapBuffers ();
	pre_display ();
	glutKeyboardFunc ( key_func );
	glutReshapeFunc ( reshape_func );
	glutIdleFunc ( idle_func );
	glutDisplayFunc ( display_func );
}

int main(int argc, char **argv) 
{
	void *lmap;
	glutInit(&argc, argv);
	const char *filename = "gbd";

	if (argv[1])
		filename = argv[1];

	if(!(lmap = shm_init(filename)))
		exit(EXIT_FAILURE);
	beat_cnt_map = (int *)lmap;
	
	open_glut_window();
	glutMainLoop();
	exit(EXIT_SUCCESS);
}


/*
 * file:  gbd.h
 * desc:  GBD Linux POSIX SHM interface
 */

#ifndef __GBD_H__
#define __GBD_H__

/* GBD beat count data is stored in a simple "C int" 
 * array in POSIX SHM */

/* Name of GBD beat data file in POSIX SHM */
#define GBD_BEAT_COUNT_FILE "gbd"

/* Array offsets */
#define KICKDRUM 0
#define SNARE    1
#define CYMBALS  2
#define AVG_ENERGY_L_CHANNEL 3
#define AVG_ENERGY_R_CHANNEL 4
#define RESERVED0 5
#define RESERVED1 6 
#define RESERVED2 7
#define RESERVED3 8
#define BASSLINE 9

/* Number of array elements */
#define GBD_BEAT_COUNT_BUF_SIZE 10

#endif /* __GBD_H__ */

/*
 * file : gbdclient.c
 * desc : ALSA external PCM filter plugin for the gbd (Generic Beat Detector) 
 *        framework
 * 
 * This file is a part of the GBD framework, 
 * https://github.com/generic-beat-detector/GBD
 * 
 * Copyright (c) GBD, 
 * generic.beat.detector@gmail.com
 *
 * The program in this file is licensed under the terms of the MIT license.
 */

/* alsa */
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <alsa/pcm_external.h>

/* external pcm filter plugin object */
typedef struct snd_pcm_gbdclient {
	snd_pcm_extplug_t ext;
	int fd;
	int channels;
} snd_pcm_gbdclient_t;

/* internet sockets */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* GBD command passing */
#define GBD_ERROR -1
#define GBD_SUCCESS 0
#define GBD_LADSPA_LIB_INIT 1
#define GBD_CLIENT_CHANNELS 2
#define GBD_AUDIO_SAMPLE_RATE 3
#define GBD_PCM_PLUGIN_INIT 4
#define GBD_BEAT_DETECTION_FUNC 5
#define GBD_PCM_PLUGIN_CLOSE 6
typedef struct __gbd_msg {
	/* note: declare strict types to avoid 
	 *       problems when executing on
	 *       a 64-bit gbdclient host OS ...
	 *       recall that the gbdserver on
	 *       the raspberry pi is 32-bit */
	int32_t cmd;
	int32_t data;
} gbd_msg_t;

static ssize_t gbd_read(int fd, void *buf, size_t n)
{
    ssize_t ret;                    
    size_t count;              
    char *pbuf;

    pbuf = buf;                      
    for (count = 0; count < n; ) {
        ret = read(fd, pbuf, n - count);
        if (ret == 0) /* eof */
            return count;
        if (ret == -1) {
            if (errno == EINTR)
                continue;
            else
                return -1;
        }
        count += ret;
        pbuf += ret;
    }
    return count;
}

static ssize_t gbd_write(int fd, const void *buf, size_t n)
{
    ssize_t ret;
    size_t count;
    const char *pbuf;

    pbuf = buf;
    for (count = 0; count < n; ) {
        ret = write(fd, pbuf, n - count);
        if (ret <= 0) {
            if (ret == -1 && errno == EINTR)
                continue;
            else
                return -1;
        }
        count += ret;
        pbuf += ret;
    }
    return count;
}

static int gbd_connect(const char *ipaddr, 
		const char *port, int type)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_family = AF_UNSPEC; /* v4/v6 */
    hints.ai_socktype = type;

    s = getaddrinfo(ipaddr, port, &hints, &result);
    if (s != 0) {
        errno = ENOSYS;
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;   
        close(sfd);
    }
    freeaddrinfo(result);
    return (rp == NULL) ? -1 : sfd;
}

/*
 * func: gbdclient_transfer
 * desc: this function is invoked by the alsa-lib runtime and
 *       receives the alsa period pcm signal in alsa float format;
 *       it is responsible for sending the signal to the gbdserver
 *       for analysis as well as for passing the signal to the 
 *       configured alsa pcm slave.
 *
 *       plugin writers wishing to port this pcm plugin to a standalone
 *       audio application platform or some other operating system's 
 *       sound framework should consult the documentation on the 
 *       GBD github wiki for more details.
 */
static snd_pcm_sframes_t gbdclient_transfer(snd_pcm_extplug_t * ext,
				     const snd_pcm_channel_area_t * dst_areas,
				     snd_pcm_uframes_t dst_offset,
				     const snd_pcm_channel_area_t * src_areas,
				     snd_pcm_uframes_t src_offset,
				     snd_pcm_uframes_t size)
{
	snd_pcm_gbdclient_t *gbd = (snd_pcm_gbdclient_t *) ext;
	float *src, *dst;
	gbd_msg_t msg;
	size_t nbytes = size * gbd->channels * sizeof(float);

	src = (float *)(src_areas->addr +
			(src_areas->first + src_areas->step * src_offset) / 8);
	
	dst = (float *)(dst_areas->addr +
			(dst_areas->first + dst_areas->step * dst_offset) / 8);

	/* notify gbdserver of incoming audio signal for analysis */
	msg.cmd = GBD_BEAT_DETECTION_FUNC;
	msg.data = (int32_t)size;
	if (gbd_write(gbd->fd, &msg, sizeof(msg)) < 0) {
		fprintf(stderr, 
				"%s:%s:%d:: WARNING!! GBD PCM cmd write failed!\n",
				__FILE__, __func__, __LINE__);
	}

	/* send audio signal to gbdserver for analysis */
	if (gbd_write(gbd->fd, src, nbytes) < 0) {
		fprintf(stderr, 
				"%s:%s:%d:: WARNING!! GBD PCM data write failed!\n",
				__FILE__, __func__, __LINE__);
	};

	/* pass audio signal to alsa pcm slave */
	memcpy(dst, src, nbytes);
	return size;
}

static int gbdclient_close(snd_pcm_extplug_t * ext)
{
	snd_pcm_gbdclient_t *gbd = ext->private_data;
	gbd_msg_t msg;
	msg.cmd = GBD_PCM_PLUGIN_CLOSE;
	if (gbd_write(gbd->fd, &msg, sizeof(msg)) < 0) {
		SNDERR("WARNING: gbd server-side PCM plugin release failed!");
	}
	close(gbd->fd); 
	free(gbd);
	return 0;
}

static int gbdclient_init(snd_pcm_extplug_t * ext)
{
	snd_pcm_gbdclient_t *gbd = (snd_pcm_gbdclient_t *) ext;
	int srate = ext->rate;
	int err;
	gbd_msg_t msg;

	/* prepare to initialize gbdserver-side pcm plugin */
	msg.cmd = GBD_CLIENT_CHANNELS;
	msg.data = gbd->channels;
	err = gbd_write(gbd->fd, &msg, sizeof(msg));
	if (err < 0) {
		SNDERR("gbd_write failed! (channels)");
		return -EINVAL;
	}

 	msg.cmd = GBD_AUDIO_SAMPLE_RATE;
	msg.data = srate;
	err = gbd_write(gbd->fd, &msg, sizeof(msg));
	if (err < 0) {
		SNDERR("gbd_write failed! (sampel rate)");
		return -EINVAL;
	}

	/* initialize gbd server-side pcm plugin */
	msg.cmd = GBD_PCM_PLUGIN_INIT;
	err = gbd_write(gbd->fd, &msg, sizeof(msg));
	if (err < 0) {
		SNDERR("gbd_write failed! (plugin init)");
		return -EINVAL;
	}

	err = gbd_read(gbd->fd, &msg, sizeof(msg));
	if (err < 0 || msg.cmd == GBD_ERROR) {
		SNDERR("Failed to initialize gbdserver-side PCM plugin module");
		return -EINVAL;
	}	

	return 0;
}

static snd_pcm_extplug_callback_t pcm_gbdclient_callback = {
	.transfer = gbdclient_transfer,
	.init = gbdclient_init,
	.close = gbdclient_close,
};

SND_PCM_PLUGIN_DEFINE_FUNC(gbdclient)
{
	snd_config_iterator_t i, next;
	snd_pcm_gbdclient_t *gbd;
	snd_config_t *sconf = NULL;
	const char *ipaddr;
	const char *port;
	long channels = 2;
	int err;
	gbd_msg_t msg;

	/* Parse config file (e.g. .asoundrc) options */
	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		const char *id;
		if (snd_config_get_id(n, &id) < 0)
			continue;

		if (strcmp(id, "comment") == 0 || strcmp(id, "type") == 0
		    || strcmp(id, "hint") == 0)
			continue;

		if (strcmp(id, "slave") == 0) {
			sconf = n;
			continue;
		}

		if (strcmp(id, "ipaddr") == 0) {
			snd_config_get_string(n, &ipaddr);
			continue;
		}

		if (strcmp(id, "port") == 0) {
			snd_config_get_string(n, &port);
			continue;
		}

		if (strcmp(id, "channels") == 0) {
			snd_config_get_integer(n, &channels);
			if (channels != 2) {
				SNDERR("Only stereo streams supported");
				return -EINVAL;
			}
			continue;
		}
		SNDERR("Unknown field %s", id);
		return -EINVAL;
	}

	/* Make sure an ip address was specified */
	if (!ipaddr || !port) {
		SNDERR("Missing \"ipaddr\" or \"port\"\n");
		return -EINVAL;
	}

	/* Make sure an ALSA slave PCM device was specified */
	if (!sconf) {
		SNDERR("Must include slave configuration with gbdclient plugin");
		return -EINVAL;
	}

	/* Alloc local GBD object */
	gbd = calloc(1, sizeof(*gbd));
	if (gbd == NULL)
		return -ENOMEM;

	/* Initialize local GDB object members */
	gbd->ext.version = SND_PCM_EXTPLUG_VERSION;
	gbd->ext.name = "gbdclient";
	gbd->ext.callback = &pcm_gbdclient_callback;
	gbd->ext.private_data = gbd;
	gbd->channels = channels;
	gbd->fd = gbd_connect(ipaddr, port, SOCK_STREAM);
	if (gbd->fd < 0) {
		SNDERR("Failed to connect with gbdserver");
		return -EINVAL;
	}

	/* Load gbd server-side DSP LADSPA library module */
	msg.cmd = GBD_LADSPA_LIB_INIT;
	err = gbd_write(gbd->fd, &msg, sizeof(msg));
	if (err < 0) {
		SNDERR("Failed to initialize gbd LADSPA module");
		return -EINVAL;
	}

	err = gbd_read(gbd->fd, &msg, sizeof(msg));
	if (err < 0 || msg.cmd == GBD_ERROR) {
		SNDERR("Failed to initialize gbd LADSPA module");
		return -EINVAL;
	}

	/* Create gbdclient external PCM filter plugin */
	err = snd_pcm_extplug_create(&gbd->ext, name, root, sconf, stream, mode);
	if (err < 0) {
		close(gbd->fd);
		free(gbd);
		return err;
	}

	/* Set external PCM filter plugin constraints */
	snd_pcm_extplug_set_param_minmax(&gbd->ext,
					 SND_PCM_EXTPLUG_HW_CHANNELS,
					 gbd->channels,
					 gbd->channels);
	snd_pcm_extplug_set_slave_param(&gbd->ext,
					SND_PCM_EXTPLUG_HW_CHANNELS,
					gbd->channels);
	snd_pcm_extplug_set_param(&gbd->ext,
				  SND_PCM_EXTPLUG_HW_FORMAT,
				  SND_PCM_FORMAT_FLOAT);
	snd_pcm_extplug_set_slave_param(&gbd->ext, SND_PCM_EXTPLUG_HW_FORMAT,
					SND_PCM_FORMAT_FLOAT);

	*pcmp = gbd->ext.pcm;
	return 0;
}
SND_PCM_PLUGIN_SYMBOL(gbdclient);

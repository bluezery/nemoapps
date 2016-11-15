#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>

#include <math.h>
#include <ao/ao.h>
#include <sndfile.h>

#define BUFFER_LEN 4096

static int __default_driver;

static char* convert_to_16bits(char *originfile)
{
	int read;
	float buffer[BUFFER_LEN];
	char convertfile[32];
	SNDFILE *infile, *outfile;
	SF_INFO sfinfo;

	sprintf(convertfile, "/tmp/%u_%lu", (uint32_t) time(NULL), pthread_self());
	printf("tmp soundfile: %s\n", convertfile);

	memset(&sfinfo, 0x0, sizeof(sfinfo));
	if ((infile = sf_open(originfile, SFM_READ, &sfinfo)) == NULL) {
		printf("fail to open input file %s(%d).\n", originfile, errno);
		return "";
	}

	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	if ((outfile = sf_open(convertfile, SFM_WRITE, &sfinfo)) == NULL) {
		printf("fail to open output file %s(%d).\n", convertfile, errno);
		return "";
	}

	while ((read = sf_read_float(infile, buffer, BUFFER_LEN)) > 0) {
		sf_write_float(outfile, buffer, read);
	}

	sf_close(infile);
	sf_close(outfile);

	return strdup(convertfile);
}

static void* sound_thread(void *userdata)
{
	ao_device *device;
	ao_sample_format format;
	SF_INFO sfinfo;
	SNDFILE *infile;


	char *originfile;
	char *convertfile;
	short *buffer;

	if (!userdata) {
		return NULL;
	}

	originfile = (char *) userdata;
	convertfile = convert_to_16bits(originfile);

	infile = sf_open(convertfile, SFM_READ, &sfinfo);

	switch (sfinfo.format & SF_FORMAT_SUBMASK) {
		case SF_FORMAT_PCM_16:
			format.bits = 16;
			break;
		case SF_FORMAT_PCM_24:
			format.bits = 24;
			break;
		case SF_FORMAT_PCM_32:
			format.bits = 32;
			break;
		case SF_FORMAT_PCM_S8:
			format.bits = 8;
			break;
		case SF_FORMAT_PCM_U8:
			format.bits = 8;
			break;
		default:
			format.bits = 16;
			break;
	}

	if (format.bits >= 32) {
		//TODO current FINE doesn't support 32 bits above
		sf_close(infile);
		return NULL;
	}

	format.channels = sfinfo.channels;
	format.rate = sfinfo.samplerate;
	format.byte_format = AO_FMT_NATIVE;
	format.matrix = 0;

	device = ao_open_live(__default_driver, &format, NULL);
	if (device == NULL) {
		fprintf(stderr, "Error opening device: %s.\n", strerror(errno));
		return NULL;
	}

	buffer = calloc(BUFFER_LEN, sizeof(short));

	while (1) {
		int read = sf_read_short(infile, buffer, BUFFER_LEN);
		if (ao_play(device, (char *) buffer, (uint32_t) (read * sizeof(short))) == 0) {
			break;
		}
	}

	sf_close(infile);
	ao_close(device);

	// remove tmp sound file
	unlink(convertfile);
    free(convertfile);

	return NULL;
}

static int __init = 0;

extern int nemosound_play(char *file)
{
	pthread_t id;

	if (!file) {
		return -1;
	}

    if (__init <= 0) {
        ao_initialize();
        __default_driver = ao_default_driver_id();
        __init++;
    }

	pthread_create(&id, NULL, sound_thread, file);

	return 0;
}

/* AudioCompress.c
** Simple commandline audio compressor
*/

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>

#include <unistd.h>
#include <stdlib.h>

#include "compress.h"
#include "config.h"

#ifdef USE_ESD
#include "esd.h"
#endif

void showhelp(char *name)
{
	fprintf(stderr, "Usage: %s [options]\n", name);
	fprintf(stderr, "\nOptions:\n");
	fprintf(stderr,
		"\t-c/-C\tDisable/enable clipping protection (default: %s)\n",
		ANTICLIP?"enabled":"disabled");
	fprintf(stderr,
		"\t-t #\tSet the target signal level (1-32767; default: %d)\n",
		TARGET);
	fprintf(stderr,
		"\t-g #\tSet the maximum gain (1-255; default: %d)\n",
		GAINMAX);
	fprintf(stderr,
		"\t-s #\tSet the smoothing value (1-%d; default: %d)\n",
		31 - GAINSHIFT, GAINSMOOTH);
	fprintf(stderr,
		"\t-b #\tSet the history length (default: %d)\n",
		BUCKETS);
	fprintf(stderr,
		"\t-x/-X \tEnable/disable byte swapping\n");

#ifdef USE_X
	fprintf(stderr,
		"\t-m/-M\tDisable/enable monitor window (default: %s)\n",
		SHOWMON?"enabled":"disabled");
#endif

#ifdef USE_ESD
	fprintf(stderr,
		"\t-e [host]\tConnect to esound daemon\n");
#endif

	fprintf(stderr, "\t-h\tShow this usage information\n");
}

int main(int argc, char *argv[])
{
	int16_t buf[4096];
	int len;
	int opt;
	int swab = 0;
#ifdef USE_ESD
	int esd = 0;
	char *esd_host = NULL;
#endif
	int mon = SHOWMON, aclip = ANTICLIP, tgain = TARGET,
		mgain = GAINMAX, gsmooth = GAINSMOOTH,
		hsize = BUCKETS;
	int fd_in = fileno(stdin), fd_out = fileno(stdout);

	while ((opt = getopt(argc, argv,
#ifdef USE_ESD
					"e::"
#endif
					"mMcCht:xX:g:s:b:")) >= 0)
	{
		switch(opt)
		{
#ifdef USE_ESD
		case 'e':
			esd = 1;
			esd_host = optarg;
			break;
#endif
		case 'm':
			mon = 0;
			break;
		case 'M':
			mon = 1;
			break;

		case 'c':
			aclip = 0;
			break;
		case 'C':
			aclip = 1;
			break;

		case 't':
			tgain = atoi(optarg);
			if (tgain < 1 || tgain > 32767)
			{
				fprintf(stderr,
					"Invalid target level %s\n", optarg);
				return 1;
			}
			break;

		case 'g':
			mgain = atoi(optarg);
			if (mgain < 1 || mgain > 255)
			{
				fprintf(stderr,
					"Invalid maximum gain %s\n", optarg);
				return 1;
			}
			break;

		case 's':
			gsmooth = atoi(optarg);
			if (gsmooth < 1 || gsmooth > 31 - GAINSHIFT)
			{
				fprintf(stderr,
					"Invalid smoothing value %s\n",
					optarg);
				return 1;
			}
			break;

		case 'b':
			hsize = atoi(optarg);
			if (hsize < 1)
			{
				fprintf(stderr,
					"Invalid history size %s\n",
					optarg);
				return 1;
			}
			break;

		case 'x':
			swab = 1;
			break;
		case 'X':
			swab = 0;
			break;

		case 'h':
			showhelp(argv[0]);
			return 0;

		default:
			showhelp(argv[0]);
			return 1;
		}
	}

	CompressCfg(mon, aclip, tgain, mgain, gsmooth, hsize);
#ifdef USE_ESD
	if(esd){
		fd_in = fd_out = esd_filter_stream(
			ESD_BITS16|ESD_STEREO|ESD_STREAM|ESD_PLAY,
			ESD_DEFAULT_RATE, esd_host, argv[0]);
		
		if (fd_in < 0) {
			fprintf(stderr, "esd connection failed\n");
			return 1;
		}
	}	
#endif

	while ((len = read(fd_in, buf, 4096*sizeof(int16_t))) > 0)
	{
		if (swab)
		{
			int i;
			for (i = 0; i < len/sizeof(int16_t); i++)
				buf[i] = ((buf[i]&255)<<8) | (buf[i]>>8);
		}
		CompressDo(buf, len);
		write(fd_out, buf, len);
	}

	if (len < 0)
	{
		perror("AudioCompress");
		return 1;
	}

	return 0;
}


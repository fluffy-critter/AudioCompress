/* AudioCompress.c
** Simple commandline audio compressor
*/

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

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
		"\t-t #\tSet the target signal level (1-32767; default: %d)\n",
		TARGET);
	fprintf(stderr,
		"\t-g #\tSet the maximum gain (1-255; default: %d)\n",
		GAINMAX);
	fprintf(stderr,
		"\t-s #\tSet the gain smoothing exponent (1-15; default: %d)\n",
		GAINSMOOTH);
	fprintf(stderr,
		"\t-b #\tSet the history length (default: %d)\n",
		BUCKETS);
	fprintf(stderr,
		"\t-x/-X \tEnable/disable byte swapping\n");

#ifdef USE_ESD
	fprintf(stderr,
		"\t-e [host]\tConnect to esound daemon\n");
#endif

	fprintf(stderr, "\t-h\tShow this usage information\n");
}

int Run(int argc, char *argv[], struct Compressor *cmp)
{
	int16_t buf[4096];
        int ofs = 0, len;
	int opt;
	int swab = 0;
#ifdef USE_ESD
	int esd = 0;
	char *esd_host = NULL;
#endif
        struct CompressorConfig *cfg = Compressor_getConfig(cmp);
	int fd_in = fileno(stdin), fd_out = fileno(stdout);

	while ((opt = getopt(argc, argv,
#ifdef USE_ESD
					"e:"
#endif
					"t:g:s:b:xXh")) >= 0)
	{
		switch(opt)
		{
#ifdef USE_ESD
		case 'e':
			esd = 1;
			esd_host = optarg;
			break;
#endif
		case 't':
			cfg->target = atoi(optarg);
			if (cfg->target < 1 || cfg->target > 32767)
			{
				fprintf(stderr,
					"Invalid target level %s\n", optarg);
				return 1;
			}
			break;

		case 'g':
			cfg->maxgain = atoi(optarg);
			if (cfg->maxgain < 1 || cfg->maxgain > 255)
			{
				fprintf(stderr,
					"Invalid maximum gain %s\n", optarg);
				return 1;
			}
			break;

		case 's':
			cfg->smooth = atoi(optarg);
			if (cfg->smooth < 1 || cfg->smooth > 15)
			{
				fprintf(stderr,
					"Invalid smoothing value %s\n",
					optarg);
				return 1;
			}
			break;

		case 'b':
                {
			unsigned int hsize = atoi(optarg);
			if (hsize < 1)
			{
				fprintf(stderr,
					"Invalid history size %s\n",
					optarg);
				return 1;
			}
                        Compressor_setHistory(cmp, hsize);
			break;
                }

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

#ifdef USE_ESD
	if (esd)
        {
		fd_in = fd_out = esd_filter_stream(
			ESD_BITS16|ESD_STEREO|ESD_STREAM|ESD_PLAY,
			ESD_DEFAULT_RATE, esd_host, argv[0]);
		
		if (fd_in < 0) {
			fprintf(stderr, "esd connection failed\n");
			return 1;
		}
	}	
#endif

        ofs = 0;
	while ((len = read(fd_in, buf + ofs, 4096*sizeof(int16_t))) > 0)
	{
                int count = len/sizeof(int16_t);
                int used = count*sizeof(int16_t);
                
		if (swab)
		{
			int i;
			for (i = 0; i < count; i++)
				buf[i] = ((buf[i]&255)<<8) | (buf[i]>>8);
		}

                if (count)
                {
                        Compressor_Process_int16(cmp, buf, count);
                        write(fd_out, buf, used);
                }
                
                //! Fix a read which generated an incomplete sample
                ofs = len - used; 
                if (ofs)
                        memmove(buf, buf + used, ofs);
	}

	if (len < 0)
	{
		perror("AudioCompress");
		return 1;
	}

	return 0;
}

//! I'd rather be using C++, what with scope-lifeime objects and so on. Sigh.
int main(int argc, char *argv[])
{
        struct Compressor *cmp = Compressor_new(0);
        int ret = Run(argc, argv, cmp);
        Compressor_delete(cmp);
        return ret;
}

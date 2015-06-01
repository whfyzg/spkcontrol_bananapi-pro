
#include <fcntl.h>
#include <linux/ioctl.h>

#include "yamahaspk.h"

#define YAMAHA_SPK_BASE 0xef

#define YAMAHA_SPK_RESET_LED  _IO(YAMAHA_SPK_BASE, 0)
#define YAMAHA_SPK_AUDIO_ROUTE_1  _IO(YAMAHA_SPK_BASE, 10)
#define YAMAHA_SPK_AUDIO_ROUTE_2  _IO(YAMAHA_SPK_BASE, 11)
#define YAMAHA_SPK_AUDIO_ROUTE_3  _IO(YAMAHA_SPK_BASE, 12)

static fd;

int yamahaspk_open (void)
{
	fd = open("/dev/yamahaspk", O_RDWR, 0);
	return 0;
}

void yamahaspk_close(void)
{
	return close(fd);
}

int yamahaspk_reset_led(void)
{
	return ioctl(fd, YAMAHA_SPK_RESET_LED, NULL);
}

int yamahaspk_switch_audio(int route)
{
	switch(route){
	case 0:
		return ioctl(fd, YAMAHA_SPK_AUDIO_ROUTE_1, NULL);
	case 1:
		return ioctl(fd, YAMAHA_SPK_AUDIO_ROUTE_2, NULL);
	case 2:
		return ioctl(fd, YAMAHA_SPK_AUDIO_ROUTE_3, NULL);
	default:
		return -1;
	}
}

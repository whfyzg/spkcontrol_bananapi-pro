
#include <fcntl.h>
#include <signal.h>
#include <linux/ioctl.h>

#include "yamahaspk.h"

#define YAMAHA_SPK_BASE 0xa1

#define YAMAHA_SPK_RESET_LED  _IO(YAMAHA_SPK_BASE, 0)
#define YAMAHA_SPK_AUDIO_ROUTE_1  _IO(YAMAHA_SPK_BASE, 10)
#define YAMAHA_SPK_AUDIO_ROUTE_2  _IO(YAMAHA_SPK_BASE, 11)
#define YAMAHA_SPK_AUDIO_ROUTE_3  _IO(YAMAHA_SPK_BASE, 12)
#define YAMAHA_SPK_STOP_LED  _IO(YAMAHA_SPK_BASE, 13)
#define YAMAHA_SPK_CONTINUE_LED _IO(YAMAHA_SPK_BASE, 14)
#define YAMAHA_SPK_START_TIMER  _IO(YAMAHA_SPK_BASE, 15)

static fd;

static int timer_count = 0;

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

int yamahaspk_stop_led(void){
	return ioctl(fd, YAMAHA_SPK_STOP_LED, NULL);
}

int yamahaspk_start_led(void){
	return ioctl(fd, YAMAHA_SPK_CONTINUE_LED, NULL);
}

int yamahaspk_start_timer(void){
	return ioctl(fd, YAMAHA_SPK_START_TIMER, NULL);
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

/*
int yamahaspk_timer_func(int sig)
{
	timer_count += 1;
//	Yamahaspk_timer_func(timer_count);
	if (timer_count < 3){
		alarm(7);
	}
		
	if (timer_count == 3){
		timer_count = 0;
		yamahaspk_start_led();
	}

	return 0;
}

int yamahaspk_start_timer(void)
{
	signal(SIGALRM, Yamahaspk_timer_func);
	alarm(7);
	return 0;
}
*/


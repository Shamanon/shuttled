
/*

 Contour ShuttlePro v2 interface

 Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)

 Updated to use uniput 2015 by Joshua Besneatte (http://pirates.jairaja.org)

 Based on a version (c) 2006 Trammell Hudson <hudson@osresearch.net>

 which was in turn

 Based heavily on code by Arendt David <admin@prnet.org>

*/

#include "shuttled.h"

typedef struct input_event EV;

int debug_strokes = 1;

unsigned short jogvalue = 0xffff;
int shuttlevalue = 0xffff;
struct timeval last_shuttle;
int need_synthetic_shuttle;
int wstat;

int nread;
char *dev_name;
int fd;
EV ev;

/* key bindings, needs to be moved to config file */
int mykey[7] = { KEY_HOME, KEY_LEFT, KEY_SPACE, KEY_RIGHT, KEY_END, KEY_LEFT, KEY_RIGHT };

/* Globals */
static int uinp_fd = -1;
struct uinput_user_dev uinp; // uInput device structure
struct input_event event; // Input device structure
/* Setup the uinput device */

void handler(int num) 
{
  // we might use this handler for many signals
  switch (num)
  {
    case SIGTERM:
      // clean up code.
      printf("Goodbye\n");
      exit(0);
      break;
  }
}

int setup_uinput_device()
{
	// Temporary variable
	int i=0;
	// Open the input device
	uinp_fd = open("/dev/uinput", O_WRONLY | O_NDELAY);
	if (uinp_fd == 0)
	{
		printf("Unable to open /dev/uinput %d\n",uinp_fd);
		return -1;
	}
	
	memset(&uinp,0,sizeof(uinp)); // Intialize the uInput device to NULL
	strncpy(uinp.name, "Contour Shuttle", UINPUT_MAX_NAME_SIZE);
	uinp.id.version = 4;
	uinp.id.bustype = BUS_USB;
	
	// Setup the uinput device
	ioctl(uinp_fd, UI_SET_EVBIT, EV_KEY);
	ioctl(uinp_fd, UI_SET_EVBIT, EV_REL);
	
	for (i=0; i < 256; i++) 
	{
		ioctl(uinp_fd, UI_SET_KEYBIT, i);
	}

	/* Create input device into input sub-system */
	wstat = write(uinp_fd, &uinp, sizeof(uinp));
	if (ioctl(uinp_fd, UI_DEV_CREATE))
	{
		printf("Unable to create UINPUT device.");
		return -1;
	}
	return 1;
}

void
send_key(unsigned int key, int press)
{
	if (debug_strokes) {
		printf("Got Stroke: %d\n", key);
	}
	
	if (press)
	{
		if (debug_strokes) printf("Press\n");
		// Report BUTTON CLICK - PRESS event
		memset(&event, 0, sizeof(event));
		gettimeofday(&event.time, NULL);
		event.type = EV_KEY;
		event.code = key;
		event.value = 1;
		wstat = write(uinp_fd, &event, sizeof(event));
		event.type = EV_SYN;
		event.code = SYN_REPORT;
		event.value = 0;
		wstat = write(uinp_fd, &event, sizeof(event));
	} else {
		if (debug_strokes) printf("Release\n");
		// Report BUTTON CLICK - RELEASE event
		memset(&event, 0, sizeof(event));
		gettimeofday(&event.time, NULL);
		event.type = EV_KEY;
		event.code = key;
		event.value = 0;
		wstat = write(uinp_fd, &event, sizeof(event));
		event.type = EV_SYN;
		event.code = SYN_REPORT;
		event.value = 0;
		wstat = write(uinp_fd, &event, sizeof(event));
	}
}

void
key(unsigned short code, unsigned int value)
{
  code -= EVENT_CODE_KEY1;

  if (code <= NUM_KEYS) {
    send_key(mykey[code-4],value);
  } else {
    fprintf(stderr, "key(%d, %d) out of range\n", code + EVENT_CODE_KEY1, value);
  }
}


void
shuttle(int value, int code)
{
	if (value < -7 || value > 7) {
		fprintf(stderr, "shuttle(%d) out of range\n", value);
	} else {
		fprintf(stderr, "shuttle sees = (%d, %d)\n", value, code);
	}
}

void
jog(int value, int code)
{
  int direction;
  int thekey;

  if (jogvalue != value) {
    value = value & 0xff;
    direction = ((value - jogvalue) & 0x80) ? -1 : 1;
	thekey = (direction > 0) ? mykey[6] : mykey[5];
	send_key(thekey,1);
	send_key(thekey,0);	
	jogvalue = value;
  }
}

void
jogshuttle(unsigned short code, unsigned int value)
{
  switch (code) {
  case EVENT_CODE_JOG:
    jog(value, code);
    break;
  case EVENT_CODE_SHUTTLE:
    shuttle(value, code);
    break;
  default:
    fprintf(stderr, "jogshuttle(%d, %d) invalid code\n", code, value);
    break;
  }
}

void
handle_event(EV ev)
{
	// FUTURE - Add check for custom translation request here?

	if (debug_strokes) fprintf(stderr, "handle_event = (%d, %d, 0x%x)\n", ev.type, ev.code, ev.value);

	switch (ev.type) {
		case EVENT_TYPE_DONE:
		case EVENT_TYPE_ACTIVE_KEY:
			break;
		case EVENT_TYPE_KEY:
			key(ev.code, ev.value);
			break;
		case EVENT_TYPE_JOGSHUTTLE:
			jogshuttle(ev.code, ev.value);
			break;
		default:
			fprintf(stderr, "handle_event() invalid type code\n");
		break;
	}
}

int
main(int argc, char **argv)
{

	int first_time = 1;

	if (argc != 2) {
		dev_name = "/dev/shuttle";
	} else {
		dev_name = argv[1];
	}
  
	// Return an error if device not found.
	if (setup_uinput_device() < 0)
	{
		fprintf(stderr, "Unable to find uinput device\n");
		exit(1);
	}
	
	signal (SIGTERM, handler);

	while (1) {
		fd = open(dev_name, O_RDONLY);
		if (fd < 0) {
			perror(dev_name);
			if (first_time) {
				exit(1);
			}
		} else {
			// Flag it as exclusive access
			if(ioctl( fd, EVIOCGRAB, 1 ) < 0) {
				perror( "evgrab ioctl" );
			} else {
				first_time = 0;
				while (1) {
					nread = read(fd, &ev, sizeof(ev));
					if (nread == sizeof(ev)) {
						handle_event(ev);
					} else {
						if (nread < 0) {
							perror("read event");
							break;
						} else {
							fprintf(stderr, "short read: %d\n", nread);
							break;
						}
					}
				}
			}
		}
		close(fd);
		sleep(1);
	}
}

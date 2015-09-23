
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

int nread;
int wstat;
char *dev_name = "/dev/shuttle";
char *conf_file = "/etc/shuttled";
int fd;
EV ev;

/* key bindings, needs to be moved to config file */
int mykey[17];

/* Globals */
static int uinp_fd = -1;
struct uinput_user_dev uinp; // uInput device structure
struct input_event event; // Input device structure

/* set up the uinput device */
int setup_uinput_device()
{
	// Temporary variables
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

/* Exit handler, gets called on pkill shuttled */
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

/* read config file */
void
read_conf_file()
{
	printf("Reading config file...\n");
  // read config file and put into mykey array
    config_t cfg, *cf;
    const config_setting_t *keys;
    //const char *keys;
    int count, n;

    cf = &cfg;
    config_init(cf);

    if (!config_read_file(cf, conf_file)) {
        fprintf(stderr, "%s:%d - %s\n",
            config_error_file(cf),
            config_error_line(cf),
            config_error_text(cf));
        config_destroy(cf);
        exit(0);
    }

    keys = config_lookup(cf, "default.keys");
    count = config_setting_length(keys);
	printf("I have %d keys:\n", count);
    for (n = 0; n < count; n++) {
        mykey[n] = config_setting_get_int_elem(keys, n);
        if(debug_strokes) printf("%d | %d\n",config_setting_get_int_elem(keys, n), n);
    }

    config_destroy(cf);
}

/* shutttle device event handling */
void
send_key(unsigned int key, int press)
{
	
	if (debug_strokes) printf("Got Stroke: %d\n", key);
	
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
  if (code <= NUM_KEYS) {
    send_key(mykey[code-1],value);
  } else {
    printf("key(%d, %d) out of range\n", code, value);
  }
}

void
shuttle(int value, int code)
{
	if (value < -7 || value > 7) {
		printf("shuttle(%d) out of range\n", value);
	} else {
		printf("shuttle sees = (%d, %d)\n", value, code);
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
	thekey = (direction > 0) ? 17 : 16;
	key(thekey,1);
	key(thekey,0);	
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
    printf("jogshuttle(%d, %d) invalid code\n", code, value);
    break;
  }
}

void
handle_event(EV ev)
{
	// FUTURE - Add check for custom translation request here?

	if (debug_strokes) printf("handle_event = (%d, %d, 0x%x)\n", ev.type, ev.code, ev.value);

	switch (ev.type) {
		case EVENT_TYPE_DONE:
		case EVENT_TYPE_ACTIVE_KEY:
			break;
		case EVENT_TYPE_KEY:
			key(ev.code - EVENT_CODE_KEY1, ev.value);
			break;
		case EVENT_TYPE_JOGSHUTTLE:
			jogshuttle(ev.code, ev.value);
			break;
		default:
			printf("handle_event() invalid type code\n");
		break;
	}
}

/* MAIN LOOP */
int
main(int argc, char **argv)
{

	int first_time = 1;

	// read config file
	read_conf_file();
	
	// use device passed from command line, or default device if none
	if (argc == 2) dev_name = argv[1];
  
	// Return an error if uinput device not found.
	if (setup_uinput_device() < 0)
	{
		printf("Unable to find uinput device\n");
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
							printf("short read: %d\n", nread);
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

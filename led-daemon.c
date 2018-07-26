#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/types.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../rpi_ws281x/ws2811.h"

static char VERSION[] = "0.1";

#define TARGET_FREQ WS2811_TARGET_FREQ
#define GPIO_PIN 18
#define DMA 10
#define STRIP_TYPE WS2811_STRIP_GRB // WS2812/SK6812RGB integrated chip+leds

#define LED_COUNT 1

ws2811_t ledstring = {
    .freq = TARGET_FREQ,
    .dmanum = DMA,
    .channel = {
            [0] = {
                .gpionum = GPIO_PIN,
                .count = LED_COUNT,
                .invert = 0,
                .brightness = 255,
                .strip_type = STRIP_TYPE,
            },
        [1] = {
            .gpionum = 0, .count = 0, .invert = 0, .brightness = 0,
        },
    },
};

ws2811_led_t color = 0x00000000;
int blink_rate = 1000;
int running = 1;
char fifo_path[4096];

static void ctrl_c_handler(int signum)
{
    (void)(signum);
    running = 0;
}

static void setup_handlers(void)
{
    struct sigaction sa = {
        .sa_handler = ctrl_c_handler,
    };

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

void parseargs(int argc, char** argv, ws2811_t* ws2811)
{
    int index;
    int c;

    static struct option longopts[] = {
        { "help", no_argument, 0, 'h' },
        { "fifo", required_argument, 0, 'f' },
        { "dma", required_argument, 0, 'd' },
        { "gpio", required_argument, 0, 'g' },
        { "invert", no_argument, 0, 'i' },
        { "strip", required_argument, 0, 's' },
        { "version", no_argument, 0, 'v' },
        { 0, 0, 0, 0 }
    };

    while (1) {
        index = 0;
        c = getopt_long(argc, argv, "hf:s:d:g:iv", longopts, &index);

        if (c == -1)
            break;

        switch (c) {
        case 0:
            /* handle flag options (array's 3rd field non-0) */
            break;

        case 'h':
            fprintf(stderr, "%s version %s\n", argv[0], VERSION);
            fprintf(stderr, "Usage: %s \n"
                            "-h (--help)    - this information\n"
                            "-f (--fifo)    - fifo path\n"
                            "-s (--strip)   - strip type - rgb, grb, gbr, rgbw\n"
                            "-d (--dma)     - dma channel to use (default 5)\n"
                            "-g (--gpio)    - GPIO to use\n"
                            "                 If omitted, default is 18 (PWM0)\n"
                            "-i (--invert)  - invert pin output (pulse LOW)\n"
                            "-v (--version) - version information\n",
                argv[0]);
            exit(-1);

        case 'f':
            if (optarg) {
                strncat(fifo_path, optarg, 4095);
            }
            break;

        case 'g':
            if (optarg) {
                int gpio = atoi(optarg);

                /*
                    PWM0, which can be set to use GPIOs 12, 18, 40, and 52.
                    Only 12 (pin 32) and 18 (pin 12) are available on the B+/2B/3B
                    PWM1 which can be set to use GPIOs 13, 19, 41, 45 and 53.
                    Only 13 is available on the B+/2B/PiZero/3B, on pin 33
                    PCM_DOUT, which can be set to use GPIOs 21 and 31.
                    Only 21 is available on the B+/2B/PiZero/3B, on pin 40.
                    SPI0-MOSI is available on GPIOs 10 and 38.
                    Only GPIO 10 is available on all models.

                    The library checks if the specified gpio is available
                    on the specific model (from model B rev 1 till 3B)

                */

                ws2811->channel[0].gpionum = gpio;
            }
            break;

        case 'i':
            ws2811->channel[0].invert = 1;
            break;

        case 'd':
            if (optarg) {
                int dma = atoi(optarg);
                if (dma < 14) {
                    ws2811->dmanum = dma;
                } else {
                    printf("invalid dma %d\n", dma);
                    exit(-1);
                }
            }
            break;

        case 's':
            if (optarg) {
                if (!strncasecmp("rgb", optarg, 4)) {
                    ws2811->channel[0].strip_type = WS2811_STRIP_RGB;
                } else if (!strncasecmp("rbg", optarg, 4)) {
                    ws2811->channel[0].strip_type = WS2811_STRIP_RBG;
                } else if (!strncasecmp("grb", optarg, 4)) {
                    ws2811->channel[0].strip_type = WS2811_STRIP_GRB;
                } else if (!strncasecmp("gbr", optarg, 4)) {
                    ws2811->channel[0].strip_type = WS2811_STRIP_GBR;
                } else if (!strncasecmp("brg", optarg, 4)) {
                    ws2811->channel[0].strip_type = WS2811_STRIP_BRG;
                } else if (!strncasecmp("bgr", optarg, 4)) {
                    ws2811->channel[0].strip_type = WS2811_STRIP_BGR;
                } else if (!strncasecmp("rgbw", optarg, 4)) {
                    ws2811->channel[0].strip_type = SK6812_STRIP_RGBW;
                } else if (!strncasecmp("grbw", optarg, 4)) {
                    ws2811->channel[0].strip_type = SK6812_STRIP_GRBW;
                } else {
                    fprintf(stderr, "invalid strip %s\n", optarg);
                    exit(-1);
                }
            }
            break;

        case 'v':
            fprintf(stderr, "%s version %s\n", argv[0], VERSION);
            exit(-1);

        case '?':
            /* getopt_long already reported error? */
            exit(-1);

        default:
            exit(-1);
        }
    }

    if (strlen(fifo_path) == 0) {
        fprintf(stderr, "Missing required option: fifo\n");
        exit(-1);
    }
}

int main(int argc, char* argv[])
{
    setup_handlers();

    int ret;

    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS) {
        fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
        return ret;
    }

    parseargs(argc, argv, &ledstring);

    if (mkfifo(fifo_path, 0666) == -1) {
        printf("Bad fifo?\n");
        printf("%s\n", fifo_path);
        exit(-1);
    }

    int tick = 0;

    int fd;

    char in[16];

    fd = open(fifo_path, O_RDONLY | O_NONBLOCK);

    while (running) {
        int bytes_read = read(fd, in, 16);
        if (bytes_read == -1) {
            if (errno == EAGAIN) {
                printf("(pipe empty)\n");
            } else {
                printf("(broken pipe)\n");
                running = 0;
            }
        } else if (bytes_read == 0) {
        } else {
            in[bytes_read] = '\0';
            printf("in:%s", in);

            sscanf(in, "%4x:%6x", &blink_rate, &color);
            printf("blink rate: %d color: %d\n", blink_rate, color);
        }

        // if blink_rate is 0, don't blink

        if (blink_rate == 0) {
            if (ledstring.channel[0].leds[0] != color) {
                ledstring.channel[0].leds[0] = color;

                ws2811_render(&ledstring);
            }
        } else if (tick > blink_rate) {
            if (ledstring.channel[0].leds[0] == color) {
                ledstring.channel[0].leds[0] = 0;
            } else {
                ledstring.channel[0].leds[0] = color;
            }

            ws2811_render(&ledstring);

            tick = 0;
        } else {
            tick++;
        }

        usleep(1000);
    }

    close(fd);

    unlink(fifo_path);

    return 0;
}

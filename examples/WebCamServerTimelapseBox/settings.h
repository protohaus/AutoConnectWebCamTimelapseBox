#define ESP_HOSTNAME "esp32timelapse" 
#define ESP_PASSWORD "12345678"

#define MIN_LENGTH_PASSWORD 8
#define MIN_LENGTH_HOSTNAME 6

#define FTP_CTRL_PORT_SD      21           // Command port on wich server is listening  
#define FTP_DATA_PORT_PASV_SD 50009        // Data port in passive mode

#define LED_TYPE    WS2812
#define LED_PIN 3
#define COLOR_ORDER GRB
#define NUM_LEDS 13
#define MAX_BRIGHTNESS 255
#define INITIAL_BRIGHTNESS_PERCENTAGE 1.0
#define LED_DELAY 500

#define MILLI_AMPS         1600 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define FRAMES_PER_SECOND  120

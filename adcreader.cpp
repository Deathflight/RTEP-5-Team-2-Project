//value1 == x axis
//value 2 == y axis

#include "adcreader.h"
#include <QDebug>

#include "gz_clk.h"
#include "gpio-sysfs.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <assert.h>

#include <cmath>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define MAX_SAMPLES 65

void ADCreader::writeReset(int fd)
{
        int ret;
        uint8_t tx1[5] = {0xff,0xff,0xff,0xff,0xff};
        uint8_t rx1[5] = {0};
        struct spi_ioc_transfer tr;
        memset(&tr,0,sizeof(struct spi_ioc_transfer));
	tr.tx_buf = (unsigned long)tx1;
	tr.rx_buf = (unsigned long)rx1;
	tr.len = ARRAY_SIZE(tx1);
	tr.delay_usecs = delay;
	tr.speed_hz = speed;
	tr.bits_per_word = bits;

        ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        if (ret < 1)
                pabort("can't send spi message1");
}


void ADCreader::writeReg(int fd, uint8_t v)
{
  int ret;
  uint8_t tx1[1];
  tx1[0] = v;
  uint8_t rx1[1] = {0};
  struct spi_ioc_transfer tr;
  memset(&tr,0,sizeof(struct spi_ioc_transfer));
  tr.tx_buf = (unsigned long)tx1;
  tr.rx_buf = (unsigned long)rx1;
  tr.len = ARRAY_SIZE(tx1);
  tr.delay_usecs = delay;
  tr.speed_hz = speed;
  tr.bits_per_word = bits;
	
        ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        if (ret < 1)
                pabort("can't send spi message2");

}

uint8_t ADCreader::readReg(int fd)
{
        int ret;
        uint8_t tx1[1];
        tx1[0] = 0;
        uint8_t rx1[1] = {0};
        struct spi_ioc_transfer tr;
	tr.tx_buf = (unsigned long)tx1;
	tr.rx_buf = (unsigned long)rx1;
	tr.len = ARRAY_SIZE(tx1);
	tr.delay_usecs = delay;
	tr.speed_hz = speed;
	tr.bits_per_word = 8;

        ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        if (ret < 1)
          pabort("can't send spi message3");
          
        return rx1[0];
}

int ADCreader::readData(int fd)
{
        int ret;
        uint8_t tx1[2] = {0,0};
        uint8_t rx1[2] = {0,0};
        struct spi_ioc_transfer tr;
        memset(&tr,0,sizeof(struct spi_ioc_transfer));	
        tr.tx_buf = (unsigned long)tx1;
	tr.rx_buf = (unsigned long)rx1;
	tr.len = ARRAY_SIZE(tx1);
	tr.delay_usecs = delay;
	tr.speed_hz = speed;
	tr.bits_per_word = 8;

        ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        if (ret < 1)
          pabort("can't send spi message4");
          
        return (rx1[0]<<8)|(rx1[1]);
}





void ADCreader::run()
{
  int ret = 0;

  // Set up ringbuffer. Due to the delay function the ringbuffer isn't really necessary, but would be helpful in case data is obtained faster than it can be plotted.
  samples = new double[MAX_SAMPLES];
  // pointer for incoming data
  pIn = samples;
  // pointer for outgoing data
  pOut = samples;

  // SPI constants
  static const char *device = "/dev/spidev0.0";
  mode = SPI_CPHA | SPI_CPOL;
  bits = 8;
  speed = 50000;
  delay = 10;
  cs0_GPIO = 8;
  cs1_GPIO = 7;


  // open SPI device
  fd = open(device, O_RDWR);
  if (fd < 0)
    pabort("can't open device");
  /*
   * spi mode
   */
  ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
  if (ret == -1)
    pabort("can't set spi mode");

  ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
  if (ret == -1)
    pabort("can't get spi mode");
  /*
   * bits per word
   */
  ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
  if (ret == -1)
    pabort("can't set bits per word");

  ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
  if (ret == -1)
    pabort("can't get bits per word");

  /*
   * max speed hz
   */
  ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
  if (ret == -1)
    pabort("can't set max speed hz");

  ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
  if (ret == -1)
    pabort("can't get max speed hz");

  fprintf(stderr, "spi mode: %d\n", mode);
  fprintf(stderr, "bits per word: %d\n", bits);
  fprintf(stderr, "max speed: %d Hz (%d KHz)\n", speed, speed/1000);

  //Some initialization is moved to the running cycle to allow for changing of channels and ADCs.


channels[0].use = true;
	channels[0].reg_clock = 0x1C; // write 00011100 : CLKDIS=1,CLOCKDIV=1,CLK=1,expects 4.9152MHz input clock
	channels[0].reg_setup = 0x40; // write 00001000 : self calibrate
	channels[1].use = true;
	channels[1].reg_clock = 0x1C; // write 00011100 : CLKDIS=1,CLOCKDIV=1,CLK=1,expects 4.9152MHz input clock
	channels[1].reg_setup = 0x40; // write 00001000 : self calibrate

  running = true;

  fprintf(stderr,"We are running!\n");
  bool sampling = false;
  int value1=0;
  int value2=0;
  pinset = false;
  while (running) {
  if(!pinset)
  {
    if(tog!=1&&tog!=0)tog=1;
	tog = !tog;
	if(tog) drdy_GPIO = 17; else drdy_GPIO = 22;
        // enable master clock for the AD
        // divisor results in roughly 4.9MHz
        // this also inits the general purpose IO
        gz_clock_ena(GZ_CLK_5MHz,5);

        // enables sysfs entry for the GPIO pin
        gpio_export(drdy_GPIO);
        // set to input
        gpio_set_dir(drdy_GPIO,0);
        // set interrupt detection to falling edge
        gpio_set_edge(drdy_GPIO,"falling");
        // get a file descriptor for the GPIO pin
        sysfs_fd = gpio_fd_open(drdy_GPIO);

	//initialize chip select pins
	gpio_export(cs0_GPIO);
	gpio_set_dir(cs0_GPIO,1);
	gpio_set_value(cs0_GPIO,tog);

	gpio_export(cs1_GPIO);
	gpio_set_dir(cs1_GPIO,1);
	gpio_set_value(cs1_GPIO,!tog);

	pinset = true;
	init = false;
  }

// If device is not in known state, reset it and rebuild state.
  if( !init ){
	// resets the AD7705 so that it expects a write to the communication register
	writeReset(fd);
	for( int i = 0 ; i < CHANNEL_N ; i+= 1 ){
		channels[i].init = false;
		}
		init = true;
  }

// Process channel, initalize if not done so already.
	if( channels[channel].use ){
		if( channels[channel].init ){
			// Tell ADC to perform a read operation (will DRDY Block until sample is ready).
			writeReg(fd,0x38 | channel);
			sampling = true;
		}else{
			// Initialize clock register.
			writeReg(fd,0x20 | channel);
			writeReg(fd,channels[channel].reg_clock);

			// Initialize setup register (will DRDY Block while calibrating).
			writeReg(fd,0x10 | channel);
			writeReg(fd,channels[channel].reg_setup);
			channels[channel].init = true;
		}
	}

		// Perform DRDY Block.
    if( (ret = gpio_poll(sysfs_fd,2000)) < 1 ) {
		fprintf(stderr,"Poll timeout, possibly the device crashed or is not connected. %d\n",ret);
		init = false;
		sampling = false;
		continue;
    }

	// If we are in the process of sampling we need to read in the data.
	if( sampling ){
		// Get value in normalized form.
		int16_t value = (int16_t) (readData(fd)-0x8000);
		//Combine and process values from both channels
		if(channel){
			value2=value;
			double formula;
			//Process values to obtain an angle
			if(abs(value1/value2)!=3.14/4)
				formula = 57*atan2(value1,value2);
			else formula = 45;
		fprintf(stderr,"-data %d       \r",value1);
			//Store processed value in buffer.
			*pIn = formula;
			if (pIn == (&samples[MAX_SAMPLES-1])) 
				pIn = samples;
			else
				pIn++;
			value1=0;
			value2=0;
		}else	value1=value;

		sampling = false;
		}

	// Determine next channel to process.
	uint8_t cchan = channel;
	do{
		channel = (channel + 1) % CHANNEL_N;
	}while( (cchan != channel) && !channels[channel].use );
	//Delay ensures enough time to switch between channels
	delay(200);
	}

close(fd);
gpio_fd_close(sysfs_fd);
}    
    


double ADCreader::getSample()
{
  assert(pOut!=pIn);
  double value = *pOut;
  if (pOut == (&samples[MAX_SAMPLES-1])) 
    pOut = samples;
  else
    pOut++;
  return value;
}


int ADCreader::hasSample()
{
  return (pOut!=pIn);
}
  

void ADCreader::quit()
{
	running = false;
	exit(0);
}

void ADCreader::pabort(const char *s)
{
        perror(s);
        abort();
}

void ADCreader::ADCselect()
{
        gpio_fd_close(sysfs_fd);

//reinitiates ADC configuration algorithm
pinset = false;
fprintf(stderr,"Click! \n");
}

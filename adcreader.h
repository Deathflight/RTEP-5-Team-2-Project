#ifndef ADCREADER
#define ADCREADER

#include <QThread>

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
// sets the clock for the AD converter
#include "gz_clk.h"
#include "gpio-sysfs.h"

// This class reads continously from the AD7705
// and stores the data in a ringbuffer
// which can be read with getSample() and
// checked if data is available with hasSample().
//
// The class needs to be started from the main
// program as a thread:
// adcreader->start();
// which is then running until the function
// adcreader->quit(); is called. 
// Then the standard method
// adcreader->wait(); needs to be called
// so that this thread has time to terminate.
// TODO: make ADC parameters configurable such as gain, sampling rate etc.
class ADCreader : public QThread
{
 public:
  ADCreader(){running = false;
		init = false;

		for( int i = 0 ; i < CHANNEL_N ; i+= 1 ){
			channels[i].use = false;
			channels[i].init = false;

		}

		channel = 0;
	};
  
  // ring buffer functions
  int hasSample();
  double getSample();

 public:
  // thread functions
  void quit();
  void run();
  void ADCselect();

 protected:
  void writeReset(int fd);
  void writeReg(int fd, uint8_t v);
  uint8_t readReg(int fd);
  int readData(int fd);
  static const uint8_t CHANNEL_N = 2;


 private:
  bool running;
  bool init;
  bool tog;
  bool pinset;
  void pabort(const char *s);
  
struct ADCChannel {
		bool use;
		bool init;

		uint8_t reg_clock;
		uint8_t reg_setup;

	} channels[CHANNEL_N];

	uint8_t channel;



  // file descriptor on the SPI interface
  int fd;

  // file descriptor on the interrupt pin
  int sysfs_fd;
  
  // data collected
  double *samples;

  // pointer to ringbuffer
  double *pIn;
  double *pOut;

  // spi constants for the ioctrl calls
  uint8_t mode;
  uint8_t bits;
  uint32_t speed;
  uint16_t delay;
  int drdy_GPIO;
  int cs0_GPIO;
  int cs1_GPIO;
};

#endif







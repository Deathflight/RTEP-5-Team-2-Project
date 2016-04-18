#include "window.h"
#include "adcreader.h"

#include <cmath>


Window::Window() :  count(0)
{

	// use the Qt signals/slots framework to update the gain -
	// every time the checkbox is ticked, the Channel function will be called
	button_1c = new QCheckBox("Channel");
	setting_1c = false;
	connect( button_1c, SIGNAL(stateChanged(int)), SLOT(Channel(int)) );


	// set up the initial plot data
	for( int index=0; index<plotDataSize; ++index )
	{
		xData[index] = index;
		yData[index] = ( M_PI * index/50 );
	}

	curve = new QwtPlotCurve;
	plot = new QwtPlot;
	// make a plot curve from the data and attach it to the plot
	curve->setSamples(xData, yData, plotDataSize);
	curve->attach(plot);

	plot->replot();
	plot->show();


	// setting layout
	// plot to the right of checkbox
	hLayout = new QHBoxLayout;
	hLayout->addWidget(button_1c);
	hLayout->addWidget(plot);

	setLayout(hLayout);
	
	plotBusy=0;
	// This is a demo for a thread which can be
	// used to read from the ADC asynchronously.
	// At the moment it doesn't do anything else than
	// running in an endless loop and which prints out "tick"
	// every second.	
	adcreader = new ADCreader();
	adcreader->start();

}

Window::~Window() {
	// tells the thread to no longer run its endless loop
	adcreader->quit();
	// wait until the run method has terminated
	adcreader->wait();
	delete adcreader;
}

void Window::timerEvent( QTimerEvent * )
{
	if(plotBusy) return;
	plotBusy = 1;
	if(adcreader->hasSample())
 	{
		double inVal = (adcreader->getSample());

		// add the new input to the plot
		memmove( yData, yData+1, (plotDataSize-1) * sizeof(double) );
		yData[plotDataSize-1] = inVal;
		curve->setSamples(xData, yData, plotDataSize);
		plot->replot();

	 }
	plotBusy=0;
}


// Function to switch between ADCs
void Window::Channel(int tog)
{
this->tog = tog;
if(tog==2) tog=1;
adcreader->ADCselect();
}

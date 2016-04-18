#ifndef WINDOW_H
#define WINDOW_H

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <QCheckBox>

#include <QBoxLayout>

#include "adcreader.h"

// class definition 'Window'
class Window : public QWidget
{
	// must include the Q_OBJECT macro for for the Qt signals/slots framework to work with this class
	Q_OBJECT
public:
	Window(); // default constructor - called when a Window is declared without arguments

	~Window();

	void timerEvent( QTimerEvent * );

public slots:
	void Channel(int tog);
// internal variables for the window class
private:
	QwtPlot      *plot;
	QwtPlotCurve *curve;
	QCheckBox *button_1c;

	// layout elements from Qt itself http://qt-project.org/doc/qt-4.8/classes.html
	QVBoxLayout  *vLayout;  // vertical layout
	QHBoxLayout  *hLayout;  // horizontal layout

	static const int plotDataSize = 100;

	// data arrays for the plot
	double xData[plotDataSize];
	double yData[plotDataSize];

	int tog;
	bool setting_1c;
	int count;
	bool plotBusy;
	ADCreader *adcreader;
};

#endif // WINDOW_H

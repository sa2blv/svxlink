#include <iostream>
#include <vector>
#include <set>

#include <qapplication.h>
#include <qwidget.h>
#include <qpainter.h>
#include <qgroupbox.h>
#include <qprogressbar.h>
#include <qlineedit.h>

#include <AsyncQtApplication.h>
#include <AsyncAudioIO.h>
#include <AsyncTimer.h>

#include "ToneDetector.h"
#include "DtmfDecoder.h"
#include "DtmfPlot.h"

using namespace std;
using namespace SigC;
using namespace Async;


set<int>	dtmf_k;
set<int>	dtmf_ko;


class BarPlot : public QWidget
{
  public:
    BarPlot(QWidget *parent, int bar_cnt) : QWidget(parent), bars(bar_cnt)
    {
      setMinimumSize(bar_cnt, 10);
      setSizePolicy(QSizePolicy::MinimumExpanding,
      	      	    QSizePolicy::MinimumExpanding);
      setBackgroundColor(black);
    }
    ~BarPlot(void) {}
    
    void setHighlight(const set<int>& highlight_set)
    {
      hl_set = highlight_set;
    }
    
    void setBar(int bar, int value)
    {
      //cout << "Setting bar " << bar << " to value " << value << endl;
      if ((bar >= 0) && ((unsigned)bar < bars.size()))
      {
      	if (value < 1)	  value = 1;
	if (value > 100)  value = 100;
      	bars[bar] = value;
      	//repaint();
      }
    }
  
  private:
    QPainter p;
    vector<int> bars;
    set<int>  	hl_set;
    
    void paintEvent(QPaintEvent *)
    {
      //cout << "Painting...\n";
      p.begin(this);                        // start painting widget
      int bar_width = width() / bars.size();
      //cout << "bar_width=" << bar_width << endl;
      for (int i=0; (unsigned)i<bars.size(); ++i)
      {
      	int bar_height = height() * bars[i] / 100;
	if (hl_set.count(i) > 0)
	{
      	  p.setPen(red);
      	  p.setBrush(red);
	}
	else
	{
      	  p.setPen(yellow);
      	  p.setBrush(yellow);
	}
      	p.drawRect(i*bar_width, height()-bar_height,
	      	   bar_width, bar_height);
      }
      p.end();                              // painting done    
    }
};


DtmfPlot *plot_win = 0;
BarPlot *bar_plot = 0;
BarPlot *overtone_plot = 0;


class Det : public ToneDetector
{
  public:
    int index;
    Det(int index, int fq, int base_N)
      : ToneDetector(fq, base_N), index(index) {}
    
};


void detValueChanged(ToneDetector *tdet, double value)
{
  Det *det = dynamic_cast<Det*>(tdet);
  bar_plot->setBar(det->index, (int)(value / 2500000000.0));
  //bar_plot->setBar(det->index, (int)(value / 15000));
}


void overtoneValueChanged(ToneDetector *tdet, double value)
{
  Det *det = dynamic_cast<Det*>(tdet);
  overtone_plot->setBar(det->index, (int)(value / 2500000000.0));
  //overtone_plot->setBar(det->index, (int)(value / 15000));
}


void repaint(Timer *t)
{
  bar_plot->repaint();
  overtone_plot->repaint();
  if (t != 0)
  {
    t->reset();
  }
  else
  {
    t = new Timer(100);
    t->expired.connect(slot(&repaint));
  }
}

void dtmf_digit_detected(char digit)
{
  //printf("DTMF digit detected: %c\n", digit);
  char str[2];
  str[0] = digit;
  str[1] = 0;
  plot_win->digit_view->setCursorPosition(
      	  plot_win->digit_view->text().length());
  plot_win->digit_view->insert(str);
} /*  */


int update_peak_meter(short *samples, int count)
{
  static short peak = 0;
  static int tot_count = 0;
  
  for (int i=0; i<count; ++i)
  {
    if (abs(samples[i]) > peak)
    {
      peak = abs(samples[i]);
    }
    if (++tot_count >= 800)
    {
      tot_count = 0;
      plot_win->peak_meter->setProgress(peak);
      //printf("peak=%d\n", peak);
      if (peak >= 32700)
      {
      	plot_win->max_peak_indicator->setBackgroundColor("red");
      }
      else
      {
      	plot_win->max_peak_indicator->setBackgroundColor("black");      
      }
      peak = 0;
    }
  }
  
  return count;
  
} /*  */


int main(int argc, char **argv)
{
  //QApplication app(argc, argv);
  QtApplication app(argc, argv);
  
  const int start_k = 15;
  const int end_k = 50;
  const int start_ko = 32;
  const int end_ko = 85;
  
  dtmf_k.insert(18-start_k);
  dtmf_k.insert(20-start_k);
  dtmf_k.insert(22-start_k);
  dtmf_k.insert(24-start_k);
  
  dtmf_k.insert(31-start_k);
  dtmf_k.insert(34-start_k);
  dtmf_k.insert(38-start_k);
  dtmf_k.insert(42-start_k);
  
  dtmf_ko.insert(35-start_ko);
  dtmf_ko.insert(39-start_ko);
  dtmf_ko.insert(43-start_ko);
  dtmf_ko.insert(47-start_ko);
  
  dtmf_ko.insert(61-start_ko);
  dtmf_ko.insert(67-start_ko);
  dtmf_ko.insert(74-start_ko);
  dtmf_ko.insert(82-start_ko);
  
  plot_win = new DtmfPlot;
  plot_win->base_plot->setColumns(1);
  plot_win->overtone_plot->setColumns(1);
  
  bar_plot = new BarPlot(plot_win->base_plot, end_k-start_k+1);
  bar_plot->setHighlight(dtmf_k);
  overtone_plot = new BarPlot(plot_win->overtone_plot, end_ko-start_ko+1);
  overtone_plot->setHighlight(dtmf_ko);
  
  AudioIO *audio_io = new AudioIO("/dev/dsp");
  audio_io->open(AudioIO::MODE_RD);
  
  ToneDetector *det;
  
  for (int k=start_k; k<end_k+1; ++k)
  {
    det = new Det(k-start_k,k*8000/205+8000/410, 205);
    det->valueChanged.connect(slot(&detValueChanged));
    audio_io->audioRead.connect(slot(det, &ToneDetector::processSamples));
  }
  
  for (int k=start_ko; k<end_ko+1; ++k)
  {
    det = new Det(k-start_ko,k*8000/201+8000/402, 201);
    det->valueChanged.connect(slot(&overtoneValueChanged));
    audio_io->audioRead.connect(slot(det, &ToneDetector::processSamples));
  }
  
  audio_io->audioRead.connect(slot(&update_peak_meter));
  
  DtmfDecoder dtmf_dec;
  audio_io->audioRead.connect(slot(&dtmf_dec, &DtmfDecoder::processSamples));
  dtmf_dec.digitDetected.connect(slot(&dtmf_digit_detected));
  
  repaint(0);
  
  app.setMainWidget(plot_win);
  plot_win->show();
  app.exec();

}


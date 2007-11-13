/**********************************************************************

  Audacity: A Digital Audio Editor

  FreqWindow.cpp

  Dominic Mazzoni

*******************************************************************//**

\class FreqWindow
\brief Displays a spectrum plot of the waveform.  Has options for
selecting parameters of the plot.

Has a feature that finds peaks and reports their value as you move
the mouse around.

*//****************************************************************//**

\class FreqPlot
\brief Works with FreqWindow to dsplay a spectrum plot of the waveform.
This class actually does the graph display.

Has a feature that finds peaks and reports their value as you move
the mouse around.

*//*******************************************************************/

/*
  Salvo Ventura - November 2006
  Extended range check for additional FFT windows
*/


#include "Audacity.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>



#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/brush.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/image.h>
#include <wx/dcmemory.h>
#include <wx/msgdlg.h>
#include <wx/file.h>
#include <wx/filedlg.h>
#include <wx/intl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statusbr.h>
#endif

#include <wx/textfile.h>

#include <math.h>

#include "FreqWindow.h"

#include "AColor.h"
#include "FFT.h"
#include "Internat.h"
#include "PitchName.h"
#include "Theme.h"
#include "AllThemeResources.h"

#include "FileDialog.h"

enum {
   FirstID = 7000,

   FreqCloseButtonID,
   FreqExportButtonID,
   FreqAlgChoiceID,
   FreqSizeChoiceID,
   FreqFuncChoiceID,
   FreqAxisChoiceID
};

FreqWindow *gFreqWindow = NULL;

// These specify the minimum plot window width

#define FREQ_WINDOW_WIDTH 480
#define FREQ_WINDOW_HEIGHT 330

void InitFreqWindow(wxWindow * parent)
{
   if (gFreqWindow)
      return;

   wxPoint where;

   where.x = 150;
   where.y = 150;

   gFreqWindow = new FreqWindow(parent, -1, _("Frequency Analysis"), where);
}

// FreqWindow

BEGIN_EVENT_TABLE(FreqWindow, wxDialog)
    EVT_CLOSE(FreqWindow::OnCloseWindow)
    EVT_SIZE(FreqWindow::OnSize)
    EVT_BUTTON(FreqCloseButtonID, FreqWindow::OnCloseButton)
    EVT_BUTTON(FreqExportButtonID, FreqWindow::OnExport)
    EVT_CHOICE(FreqAlgChoiceID, FreqWindow::OnAlgChoice)
    EVT_CHOICE(FreqSizeChoiceID, FreqWindow::OnSizeChoice)
    EVT_CHOICE(FreqFuncChoiceID, FreqWindow::OnFuncChoice)
    EVT_CHOICE(FreqAxisChoiceID, FreqWindow::OnAxisChoice)
END_EVENT_TABLE()

FreqWindow::FreqWindow(wxWindow * parent, wxWindowID id,
                           const wxString & title,
                           const wxPoint & pos):
  wxDialog(parent, id, title, pos, wxDefaultSize,
     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX ),
  mData(NULL), mProcessed(NULL), mBitmap(NULL)
{
   mMouseX = 0;
   mMouseY = 0;

   mFreqFont = wxFont(fontSize, wxSWISS, wxNORMAL, wxNORMAL);
   mArrowCursor = new wxCursor(wxCURSOR_ARROW);
   mCrossCursor = new wxCursor(wxCURSOR_CROSS);

   mLeftMargin = 40;
   mBottomMargin = 20;

   mFreqPlot = new FreqPlot(this, 0,
                            wxDefaultPosition, wxDefaultSize);

   wxString algChoiceStrings[5] = { _("Spectrum"),
      _("Standard Autocorrelation"),
      _("Cuberoot Autocorrelation"),
      _("Enhanced Autocorrelation"),
     /* i18n-hint: This is a technical term, derived from the word
        "spectrum".  Do not translate it unless you are sure you
        know the correct technical word in your language. */
      _("Cepstrum")
   };

   wxStaticText *algLabel = new wxStaticText(this, wxID_ANY, _("Algorithm:"));
   mAlgChoice = new wxChoice(this, FreqAlgChoiceID,
                             wxDefaultPosition, wxDefaultSize,
                             5, algChoiceStrings);

   mAlgChoice->SetSelection(0);

   wxString sizeChoiceStrings[8] = { wxT("128"),
      wxT("256"),
      wxT("512"),
      wxT("1024"),
      wxT("2048"),
      wxT("4096"),
      wxT("8192"),
      wxT("16384")
   };

   wxStaticText *sizeLabel = new wxStaticText(this, wxID_ANY,
                                              wxString(_("Size")) + wxT(":"));
   mSizeChoice = new wxChoice(this, FreqSizeChoiceID,
                              wxDefaultPosition, wxDefaultSize,
                              8, sizeChoiceStrings);

   mSizeChoice->SetSelection(2);

   int f = NumWindowFuncs();

   wxString *funcChoiceStrings = new wxString[f];
   for (int i = 0; i < f; i++) {
      /* i18n-hint: This refers to a "window function", used in the
         Frequency analyze dialog box. */
      funcChoiceStrings[i] = WindowFuncName(i) + wxString(_(" window"));
   }

   wxStaticText *funcLabel = new wxStaticText(this, wxID_ANY, _("Function:"));
   mFuncChoice = new wxChoice(this, FreqFuncChoiceID,
                              wxDefaultPosition, wxDefaultSize,
                              f, funcChoiceStrings);

   mFuncChoice->SetSelection(3);
   delete[]funcChoiceStrings;

   wxString axisChoiceStrings[2] = { _("Linear frequency"),
      _("Log frequency")
   };

   wxStaticText *axisLabel = new wxStaticText(this, wxID_ANY, _("Axis:"));
   mAxisChoice = new wxChoice(this, FreqAxisChoiceID,
                              wxDefaultPosition, wxDefaultSize,
                              2, axisChoiceStrings);

   mAxisChoice->SetSelection(0);

   mLogAxis = false;

   mExportButton = new wxButton(this, FreqExportButtonID,
                                _("&Export..."));

   mCloseButton = new wxButton(this, FreqCloseButtonID,
                               _("Close"));

#ifndef TARGET_CARBON
   mCloseButton->SetDefault();
   mCloseButton->SetFocus();
#endif

   mInfo = new wxStatusBar(this, wxID_ANY, wxST_SIZEGRIP);

   // Set minimum sizes

   mFreqPlot->SetMinSize( wxSize( FREQ_WINDOW_WIDTH, FREQ_WINDOW_HEIGHT ) );

   wxRect r( 0, 0, 0, 0 );

   r.Union( algLabel->GetRect() );
   r.Union( funcLabel->GetRect() );
   algLabel->SetMinSize( r.GetSize() );
   funcLabel->SetMinSize( r.GetSize() );

   r = wxRect( 0, 0, 0, 0 );

   r.Union( mAlgChoice->GetRect() );
   r.Union( mFuncChoice->GetRect() );
   mAlgChoice->SetMinSize( r.GetSize() );
   mFuncChoice->SetMinSize( r.GetSize() );

   r = wxRect( 0, 0, 0, 0 );

   r.Union( sizeLabel->GetRect() );
   r.Union( axisLabel->GetRect() );
   sizeLabel->SetMinSize( r.GetSize() );
   axisLabel->SetMinSize( r.GetSize() );

   r = wxRect( 0, 0, 0, 0 );

   r.Union( mSizeChoice->GetRect() );
   r.Union( mAxisChoice->GetRect() );
   mSizeChoice->SetMinSize( r.GetSize() );
   mAxisChoice->SetMinSize( r.GetSize() );

   r = wxRect( 0, 0, 0, 0 );

   r.Union( mExportButton->GetRect() );
   r.Union( mCloseButton->GetRect() );
   mExportButton->SetMinSize( r.GetSize() );
   mCloseButton->SetMinSize( r.GetSize() );

   // Add everything to the sizers

   wxBoxSizer *vs = new wxBoxSizer( wxVERTICAL );
   wxBoxSizer *hs;

   szr = new wxFlexGridSizer(2);
   szr->AddGrowableCol( 1, 1 );
   szr->AddGrowableRow( 0, 1 );
   szr->SetFlexibleDirection( wxBOTH );

   vRuler = new RulerPanel(this, wxID_ANY);
   vRuler->ruler.SetBounds(0, 0, 100, 100); // Ruler can't handle small sizes
   vRuler->ruler.SetOrientation(wxVERTICAL);
   vRuler->ruler.SetRange(10.0, -90.0);
   vRuler->ruler.SetFormat(Ruler::LinearDBFormat);
   vRuler->ruler.SetUnits(_("dB"));
   vRuler->ruler.SetLabelEdges(true);
   int w, h;
   vRuler->ruler.GetMaxSize(&w, NULL);
   vRuler->SetSize(wxSize(w, 150));  // height needed for wxGTK
   szr->Add( vRuler, 0, wxEXPAND|wxTOP|wxBOTTOM, 1 ); //for border around graph
   szr->Add( mFreqPlot, 1, wxEXPAND );
   szr->Add(1,1); //spacer on next row, under vRuler

   hRuler  = new RulerPanel(this, wxID_ANY);
   hRuler->ruler.SetBounds(0, 0, 100, 100); // Ruler can't handle small sizes
   hRuler->ruler.SetOrientation(wxHORIZONTAL);
   hRuler->ruler.SetLog(false);  //default for freq axis
   hRuler->ruler.SetRange(10, 20000);  //dummy values - will get replaced
   hRuler->ruler.SetFormat(Ruler::RealFormat);
   hRuler->ruler.SetUnits(_("Hz"));
   hRuler->ruler.SetFlip(true);
   hRuler->ruler.SetLabelEdges(false);
   hRuler->ruler.GetMaxSize(NULL, &h);
   hRuler->SetMinSize(wxSize(-1, h));
   szr->Add( hRuler, 0, wxEXPAND|wxLEFT|wxRIGHT, 1 ); //for border around graph
   szr->Add(1,1); //spacer
   mInfoText = new wxStaticText(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);   //box for info text
   szr->Add( mInfoText, 0, wxEXPAND|wxALL, 5);

   vs->Add(szr, 1, wxEXPAND|wxALL, 5);

   vs->Add( 1, 5, 0 );

   hs = new wxBoxSizer( wxHORIZONTAL );
   hs->Add( algLabel, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 5 );
   hs->Add( mAlgChoice, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 5 );
   hs->Add( sizeLabel, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 5 );
   hs->Add( mSizeChoice, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 5 );
   hs->Add( 10, 1, 0 );
   hs->Add( mExportButton, 0, wxALIGN_CENTER | wxALIGN_RIGHT | wxLEFT | wxRIGHT, 5 );
   vs->Add( hs, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

   hs = new wxBoxSizer( wxHORIZONTAL );
   hs->Add( funcLabel, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 5 );
   hs->Add( mFuncChoice, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 5 );
   hs->Add( axisLabel, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 5 );
   hs->Add( mAxisChoice, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 5 );
   hs->Add( 10, 1, 0 );
   hs->Add( mCloseButton, 0, wxALIGN_CENTER | wxALIGN_RIGHT | wxLEFT | wxRIGHT, 5 );
   vs->Add( hs, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

   vs->Add( mInfo, 0, wxEXPAND | wxBOTTOM, 0 );

   SetAutoLayout( false );
   SetSizerAndFit( vs );
   Layout();
}

FreqWindow::~FreqWindow()
{
   delete mFreqPlot;
   if (mBitmap)
      delete mBitmap;
   delete mCloseButton;
   delete mAlgChoice;
   delete mSizeChoice;
   delete mFuncChoice;
   delete mArrowCursor;
   delete mCrossCursor;
   if (mData)
      delete[]mData;
}

void FreqWindow::OnSize(wxSizeEvent & event)
{
   Layout();

   mUpdateRect.x = 0;
   mUpdateRect.y = 0;
   mUpdateRect.SetSize( mFreqPlot->GetSize() );
   mPlotRect = mUpdateRect;

   DrawPlot();

   Refresh(true);
}

void FreqWindow::DrawPlot()
{
   if (mUpdateRect.width == 0 || mUpdateRect.height == 0)
   {
      // Update rect not yet initialized properly
      // (can happen during initialization phase on wx2.8)
      return;
   }

   if (mBitmap)
   {
      delete mBitmap;
      mBitmap = NULL;
   }

   mBitmap = new wxBitmap(mUpdateRect.width, mUpdateRect.height);
   
   wxMemoryDC memDC;
   memDC.SelectObject(*mBitmap);

   memDC.SetBackground(wxBrush(wxColour(254, 254, 254)));// DONT-THEM Mask colour.
   memDC.Clear();

   wxRect r = mPlotRect;

   memDC.SetPen(*wxBLACK_PEN);
   memDC.SetBrush(*wxWHITE_BRUSH);
   memDC.DrawRectangle(r);

   if (!mProcessed) {
      if (mData && mDataLen < mWindowSize)
         memDC.DrawText(_("Not enough data selected."), r.x + 5, r.y + 5);

      return;
   }

   float yTotal = (mYMax - mYMin);

   int alg = mAlgChoice->GetSelection();

   int i;

   memDC.SetFont(mFreqFont);

   // Set up y axis ruler

   if (alg == 0) {
      vRuler->ruler.SetUnits(_("dB"));
      vRuler->ruler.SetFormat(Ruler::LinearDBFormat);
   } else {
      vRuler->ruler.SetUnits(wxT(""));
      vRuler->ruler.SetFormat(Ruler::RealFormat);
   }
   int w1, w2, h;
   vRuler->ruler.GetMaxSize(&w1, &h);
   vRuler->ruler.SetRange(mYMax, mYMin);
   vRuler->ruler.GetMaxSize(&w2, &h);
   if( w1 != w2 )   // Reduces flicker
   {
      vRuler->SetSize(wxSize(w2,h));
      szr->Layout();
      hRuler->Refresh(false);
   }
   vRuler->Refresh(false);

   // Set up x axis ruler

   int width = r.width - 2;

   float xMin, xMax, xPos, xRatio, xLast, xStep;

   if (alg == 0) {
      xMin = mRate / mWindowSize;
      xMax = mRate / 2;
      xRatio = xMax / xMin;
      xPos = xMin;
      xLast = xPos / 2.0;
      if (mLogAxis)
      {
         xStep = pow(2.0f, (log(xRatio) / log(2.0f)) / width);
         hRuler->ruler.SetLog(true);
      }
      else
      {
         xStep = (xMax - xMin) / width;
         hRuler->ruler.SetLog(false);
      }
      hRuler->ruler.SetUnits(_("Hz"));
   } else {
      xMin = 0;
      xMax = mProcessedSize / mRate;
      xPos = xMin;
      xLast = xPos / 2.0;
      xStep = (xMax - xMin) / width;
      hRuler->ruler.SetLog(false);
      hRuler->ruler.SetUnits(_("s"));
   }
   hRuler->ruler.SetRange(xMin, xMax-xStep);
   hRuler->Refresh(false);

   // Draw the plot

   if (alg == 0)
      memDC.SetPen(wxPen(theTheme.Colour( clrHzPlot ), 1, wxSOLID));
   else
      memDC.SetPen(wxPen(theTheme.Colour( clrWavelengthPlot), 1, wxSOLID));

   xPos = xMin;

   for (i = 0; i < width; i++) {
      float y;

      if (mLogAxis)
         y = GetProcessedValue(xPos, xPos * xStep);
      else
         y = GetProcessedValue(xPos, xPos + xStep);

      float ynorm = (y - mYMin) / yTotal;

      int lineheight = int (ynorm * (r.height - 1));

      if (lineheight > r.height - 2)
         lineheight = r.height - 2;

      if (ynorm > 0.0)
         memDC.DrawLine(r.x + 1 + i, r.y + r.height - 1 - lineheight,
                        r.x + 1 + i, r.y + r.height - 1);

      if (mLogAxis)
         xPos *= xStep;
      else
         xPos += xStep;
   }

   // Outline the graph
   memDC.SetPen(*wxBLACK_PEN);
   memDC.SetBrush(*wxTRANSPARENT_BRUSH);
   memDC.DrawRectangle(r);

   memDC.SelectObject( wxNullBitmap );

   mBitmap->SetMask( new wxMask( *mBitmap, wxColour(254, 254, 254) ) );

}


void FreqWindow::PlotMouseEvent(wxMouseEvent & event)
{
   if (event.Moving() && (event.m_x != mMouseX || event.m_y != mMouseY)) {
      mMouseX = event.m_x;
      mMouseY = event.m_y;

      wxRect r = mPlotRect;

      if (r.Inside(mMouseX, mMouseY))
         mFreqPlot->SetCursor(*mCrossCursor);
      else
         mFreqPlot->SetCursor(*mArrowCursor);

      mFreqPlot->Refresh(false);
      mInfoText->Refresh();
   }
}

void FreqWindow::OnAlgChoice(wxCommandEvent & event)
{
   // Log-frequency axis works for spectrum plots only.
   if (mAlgChoice->GetSelection() == 0) {
      mAxisChoice->Enable(true);
      mLogAxis = (mAxisChoice->GetSelection())?true:false;
   }
   else {
      mAxisChoice->Disable();
      mLogAxis = false;
   }
   Recalc();
}

void FreqWindow::OnSizeChoice(wxCommandEvent & event)
{
   Recalc();
}

void FreqWindow::OnFuncChoice(wxCommandEvent & event)
{
   Recalc();
}

void FreqWindow::OnAxisChoice(wxCommandEvent & event)
{
   mLogAxis = (mAxisChoice->GetSelection())?true:false;

   DrawPlot();
   mFreqPlot->Refresh(true);
}

// If f(0)=y0, f(1)=y1, f(2)=y2, and f(3)=y3, this function finds
// the degree-three polynomial which best fits these points and
// returns the value of this polynomial at a value x.  Usually
// 0 < x < 3

/* Declare Static functions */
static float CubicInterpolate(float y0, float y1, float y2, float y3, float x);
static float CubicMaximize(float y0, float y1, float y2, float y3, float * max);

float CubicInterpolate(float y0, float y1, float y2, float y3, float x)
{
   float a, b, c, d;

   a = y0 / -6.0 + y1 / 2.0 - y2 / 2.0 + y3 / 6.0;
   b = y0 - 5.0 * y1 / 2.0 + 2.0 * y2 - y3 / 2.0;
   c = -11.0 * y0 / 6.0 + 3.0 * y1 - 3.0 * y2 / 2.0 + y3 / 3.0;
   d = y0;

   float xx = x * x;
   float xxx = xx * x;

   return (a * xxx + b * xx + c * x + d);
}

float CubicMaximize(float y0, float y1, float y2, float y3, float * max)
{
   // Find coefficients of cubic

   float a, b, c, d;

   a = y0 / -6.0 + y1 / 2.0 - y2 / 2.0 + y3 / 6.0;
   b = y0 - 5.0 * y1 / 2.0 + 2.0 * y2 - y3 / 2.0;
   c = -11.0 * y0 / 6.0 + 3.0 * y1 - 3.0 * y2 / 2.0 + y3 / 3.0;
   d = y0;

   // Take derivative

   float da, db, dc;

   da = 3 * a;
   db = 2 * b;
   dc = c;

   // Find zeroes of derivative using quadratic equation

   float discriminant = db * db - 4 * da * dc;
   if (discriminant < 0.0)
      return float(-1.0);              // error

   float x1 = (-db + sqrt(discriminant)) / (2 * da);
   float x2 = (-db - sqrt(discriminant)) / (2 * da);

   // The one which corresponds to a local _maximum_ in the
   // cubic is the one we want - the one with a negative
   // second derivative

   float dda = 2 * da;
   float ddb = db;

   if (dda * x1 + ddb < 0)
   {
      *max = a*x1*x1*x1+b*x1*x1+c*x1+d;
      return x1;
   }
   else
   {
      *max = a*x2*x2*x2+b*x2*x2+c*x2+d;
      return x2;
   }
}

float FreqWindow::GetProcessedValue(float freq0, float freq1)
{
   int alg = mAlgChoice->GetSelection();

   float bin0, bin1, binwidth;

   if (alg == 0) {
      bin0 = freq0 * mWindowSize / mRate;
      bin1 = freq1 * mWindowSize / mRate;
   } else {
      bin0 = freq0 * mRate;
      bin1 = freq1 * mRate;
   }
   binwidth = bin1 - bin0;

   float value = float(0.0);

   if (binwidth < 1.0) {
      float binmid = (bin0 + bin1) / 2.0;
      int ibin = int (binmid) - 1;
      if (ibin < 1)
         ibin = 1;
      if (ibin >= mProcessedSize - 3)
         ibin = mProcessedSize - 4;

      value = CubicInterpolate(mProcessed[ibin],
                               mProcessed[ibin + 1],
                               mProcessed[ibin + 2],
                               mProcessed[ibin + 3], binmid - ibin);

   } else {
      if (int (bin1) > int (bin0))
         value += mProcessed[int (bin0)] * (int (bin0) + 1 - bin0);
      bin0 = 1 + int (bin0);
      while (bin0 < int (bin1)) {
         value += mProcessed[int (bin0)];
         bin0 += 1.0;
      }
      value += mProcessed[int (bin1)] * (bin1 - int (bin1));

      value /= binwidth;
   }

   return value;
}

void FreqWindow::PlotPaint(wxPaintEvent & evt)
{
   wxPaintDC dc( (wxWindow *) evt.GetEventObject() );

   dc.DrawBitmap( *mBitmap, 0, 0, true );
   if( mProcessed == NULL )
      return;

   int alg = mAlgChoice->GetSelection();

   dc.SetFont(mFreqFont);

   wxRect r = mPlotRect;

   int width = r.width - 2;

   float xMin, xMax, xPos, xRatio, xLast, xStep;

   if (alg == 0) {
      xMin = mRate / mWindowSize;
      xMax = mRate / 2;
      xRatio = xMax / xMin;
      xPos = xMin;
      xLast = xPos / 2.0;
      if (mLogAxis)
         xStep = pow(2.0f, (log(xRatio) / log(2.0f)) / width);
      else
         xStep = (xMax - xMin) / width;
   } else {
      xMin = 0;
      xMax = mProcessedSize / mRate;
      xPos = xMin;
      xLast = xPos / 2.0;
      xStep = (xMax - xMin) / width;
   }

   // Find the peak nearest the cursor and plot it

   float bestpeak = float(0.0);
   if ( r.Inside(mMouseX, mMouseY) & (mMouseX!=0) & (mMouseX!=r.width-1) ) {
      if (mLogAxis)
         xPos = xMin * pow(xStep, mMouseX - (r.x + 1));
      else
         xPos = xMin + xStep * (mMouseX - (r.x + 1));

      bool up = (mProcessed[1] > mProcessed[0]);
      float bestdist = 1000000;
      float bestValue = 0.0;
      for (int bin = 2; bin < mProcessedSize; bin++) {
         bool nowUp = mProcessed[bin] > mProcessed[bin - 1];
         if (!nowUp && up) {
            // Local maximum.  Find actual value by cubic interpolation
            int leftbin = bin - 2;
            if (leftbin < 1)
               leftbin = 1;
            float valueAtMax = 0.0;
            float max = leftbin + CubicMaximize(mProcessed[leftbin],
                                                mProcessed[leftbin + 1],
                                                mProcessed[leftbin + 2],
                                                mProcessed[leftbin + 3], &valueAtMax);

            float thispeak;
            if (alg == 0)
               thispeak = max * mRate / mWindowSize;
            else
               thispeak = max / mRate;

            if (fabs(thispeak - xPos) < bestdist) {
               bestpeak = thispeak;
               bestdist = fabs(thispeak - xPos);
               bestValue = valueAtMax;
               if (thispeak > xPos)
                  break;
            }
         }
         up = nowUp;
      }

      int px;
      if (mLogAxis)
         px = int (log(bestpeak / xMin) / log(xStep));
      else
         px = int ((bestpeak - xMin) * width / (xMax - xMin));

      dc.SetPen(wxPen(wxColour(160,160,160), 1, wxSOLID));
      dc.DrawLine(r.x + 1 + px, r.y, r.x + 1 + px, r.y + r.height);

       // print out info about the cursor location

      float value;

      if (mLogAxis) {
         xPos = xMin * pow(xStep, mMouseX - (r.x + 1));
         value = GetProcessedValue(xPos, xPos * xStep);
      } else {
         xPos = xMin + xStep * (mMouseX - (r.x + 1));
         value = GetProcessedValue(xPos, xPos + xStep);
      }

      wxString info;
      wxString xpitch;
      wxString peakpitch;
      const wxChar *xp;
      const wxChar *pp;

      if (alg == 0) {
         xpitch = PitchName_Absolute(Freq2Pitch(xPos));
         peakpitch = PitchName_Absolute(Freq2Pitch(bestpeak));
         xp = xpitch.c_str();
         pp = peakpitch.c_str();

         info.Printf(_("Cursor: %d Hz (%s) = %d dB    Peak: %d Hz (%s) = %.1f dB"),
               int (xPos + 0.5), xp,
               int (value + 0.5), int (bestpeak + 0.5),
               pp, bestValue);
      } else if (xPos > 0.0 && bestpeak > 0.0) {
         xpitch = PitchName_Absolute(Freq2Pitch(1.0 / xPos));
         peakpitch = PitchName_Absolute(Freq2Pitch(1.0 / bestpeak));
         xp = xpitch.c_str();
         pp = peakpitch.c_str();
         info.Printf(_("Cursor: %.4f sec (%d Hz) (%s) = %f,    Peak: %.4f sec (%d Hz) (%s) = %.3f"),
                     xPos,
                     int (1.0 / xPos + 0.5),
                     xp, value, bestpeak, int (1.0 / bestpeak + 0.5), pp, bestValue);
      }
      mInfoText->SetLabel(info);
   }
   else
      mInfoText->SetLabel(wxT(""));


   // Outline the graph
   dc.SetPen(*wxBLACK_PEN);
   dc.SetBrush(*wxTRANSPARENT_BRUSH);
   dc.DrawRectangle(r);
}

void FreqWindow::OnCloseWindow(wxCloseEvent & WXUNUSED(event))
{
   this->Show(FALSE);
}

void FreqWindow::OnCloseButton(wxCommandEvent & WXUNUSED(event))
{
   this->Show(FALSE);
}

void FreqWindow::Plot(int len, float *data, double rate)
{
   mRate = rate;
   mDataLen = len;
   if (mData)
      delete[]mData;
   mData = new float[len];
   for (int i = 0; i < len; i++)
      mData[i] = data[i];
   Recalc();
   wxSizeEvent dummy;
   OnSize( dummy );
}

void FreqWindow::Recalc()
{
   if (mProcessed)
      delete mProcessed;
   mProcessed = NULL;

   if (!mData) {
      mFreqPlot->Refresh(true);
      return;
   }

   int alg = mAlgChoice->GetSelection();
   int windowFunc = mFuncChoice->GetSelection();
   long windowSize = 0;
   (mSizeChoice->GetStringSelection()).ToLong(&windowSize);

   if (!(windowSize >= 32 && windowSize <= 65536 &&
         alg >= 0 && alg <= 4 && windowFunc >= 0 && windowFunc <= 9)) {
      mFreqPlot->Refresh(true);
      return;
   }

   mWindowSize = windowSize;

   if (mDataLen < mWindowSize) {
      mFreqPlot->Refresh(true);
      return;
   }

   mProcessed = new float[mWindowSize];

   int i;
   for (i = 0; i < mWindowSize; i++)
      mProcessed[i] = float(0.0);
   int half = mWindowSize / 2;

   float *in = new float[mWindowSize];
   float *in2 = new float[mWindowSize];
   float *out = new float[mWindowSize];
   float *out2 = new float[mWindowSize];
   float *win = new float[mWindowSize];

   // initialize the window
   for(int i=0; i<mWindowSize; i++)
      win[i] = 1.0;
   WindowFunc(windowFunc, mWindowSize, win);

   int start = 0;
   int windows = 0;
   while (start + mWindowSize <= mDataLen) {
      for (i = 0; i < mWindowSize; i++)
         in[i] = win[i] * mData[start + i];

      switch (alg) {
      case 0:                  // Spectrum
         PowerSpectrum(mWindowSize, in, out);

         for (i = 0; i < half; i++)
            mProcessed[i] += out[i];
         break;

      case 1:
      case 2:
      case 3:   // Autocorrelation, Cuberoot AC or Enhanced AC

         // Take FFT
         FFT(mWindowSize, false, in, NULL, out, out2);

         // Compute power
         for (i = 0; i < mWindowSize; i++)
            in[i] = (out[i] * out[i]) + (out2[i] * out2[i]);

         if (alg == 1) {
            for (i = 0; i < mWindowSize; i++)
               in[i] = sqrt(in[i]);
         }
         if (alg == 2 || alg == 3) {
            // Tolonen and Karjalainen recommend taking the cube root
            // of the power, instead of the square root

            for (i = 0; i < mWindowSize; i++)
               in[i] = pow(in[i], 1.0f / 3.0f);
         }
         // Take FFT
         FFT(mWindowSize, false, in, NULL, out, out2);

         // Take real part of result
         for (i = 0; i < half; i++)
            mProcessed[i] += out[i];
         break;

      case 4:                  // Cepstrum
         FFT(mWindowSize, false, in, NULL, out, out2);

         // Compute log power
         float power;
         for (i = 0; i < mWindowSize; i++)
         {
            power = (out[i] * out[i]) + (out2[i] * out2[i]);
            if(power <= 0.)
               in[i] = -100000.;
            else
               in[i] = log((out[i] * out[i]) + (out2[i] * out2[i]));
         }
         // Take IFFT
         FFT(mWindowSize, true, in, NULL, out, out2);

         // Take real part of result
         for (i = 0; i < half; i++)
            mProcessed[i] += out[i];

         break;
      }                         //switch

      start += half;
      windows++;
   }

   switch (alg) {
   case 0:                     // Spectrum
      // Convert to decibels
      mYMin = 1000000.;
      mYMax = -1000000.;
      for (i = 0; i < half; i++)
      {
         mProcessed[i] = 10 * log10(mProcessed[i] / mWindowSize / windows);
         if(mProcessed[i] > mYMax)
            mYMax = mProcessed[i];
         else if(mProcessed[i] < mYMin)
            mYMin = mProcessed[i];
      }
      if(mYMin < -90.)
         mYMin = -90.;
      mYMax += .5;

      mProcessedSize = half;
      mYStep = 10;
      break;

   case 1:                     // Standard Autocorrelation
   case 2:                     // Cuberoot Autocorrelation
      for (i = 0; i < half; i++)
         mProcessed[i] = mProcessed[i] / windows;

      // Find min/max
      mYMin = mProcessed[0];
      mYMax = mProcessed[0];
      for (i = 1; i < half; i++)
         if (mProcessed[i] > mYMax)
            mYMax = mProcessed[i];
         else if (mProcessed[i] < mYMin)
            mYMin = mProcessed[i];

      mYStep = 1;

      mProcessedSize = half;
      break;

   case 3:                     // Enhanced Autocorrelation
      for (i = 0; i < half; i++)
         mProcessed[i] = mProcessed[i] / windows;

      // Peak Pruning as described by Tolonen and Karjalainen, 2000

      // Clip at zero, copy to temp array
      for (i = 0; i < half; i++) {
         if (mProcessed[i] < 0.0)
            mProcessed[i] = float(0.0);
         out[i] = mProcessed[i];
      }

      // Subtract a time-doubled signal (linearly interp.) from the original
      // (clipped) signal
      for (i = 0; i < half; i++)
         if ((i % 2) == 0)
            mProcessed[i] -= out[i / 2];
         else
            mProcessed[i] -= ((out[i / 2] + out[i / 2 + 1]) / 2);

      // Clip at zero again
      for (i = 0; i < half; i++)
         if (mProcessed[i] < 0.0)
            mProcessed[i] = float(0.0);

      // Find new min/max
      mYMin = mProcessed[0];
      mYMax = mProcessed[0];
      for (i = 1; i < half; i++)
         if (mProcessed[i] > mYMax)
            mYMax = mProcessed[i];
         else if (mProcessed[i] < mYMin)
            mYMin = mProcessed[i];

      mYStep = 1;

      mProcessedSize = half;
      break;

   case 4:                     // Cepstrum
      for (i = 0; i < half; i++)
         mProcessed[i] = mProcessed[i] / windows;

      // Find min/max, ignoring first and last few values
      int ignore = 4;
      mYMin = mProcessed[ignore];
      mYMax = mProcessed[ignore];
      for (i = ignore + 1; i < half - ignore; i++)
         if (mProcessed[i] > mYMax)
            mYMax = mProcessed[i];
         else if (mProcessed[i] < mYMin)
            mYMin = mProcessed[i];

      mYStep = 1;

      mProcessedSize = half;
      break;
   }

   delete[]in;
   delete[]in2;
   delete[]out;
   delete[]out2;
   delete[]win;

   DrawPlot();
   mFreqPlot->Refresh(true);
}

void FreqWindow::OnExport(wxCommandEvent & WXUNUSED(event))
{
   wxString fName = _("spectrum.txt");

   fName = FileSelector(_("Export Spectral Data As:"),
                        NULL, fName, wxT("txt"), wxT("*.txt"), wxSAVE, this);

   if (fName == wxT(""))
      return;

   wxTextFile f(fName);
#ifdef __WXMAC__
   wxFile *temp = new wxFile();
   temp->Create(fName);
   delete temp;
#else
   f.Create();
#endif
   f.Open();
   if (!f.IsOpened()) {
      wxMessageBox(_("Couldn't write to file: ") + fName);
      return;
   }

   if (mAlgChoice->GetSelection() == 0) {
      f.AddLine(_("Frequency (Hz)\tLevel (dB)"));
      for (int i = 1; i < mProcessedSize; i++)
         f.AddLine(wxString::
                   Format(wxT("%f\t%f"), i * mRate / mWindowSize,
                          mProcessed[i]));
   } else {
      f.AddLine(_("Lag (seconds)\tFrequency (Hz)\tLevel"));
      for (int i = 1; i < mProcessedSize; i++)
         f.AddLine(wxString::Format(wxT("%f\t%f\t%f"),
                                    i / mRate, mRate / i, mProcessed[i]));
   }

#ifdef __WXMAC__
   f.Write(wxTextFileType_Mac);
#else
   f.Write();
#endif
   f.Close();
}


BEGIN_EVENT_TABLE(FreqPlot, wxWindow)
    EVT_PAINT(FreqPlot::OnPaint)
    EVT_MOUSE_EVENTS(FreqPlot::OnMouseEvent)
END_EVENT_TABLE()

FreqPlot::FreqPlot(wxWindow * parent, wxWindowID id,
                       const wxPoint & pos,
                       const wxSize & size):wxWindow(parent, id, pos, size)
{
   freqWindow = (FreqWindow *) parent;
}

void FreqPlot::OnPaint(wxPaintEvent & evt)
{
   freqWindow->PlotPaint(evt);
}

void FreqPlot::OnMouseEvent(wxMouseEvent & event)
{
   freqWindow->PlotMouseEvent(event);
}

// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: 8fce6648-17b9-4632-b845-3a6d25117783


// @(#)root/hist:$Id: TGraph2D.cxx,v 1.00
// Author: Olivier Couet

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TROOT.h"
#include "TBuffer.h"
#include "TMath.h"
#include "TH2.h"
#include "TF2.h"
#include "TList.h"
#include "TGraph2D.h"
#include "TGraphDelaunay.h"
#include "TGraphDelaunay2D.h"
#include "TVirtualPad.h"
#include "TVirtualFitter.h"
#include "TVirtualHistPainter.h"
#include "TPluginManager.h"
#include "TSystem.h"
#include "strtok.h"
#include "snprintf.h"

#include <cstdlib>
#include <cassert>
#include <iostream>
#include <fstream>

#include "HFitInterface.h"
#include "Fit/DataRange.h"
#include "Math/MinimizerOptions.h"

ClassImp(TGraph2D);


/** \class TGraph2D
    \ingroup Graphs
Graphics object made of three arrays X, Y and Z with the same number of points each.

- [Creating a TGraph2D](\ref G2D00)
- [Drawing options](\ref G2D01)
- [Examples](\ref G2D02)
   - [SURF1 Example](\ref G2D021)
   - [Fitting Example](\ref G2D022)
   - [PCOL Example](\ref G2D023)
- [Definition of the Delaunay triangulation (After B. Delaunay)](\ref G2D03)


\anchor G2D00
## Creating a TGraph2D

This class has different constructors:
- With an array's dimension and three arrays x, y, and z:
~~~ {.cpp}
     auto g = new TGraph2D(n, x, y, z);
~~~
   x, y, z arrays can be doubles, floats, or ints.
- With an array's dimension only:
~~~ {.cpp}
     auto g = new TGraph2D(n);
~~~
   The internal arrays are then filled with `SetPoint()`. The following line
   fills the internal arrays at the position `i` with the values
   `x`, `y`, `z`.
~~~ {.cpp}
     g->SetPoint(i, x, y, z);
~~~
- Without parameters:
~~~ {.cpp}
     auto g = new TGraph2D();
~~~
   again `SetPoint()` must be used to fill the internal arrays.
-  From a file:
~~~ {.cpp}
     auto g = new TGraph2D("graph.dat");
~~~
   Arrays are read from the ASCII file "graph.dat" according to a specifies
   format. The default format is `%%lg %%lg %%lg`

Note that in any of these three cases, `SetPoint()` can be used to change a data
point or add a new one. If the data point index (`i`) is greater than the
current size of the internal arrays, they are automatically extended.

Like TGraph some TGraph2D constructors do not have the TGraph2D title and name as parameters.
For these constructors TGraph2D has the default title and name "Graph2D". To change the
default title and name `SetTitle` and `SetName` should be called on the TGraph2D after its
creation.

\anchor G2D01
## Drawing options

Specific drawing options can be used to paint a TGraph2D:

| Option   | Description                                                       |
|----------|-------------------------------------------------------------------|
| "TRI"    | The Delaunay triangles are drawn using filled area. An hidden surface drawing technique is used. The surface is painted with the current fill area color. The edges of each triangles are painted with the current line color. |
| "TRIW"   | The Delaunay triangles are drawn as wire frame. |
| "TRI1"   | The Delaunay triangles are painted with color levels. The edges of each triangles are painted with the current line color. |
| "TRI2"   | The Delaunay triangles are painted with color levels. |
| "P"      | Draw a marker at each vertex. |
| "P0"     | Draw a circle at each vertex. Each circle background is white. |
| "PCOL"   | Draw a marker at each vertex. The color of each marker is defined according to its Z position. |
| "LINE"   | Draw a 3D polyline. |
| "CONT5"  | Draw a contour plot using Delaunay triangles.|

The Delaunay triangulation algorithm assumes that each (x, y) coordinate corresponds to a unique z value,
meaning duplicate (x, y) points are not allowed. Consequently, when using drawing options that rely on this
algorithm (e.g., TRI, SURF, etc.), a warning may appear instructing you to remove duplicates
(see RemoveDuplicates()).

A TGraph2D can be also drawn with any options valid to draw a 2D histogram
(like `COL`, `SURF`, `LEGO`, `CONT` etc..).

When a TGraph2D is drawn with one of the 2D histogram drawing option,
an intermediate 2D histogram is filled using the Delaunay triangles
to interpolate the data set. The 2D histogram has equidistant bins along the X
and Y directions. The number of bins along each direction can be change using
`SetNpx()` and `SetNpy()`. Each bin is filled with the Z
value found via a linear interpolation on the plane defined by the triangle above
the (X,Y) coordinates of the bin center.

The existing (X,Y,Z) points can be randomly scattered.
The Delaunay triangles are build in the (X,Y) plane. These 2D triangles are then
used to define flat planes in (X,Y,Z) over which the interpolation is done to fill
the 2D histogram. The 3D triangles int takes build a 3D surface in
the form of tessellating triangles at various angles. The triangles found can be
drawn in 3D with one of the TGraph2D specific drawing options.

The histogram generated by the Delaunay interpolation can be accessed using the
`GetHistogram()` method.

The axis settings (title, ranges etc ...) can be changed accessing the axis via
the GetXaxis GetYaxis and GetZaxis methods. They access the histogram axis created
at drawing time only. Therefore they should called after the TGraph2D is drawn:

~~~ {.cpp}
     auto g = new TGraph2D();

     [...]

     g->Draw("tri1");
     gPad->Update();
     g->GetXaxis()->SetTitle("X axis title");
~~~

\anchor G2D02
## Examples

\anchor G2D021
### SURF1 Example

Begin_Macro(source)
{
   auto c = new TCanvas("c","Graph2D example",0,0,600,400);
   Double_t x, y, z, P = 6.;
   Int_t np = 200;
   auto dt = new TGraph2D();
   dt->SetTitle("Graph title; X axis title; Y axis title; Z axis title");
   auto r = new TRandom();
   for (Int_t N=0; N<np; N++) {
      x = 2*P*(r->Rndm(N))-P;
      y = 2*P*(r->Rndm(N))-P;
      z = (sin(x)/x)*(sin(y)/y)+0.2;
      dt->SetPoint(N,x,y,z);
   }
   gStyle->SetPalette(1);
   dt->Draw("surf1");
}
End_Macro

\anchor G2D022
### Fitting Example

2D graphs can be fitted as shown by the following example:

Begin_Macro(source)
../../../tutorials/math/fit/graph2dfit.C
End_Macro

\anchor G2D023
### PCOL Example

Example showing the PCOL option.

Begin_Macro(source)
{
   auto c = new TCanvas("c","Graph2D example",0,0,600,400);
   Double_t P = 5.;
   Int_t npx  = 20 ;
   Int_t npy  = 20 ;
   Double_t x = -P;
   Double_t y = -P;
   Double_t z;
   Int_t k = 0;
   Double_t dx = (2*P)/npx;
   Double_t dy = (2*P)/npy;
   auto dt = new TGraph2D(npx*npy);
   dt->SetNpy(41);
   dt->SetNpx(40);
   for (Int_t i=0; i<npx; i++) {
      for (Int_t j=0; j<npy; j++) {
         z = sin(sqrt(x*x+y*y))+1;
         dt->SetPoint(k,x,y,z);
         k++;
         y = y+dy;
      }
      x = x+dx;
      y = -P;
   }
   gStyle->SetPalette(1);
   dt->SetMarkerStyle(20);
   dt->Draw("pcol");
}
End_Macro

\anchor G2D03
## Definition of the Delaunay triangulation (After B. Delaunay)

For a set S of points in the Euclidean plane, the unique triangulation DT(S)
of S such that no point in S is inside the circumcircle of any triangle in
DT(S). DT(S) is the dual of the Voronoi diagram of S.
If n is the number of points in S, the Voronoi diagram of S is the partitioning
of the plane containing S points into n convex polygons such that each polygon
contains exactly one point and every point in a given polygon is closer to its
central point than to any other. A Voronoi diagram is sometimes also known as
a Dirichlet tessellation.

\image html tgraph2d_delaunay.png

[This applet](http://www.cs.cornell.edu/Info/People/chew/Delaunay.html)
gives a nice practical view of Delaunay triangulation and Voronoi diagram.
*/


////////////////////////////////////////////////////////////////////////////////
/// Graph2D default constructor

TGraph2D::TGraph2D()
   : TNamed("Graph2D", "Graph2D"), TAttLine(1, 1, 1), TAttFill(0, 1001), fNpoints(0)
{
   fSize      = 0;
   fMargin    = 0.;
   fNpx       = 40;
   fNpy       = 40;
   fDirectory = nullptr;
   fHistogram = nullptr;
   fDelaunay = nullptr;
   fMaximum   = -1111;
   fMinimum   = -1111;
   fX         = nullptr;
   fY         = nullptr;
   fZ         = nullptr;
   fZout      = 0;
   fMaxIter   = 100000;
   fPainter   = nullptr;
   fFunctions = new TList;
   fUserHisto = kFALSE;
}


////////////////////////////////////////////////////////////////////////////////
/// Graph2D constructor with three vectors of ints as input.

TGraph2D::TGraph2D(Int_t n, Int_t *x, Int_t *y, Int_t *z)
   : TNamed("Graph2D", "Graph2D"), TAttLine(1, 1, 1), TAttFill(0, 1001), fNpoints(n)
{
   Build(n);

   // Copy the input vectors into local arrays
   for (Int_t i = 0; i < fNpoints; ++i) {
      fX[i] = (Double_t)x[i];
      fY[i] = (Double_t)y[i];
      fZ[i] = (Double_t)z[i];
   }
}


////////////////////////////////////////////////////////////////////////////////
/// Graph2D constructor with three vectors of floats as input.

TGraph2D::TGraph2D(Int_t n, Float_t *x, Float_t *y, Float_t *z)
   : TNamed("Graph2D", "Graph2D"), TAttLine(1, 1, 1), TAttFill(0, 1001), fNpoints(n)
{
   Build(n);

   // Copy the input vectors into local arrays
   for (Int_t i = 0; i < fNpoints; ++i) {
      fX[i] = x[i];
      fY[i] = y[i];
      fZ[i] = z[i];
   }
}


////////////////////////////////////////////////////////////////////////////////
/// Graph2D constructor with three vectors of doubles as input.

TGraph2D::TGraph2D(Int_t n, Double_t *x, Double_t *y, Double_t *z)
   : TNamed("Graph2D", "Graph2D"), TAttLine(1, 1, 1), TAttFill(0, 1001), fNpoints(n)
{
   Build(n);

   // Copy the input vectors into local arrays
   for (Int_t i = 0; i < fNpoints; ++i) {
      fX[i] = x[i];
      fY[i] = y[i];
      fZ[i] = z[i];
   }
}


////////////////////////////////////////////////////////////////////////////////
/// Graph2D constructor with a TH2 (h2) as input.
/// Only the h2's bins within the X and Y axis ranges are used.
/// Empty bins, recognized when both content and errors are zero, are excluded.

TGraph2D::TGraph2D(TH2 *h2)
   : TNamed("Graph2D", "Graph2D"), TAttLine(1, 1, 1), TAttFill(0, 1001), fNpoints(0)
{
   Build(h2->GetNbinsX()*h2->GetNbinsY());

   TString gname = "Graph2D_from_" + TString(h2->GetName());
   SetName(gname);
   // need to call later because sets title in ref histogram
   SetTitle(h2->GetTitle());

   TAxis *xaxis = h2->GetXaxis();
   TAxis *yaxis = h2->GetYaxis();
   Int_t xfirst = xaxis->GetFirst();
   Int_t xlast  = xaxis->GetLast();
   Int_t yfirst = yaxis->GetFirst();
   Int_t ylast  = yaxis->GetLast();

   Double_t x, y, z;
   Int_t k = 0;

   for (Int_t i = xfirst; i <= xlast; i++) {
      for (Int_t j = yfirst; j <= ylast; j++) {
         x = xaxis->GetBinCenter(i);
         y = yaxis->GetBinCenter(j);
         z = h2->GetBinContent(i, j);
         Double_t ez = h2->GetBinError(i, j);
         if (z != 0. || ez != 0) {
            SetPoint(k, x, y, z);
            k++;
         }
      }
   }
}


////////////////////////////////////////////////////////////////////////////////
/// Graph2D constructor with name, title and three vectors of doubles as input.
/// name   : name of 2D graph (avoid blanks)
/// title  : 2D graph title
///          if title is of the form "stringt;stringx;stringy;stringz"
///          the 2D graph title is set to stringt, the x axis title to stringx,
///          the y axis title to stringy,etc

TGraph2D::TGraph2D(const char *name, const char *title,
                   Int_t n, Double_t *x, Double_t *y, Double_t *z)
   : TNamed(name, title), TAttLine(1, 1, 1), TAttFill(0, 1001), fNpoints(n)
{
   Build(n);

   // Copy the input vectors into local arrays
   for (Int_t i = 0; i < fNpoints; ++i) {
      fX[i] = x[i];
      fY[i] = y[i];
      fZ[i] = z[i];
   }
}


////////////////////////////////////////////////////////////////////////////////
/// Graph2D constructor. The arrays fX, fY and fZ should be filled via
/// calls to SetPoint

TGraph2D::TGraph2D(Int_t n)
   : TNamed("Graph2D", "Graph2D"), TAttLine(1, 1, 1), TAttFill(0, 1001), fNpoints(n)
{
   Build(n);
   for (Int_t i = 0; i < fNpoints; i++) {
      fX[i] = 0.;
      fY[i] = 0.;
      fZ[i] = 0.;
   }
}


////////////////////////////////////////////////////////////////////////////////
/// Graph2D constructor reading input from filename
/// filename is assumed to contain at least three columns of numbers.
/// For files separated by a specific delimiter different from ' ' and '\\t' (e.g. ';' in csv files)
/// you can avoid using %*s to bypass this delimiter by explicitly specify the "option" argument,
/// e.g. option=" \\t,;" for columns of figures separated by any of these characters (' ', '\\t', ',', ';')
/// used once (e.g. "1;1") or in a combined way (" 1;,;;  1").
/// Note in that case, the instantiation is about 2 times slower.

TGraph2D::TGraph2D(const char *filename, const char *format, Option_t *option)
   : TNamed("Graph2D", filename), TAttLine(1, 1, 1), TAttFill(0, 1001), fNpoints(0)
{
   Double_t x, y, z;
   TString fname = filename;
   gSystem->ExpandPathName(fname);

   std::ifstream infile(fname.Data());
   if (!infile.good()) {
      MakeZombie();
      Error("TGraph2D", "Cannot open file: %s, TGraph2D is Zombie", filename);
      return;
   } else {
      Build(100);
   }
   std::string line;
   Int_t np = 0;

   if (strcmp(option, "") == 0) { // No delimiters specified (standard constructor).

      while (std::getline(infile, line, '\n')) {
         if (3 != sscanf(line.c_str(), format, &x, &y, &z)) {
            continue; // skip empty and ill-formed lines
         }
         SetPoint(np, x, y, z);
         np++;
      }

   } else { // A delimiter has been specified in "option"

      // Checking format and creating its boolean equivalent
      TString format_ = TString(format) ;
      format_.ReplaceAll(" ", "") ;
      format_.ReplaceAll("\t", "") ;
      format_.ReplaceAll("lg", "") ;
      format_.ReplaceAll("s", "") ;
      format_.ReplaceAll("%*", "0") ;
      format_.ReplaceAll("%", "1") ;
      if (!format_.IsDigit()) {
         Error("TGraph2D", "Incorrect input format! Allowed format tags are {\"%%lg\",\"%%*lg\" or \"%%*s\"}");
         return;
      }
      Int_t ntokens = format_.Length() ;
      if (ntokens < 3) {
         Error("TGraph2D", "Incorrect input format! Only %d tag(s) in format whereas 3 \"%%lg\" tags are expected!", ntokens);
         return;
      }
      Int_t ntokensToBeSaved = 0 ;
      Bool_t * isTokenToBeSaved = new Bool_t [ntokens] ;
      for (Int_t idx = 0; idx < ntokens; idx++) {
         isTokenToBeSaved[idx] = TString::Format("%c", format_[idx]).Atoi() ; //atoi(&format_[idx]) does not work for some reason...
         if (isTokenToBeSaved[idx] == 1) {
            ntokensToBeSaved++ ;
         }
      }
      if (ntokens >= 3 && ntokensToBeSaved != 3) { //first condition not to repeat the previous error message
         Error("TGraph2D", "Incorrect input format! There are %d \"%%lg\" tag(s) in format whereas 3 and only 3 are expected!", ntokensToBeSaved);
         delete [] isTokenToBeSaved ;
         return;
      }

      // Initializing loop variables
      Bool_t isLineToBeSkipped = kFALSE ; //empty and ill-formed lines
      char * token = nullptr ;
      TString token_str = "" ;
      Int_t token_idx = 0 ;
      Double_t * value = new Double_t [3] ;  //x,y,z buffers
      Int_t value_idx = 0 ;

      // Looping
      char *rest;
      while (std::getline(infile, line, '\n')) {
         if (!line.empty()) {
            if (line[line.size() - 1] == char(13)) {  // removing DOS CR character
               line.erase(line.end() - 1, line.end()) ;
            }
            token = R__STRTOK_R(const_cast<char*>(line.c_str()), option, &rest);
            while (token != nullptr && value_idx < 3) {
               if (isTokenToBeSaved[token_idx]) {
                  token_str = TString(token) ;
                  token_str.ReplaceAll("\t", "") ;
                  if (!token_str.IsFloat()) {
                     isLineToBeSkipped = kTRUE ;
                     break ;
                  } else {
                     value[value_idx] = token_str.Atof() ;
                     value_idx++ ;
                  }
               }
               token = R__STRTOK_R(nullptr, option, &rest); // next token
               token_idx++ ;
            }
            if (!isLineToBeSkipped && value_idx == 3) {
               x = value[0] ;
               y = value[1] ;
               z = value[2] ;
               SetPoint(np, x, y, z) ;
               np++ ;
            }
         }
         isLineToBeSkipped = kFALSE ;
         token = nullptr ;
         token_idx = 0 ;
         value_idx = 0 ;
      }

      // Cleaning
      delete [] isTokenToBeSaved ;
      delete [] value ;
      delete token ;
   }
   infile.close();
}


////////////////////////////////////////////////////////////////////////////////
/// Graph2D copy constructor.
/// copy everything apart from the list of contained functions

TGraph2D::TGraph2D(const TGraph2D &g)
: TNamed(g), TAttLine(g), TAttFill(g), TAttMarker(g),
   fX(nullptr), fY(nullptr), fZ(nullptr),
   fHistogram(nullptr), fDirectory(nullptr), fPainter(nullptr)
{
   fFunctions = new TList();   // do not copy the functions

   // use operator=
   (*this) = g;

   // append TGraph2D to gdirectory
   if (TH1::AddDirectoryStatus()) {
      fDirectory = gDirectory;
      if (fDirectory) {
         // append without replacing existing objects
         fDirectory->Append(this);
      }
   }
}


////////////////////////////////////////////////////////////////////////////////
/// TGraph2D destructor.

TGraph2D::~TGraph2D()
{
   Clear();
}


////////////////////////////////////////////////////////////////////////////////
/// Graph2D operator "="

TGraph2D& TGraph2D::operator=(const TGraph2D &g)
{
   if (this == &g) return *this;

   // delete before existing contained objects
   if (fX) delete [] fX;
   if (fY) delete [] fY;
   if (fZ) delete [] fZ;
   if (fHistogram &&  !fUserHisto) {
      delete fHistogram;
      fHistogram = nullptr;
      fDelaunay = nullptr;
   }
   // copy everything except the function list
   fNpoints = g.fNpoints;
   fNpx = g.fNpx;
   fNpy = g.fNpy;
   fMaxIter = g.fMaxIter;
   fSize = fNpoints; // force size to be the same of npoints
   fX         = (fSize > 0) ? new Double_t[fSize] : nullptr;
   fY         = (fSize > 0) ? new Double_t[fSize] : nullptr;
   fZ         = (fSize > 0) ? new Double_t[fSize] : nullptr;
   fMinimum = g.fMinimum;
   fMaximum = g.fMaximum;
   fMargin = g.fMargin;
   fZout = g.fZout;
   fUserHisto = g.fUserHisto;
   if (g.fHistogram)
      fHistogram = (fUserHisto ) ? g.fHistogram : new TH2D(*g.fHistogram);

   // copy the points
   for (Int_t n = 0; n < fSize; n++) {
      fX[n] = g.fX[n];
      fY[n] = g.fY[n];
      fZ[n] = g.fZ[n];
   }

   return *this;
}

////////////////////////////////////////////////////////////////////////////////
/// Creates the 2D graph basic data structure

void TGraph2D::Build(Int_t n)
{
   if (n <= 0) {
      Error("TGraph2D", "Invalid number of points (%d)", n);
      return;
   }

   fSize      = n;
   fMargin    = 0.;
   fNpx       = 40;
   fNpy       = 40;
   fDirectory = nullptr;
   fHistogram = nullptr;
   fDelaunay = nullptr;
   fMaximum   = -1111;
   fMinimum   = -1111;
   fX         = new Double_t[fSize];
   fY         = new Double_t[fSize];
   fZ         = new Double_t[fSize];
   fZout      = 0;
   fMaxIter   = 100000;
   fFunctions = new TList;
   fPainter   = nullptr;
   fUserHisto = kFALSE;

   if (TH1::AddDirectoryStatus()) {
      fDirectory = gDirectory;
      if (fDirectory) {
         fDirectory->Append(this, kTRUE);
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Performs the operation: `z = z + c1*f(x,y,z)`
/// Errors are not recalculated.
///
/// \param f may be a 2-D function TF2 or 3-d function TF3
/// \param c1 a scaling factor, 1 by default

void TGraph2D::Add(TF2 *f, Double_t c1)
{
   //if (fHistogram) SetBit(kResetHisto);

   for (Int_t i = 0; i < fNpoints; i++) {
      fZ[i] += c1*f->Eval(fX[i], fY[i], fZ[i]);
   }
   if (gPad) gPad->Modified();
}

////////////////////////////////////////////////////////////////////////////////
/// Apply function f to all the data points
/// f may be a 2-D function TF2 or 3-d function TF3
/// The Z values of the 2D graph are replaced by the new values computed
/// using the function

void TGraph2D::Apply(TF2 *f)
{
   //if (fHistogram) SetBit(kResetHisto);

   for (Int_t i = 0; i < fNpoints; i++) {
      fZ[i] = f->Eval(fX[i], fY[i], fZ[i]);
   }
   if (gPad) gPad->Modified();
}

////////////////////////////////////////////////////////////////////////////////
/// Browse

void TGraph2D::Browse(TBrowser *)
{
   Draw("p0");
   gPad->Update();
}


////////////////////////////////////////////////////////////////////////////////
/// Free all memory allocated by this object.

void TGraph2D::Clear(Option_t * /*option = "" */)
{
   if (fX) delete [] fX;
   fX = nullptr;
   if (fY) delete [] fY;
   fY = nullptr;
   if (fZ) delete [] fZ;
   fZ = nullptr;
   fSize = fNpoints = 0;
   if (fHistogram && !fUserHisto) {
      delete fHistogram;
      fHistogram = nullptr;
      fDelaunay = nullptr;
   }
   if (fFunctions) {
      fFunctions->SetBit(kInvalidObject);
      fFunctions->Delete();
      delete fFunctions;
      fFunctions = nullptr;
   }
   if (fDirectory) {
      fDirectory->Remove(this);
      fDirectory = nullptr;
   }
}


////////////////////////////////////////////////////////////////////////////////
/// Perform the automatic addition of the graph to the given directory
///
/// Note this function is called in place when the semantic requires
/// this object to be added to a directory (I.e. when being read from
/// a TKey or being Cloned)

void TGraph2D::DirectoryAutoAdd(TDirectory *dir)
{
   Bool_t addStatus = TH1::AddDirectoryStatus();
   if (addStatus) {
      SetDirectory(dir);
      if (dir) {
         ResetBit(kCanDelete);
      }
   }
}


////////////////////////////////////////////////////////////////////////////////
/// Computes distance from point px,py to a graph

Int_t TGraph2D::DistancetoPrimitive(Int_t px, Int_t py)
{
   Int_t distance = 9999;
   if (fHistogram) distance = fHistogram->DistancetoPrimitive(px, py);
   return distance;
}


////////////////////////////////////////////////////////////////////////////////
/// Specific drawing options can be used to paint a TGraph2D:
///
///  - "TRI"  : The Delaunay triangles are drawn using filled area.
///             An hidden surface drawing technique is used. The surface is
///             painted with the current fill area color. The edges of each
///             triangles are painted with the current line color.
///  - "TRIW" : The Delaunay triangles are drawn as wire frame
///  - "TRI1" : The Delaunay triangles are painted with color levels. The edges
///             of each triangles are painted with the current line color.
///  - "TRI2" : the Delaunay triangles are painted with color levels.
///  - "P"    : Draw a marker at each vertex
///  - "P0"   : Draw a circle at each vertex. Each circle background is white.
///  - "PCOL" : Draw a marker at each vertex. The color of each marker is
///             defined according to its Z position.
///  - "CONT" : Draw contours
///  - "LINE" : Draw a 3D polyline
///
/// A TGraph2D can be also drawn with ANY options valid to draw a 2D histogram.
///
/// When a TGraph2D is drawn with one of the 2D histogram drawing option,
/// a intermediate 2D histogram is filled using the Delaunay triangles
/// technique to interpolate the data set.

void TGraph2D::Draw(Option_t *option)
{
   TString opt = option;
   opt.ToLower();
   if (gPad) {
      if (!gPad->IsEditable()) gROOT->MakeDefCanvas();
      if (!opt.Contains("same")) {
         //the following statement is necessary in case one attempts to draw
         //a temporary histogram already in the current pad
         if (TestBit(kCanDelete)) gPad->GetListOfPrimitives()->Remove(this);
         gPad->Clear();
      }
   }
   AppendPad(opt.Data());
}


////////////////////////////////////////////////////////////////////////////////
/// Executes action corresponding to one event

void TGraph2D::ExecuteEvent(Int_t event, Int_t px, Int_t py)
{
   if (fHistogram) fHistogram->ExecuteEvent(event, px, py);
}


////////////////////////////////////////////////////////////////////////////////
/// search object named name in the list of functions

TObject *TGraph2D::FindObject(const char *name) const
{
   return fFunctions ? fFunctions->FindObject(name) : nullptr;
}


////////////////////////////////////////////////////////////////////////////////
/// search object obj in the list of functions

TObject *TGraph2D::FindObject(const TObject *obj) const
{
   return fFunctions ? fFunctions->FindObject(obj) : nullptr;
}


////////////////////////////////////////////////////////////////////////////////
/// Fits this graph with function with name fname
/// Predefined functions such as gaus, expo and poln are automatically
/// created by ROOT.
/// fname can also be a formula, accepted by the linear fitter (linear parts divided
/// by "++" sign), for example "x++sin(y)" for fitting "[0]*x+[1]*sin(y)"

TFitResultPtr TGraph2D::Fit(const char *fname, Option_t *option, Option_t *)
{

   char *linear;
   linear = (char*)strstr(fname, "++");

   if (linear) {
      TF2 f2(fname, fname);
      return Fit(&f2, option, "");
   }
   TF2 * f2 = (TF2*)gROOT->GetFunction(fname);
   if (!f2) {
      Printf("Unknown function: %s", fname);
      return -1;
   }
   return Fit(f2, option, "");

}


////////////////////////////////////////////////////////////////////////////////
/// Fits this 2D graph with function f2
///
///  f2 is an already predefined function created by TF2.
///
/// See TGraph::Fit for the available fitting options and fitting notes
///
TFitResultPtr TGraph2D::Fit(TF2 *f2, Option_t *option, Option_t *)
{
   // internal graph2D fitting methods
   Foption_t fitOption;
   Option_t *goption = "";
   ROOT::Fit::FitOptionsMake(ROOT::Fit::EFitObjectType::kGraph, option, fitOption);

   // create range and minimizer options with default values
   ROOT::Fit::DataRange range(2);
   ROOT::Math::MinimizerOptions minOption;
   return ROOT::Fit::FitObject(this, f2 , fitOption , minOption, goption, range);
}


////////////////////////////////////////////////////////////////////////////////
/// Display a GUI panel with all graph fit options.
///
///   See class TFitEditor for example

void TGraph2D::FitPanel()
{
   if (!gPad)
      gROOT->MakeDefCanvas();

   if (!gPad) {
      Error("FitPanel", "Unable to create a default canvas");
      return;
   }

   // use plugin manager to create instance of TFitEditor
   TPluginHandler *handler = gROOT->GetPluginManager()->FindHandler("TFitEditor");
   if (handler && handler->LoadPlugin() != -1) {
      if (handler->ExecPlugin(2, gPad, this) == 0)
         Error("FitPanel", "Unable to crate the FitPanel");
   } else
      Error("FitPanel", "Unable to find the FitPanel plug-in");

}


////////////////////////////////////////////////////////////////////////////////
/// Get x axis of the graph.

TAxis *TGraph2D::GetXaxis() const
{
   TH1 *h = ((TGraph2D*)this)->GetHistogram("empty");
   if (!h) return nullptr;
   return h->GetXaxis();
}


////////////////////////////////////////////////////////////////////////////////
/// Get y axis of the graph.

TAxis *TGraph2D::GetYaxis() const
{
   TH1 *h = ((TGraph2D*)this)->GetHistogram("empty");
   if (!h) return nullptr;
   return h->GetYaxis();
}


////////////////////////////////////////////////////////////////////////////////
/// Get z axis of the graph.

TAxis *TGraph2D::GetZaxis() const
{
   TH1 *h = ((TGraph2D*)this)->GetHistogram("empty");
   if (!h) return nullptr;
   return h->GetZaxis();
}


////////////////////////////////////////////////////////////////////////////////
/// Returns the X and Y graphs building a contour. A contour level may
/// consist in several parts not connected to each other. This function
/// returns them in a graphs' list.

TList *TGraph2D::GetContourList(Double_t contour)
{
   if (fNpoints <= 0) {
      Error("GetContourList", "Empty TGraph2D");
      return nullptr;
   }

   if (!fHistogram) GetHistogram("empty");

   if (!fPainter) fPainter = fHistogram->GetPainter();

   return fPainter->GetContourList(contour);
}


////////////////////////////////////////////////////////////////////////////////
/// This function is called by Graph2DFitChisquare.
/// It always returns a negative value. Real implementation in TGraph2DErrors

Double_t TGraph2D::GetErrorX(Int_t) const
{
   return -1;
}


////////////////////////////////////////////////////////////////////////////////
/// This function is called by Graph2DFitChisquare.
/// It always returns a negative value. Real implementation in TGraph2DErrors

Double_t TGraph2D::GetErrorY(Int_t) const
{
   return -1;
}


////////////////////////////////////////////////////////////////////////////////
/// This function is called by Graph2DFitChisquare.
/// It always returns a negative value. Real implementation in TGraph2DErrors

Double_t TGraph2D::GetErrorZ(Int_t) const
{
   return -1;
}


////////////////////////////////////////////////////////////////////////////////
/// Add a TGraphDelaunay in the list of the fHistogram's functions

void TGraph2D::CreateInterpolator(Bool_t oldInterp)
{

   TList *hl = fHistogram->GetListOfFunctions();

   if (oldInterp) {
      TGraphDelaunay *dt = new TGraphDelaunay(this);
      dt->SetMaxIter(fMaxIter);
      dt->SetMarginBinsContent(fZout);
      fDelaunay = dt;
      SetBit(kOldInterpolation);
      if (!hl->FindObject("TGraphDelaunay")) hl->Add(fDelaunay);
   } else {
      TGraphDelaunay2D *dt = new TGraphDelaunay2D(this);
      dt->SetMarginBinsContent(fZout);
      fDelaunay = dt;
      ResetBit(kOldInterpolation);
      if (!hl->FindObject("TGraphDelaunay2D")) hl->Add(fDelaunay);
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Return pointer to function with name.
///
/// Functions such as TGraph2D::Fit store the fitted function in the list of
/// functions of this graph.

TF2 *TGraph2D::GetFunction(const char *name) const
{
   return dynamic_cast<TF2*>(FindObject(name));
}

////////////////////////////////////////////////////////////////////////////////
/// By default returns a pointer to the Delaunay histogram. If fHistogram
/// doesn't exist, books the 2D histogram fHistogram with a margin around
/// the hull. Calls TGraphDelaunay::Interpolate at each bin centre to build up
/// an interpolated 2D histogram.
///
/// If the "empty" option is selected, returns an empty histogram booked with
/// the limits of fX, fY and fZ. This option is used when the data set is
/// drawn with markers only. In that particular case there is no need to
/// find the Delaunay triangles.
///
/// By default use the new interpolation routine based on Triangles
/// If the option "old" the old interpolation is used

TH2D *TGraph2D::GetHistogram(Option_t *option)
{
   // for an empty graph create histogram in [0,1][0,1]
   if (fNpoints <= 0) {
      if (!fHistogram) {
         // do not add the histogram to gDirectory
         TDirectory::TContext ctx(nullptr);
         fHistogram = new TH2D(GetName(), GetTitle(), fNpx , 0., 1., fNpy, 0., 1.);
         fHistogram->SetBit(TH1::kNoStats);
      }
      return fHistogram;
   }

   TString opt = option;
   opt.ToLower();
   Bool_t empty = opt.Contains("empty");
   Bool_t oldInterp = opt.Contains("old");

   if (fHistogram) {
      if (!empty && fHistogram->GetEntries() == 0) {
         if (!fUserHisto) {
            delete fHistogram;
            fHistogram = nullptr;
            fDelaunay = nullptr;
         }
      } else if (fHistogram->GetEntries() == 0)
      {;      }
         // check case if interpolation type has changed
      else if ( (TestBit(kOldInterpolation) && !oldInterp) || ( !TestBit(kOldInterpolation) && oldInterp ) ) {
         delete fHistogram;
         fHistogram = nullptr;
         fDelaunay = nullptr;
      }
      // normal case return existing histogram
      else {
         return fHistogram;
      }
   }

   Double_t hxmax, hymax, hxmin, hymin;

   // Book fHistogram if needed. It is not added in the current directory
   if (!fUserHisto) {
      Double_t xmax  = GetXmaxE();
      Double_t ymax  = GetYmaxE();
      Double_t xmin  = GetXminE();
      Double_t ymin  = GetYminE();
      hxmin = xmin - fMargin * (xmax - xmin);
      hymin = ymin - fMargin * (ymax - ymin);
      hxmax = xmax + fMargin * (xmax - xmin);
      hymax = ymax + fMargin * (ymax - ymin);
      Double_t epsilon = 1e-9;
      if (TMath::AreEqualRel(hxmax,hxmin,epsilon)) {
         if (TMath::Abs(hxmin) < epsilon) {
            hxmin = -0.001;
            hxmax =  0.001;
         } else {
            hxmin = hxmin-TMath::Abs(hxmin)*(epsilon/2.);
            hxmax = hxmax+TMath::Abs(hxmax)*(epsilon/2.);
         }
      }
      if (TMath::AreEqualRel(hymax, hymin, epsilon)) {
         if (TMath::Abs(hymin) < epsilon) {
            hymin = -0.001;
            hymax =  0.001;
         } else {
            hymin = hymin-TMath::Abs(hymin)*(epsilon/2.);
            hymax = hymax+TMath::Abs(hymax)*(epsilon/2.);
         }
      }
      if (fHistogram) {
         fHistogram->GetXaxis()->SetLimits(hxmin, hxmax);
         fHistogram->GetYaxis()->SetLimits(hymin, hymax);
      } else {
         TDirectory::TContext ctx(nullptr); // to avoid adding fHistogram to gDirectory
         fHistogram = new TH2D(GetName(), GetTitle(),
                               fNpx , hxmin, hxmax,
                               fNpy, hymin, hymax);
         CreateInterpolator(oldInterp);
      }
      fHistogram->SetBit(TH1::kNoStats);
      fHistogram->Sumw2(kFALSE);
   } else {
      hxmin = fHistogram->GetXaxis()->GetXmin();
      hymin = fHistogram->GetYaxis()->GetXmin();
      hxmax = fHistogram->GetXaxis()->GetXmax();
      hymax = fHistogram->GetYaxis()->GetXmax();
   }

   // Option "empty" is selected. An empty histogram is returned.
   Double_t hzmax, hzmin;
   if (empty) {
      if (fMinimum != -1111) {
         hzmin = fMinimum;
      } else {
         hzmin = GetZminE();
      }
      if (fMaximum != -1111) {
         hzmax = fMaximum;
      } else {
         hzmax = GetZmaxE();
      }
      if (hzmin == hzmax) {
         Double_t hz = hzmin;
         if (hz==0) {
            hzmin = -0.01;
            hzmax = 0.01;
         } else {
            hzmin = hz - 0.01 * TMath::Abs(hz);
            hzmax = hz + 0.01 * TMath::Abs(hz);
         }
      }
      fHistogram->SetMinimum(hzmin);
      fHistogram->SetMaximum(hzmax);
      return fHistogram;
   }

   Double_t dx = (hxmax - hxmin) / fNpx;
   Double_t dy = (hymax - hymin) / fNpy;

   Double_t x, y, z;

   for (Int_t ix = 1; ix <= fNpx; ix++) {
      x  = hxmin + (ix - 0.5) * dx;
      for (Int_t iy = 1; iy <= fNpy; iy++) {
         y  = hymin + (iy - 0.5) * dy;
         // do interpolation
         if (oldInterp)
            z  = ((TGraphDelaunay*)fDelaunay)->ComputeZ(x, y);
         else
            z  = ((TGraphDelaunay2D*)fDelaunay)->ComputeZ(x, y);

         fHistogram->Fill(x, y, z);
      }
   }

   hzmin = GetZminE();
   hzmax = GetZmaxE();
   if (hzmin < fHistogram->GetMinimum()) fHistogram->SetMinimum(hzmin);
   if (hzmax > fHistogram->GetMaximum()) fHistogram->SetMaximum(hzmax);

   if (fMinimum != -1111) fHistogram->SetMinimum(fMinimum);
   if (fMaximum != -1111) fHistogram->SetMaximum(fMaximum);

   return fHistogram;
}


////////////////////////////////////////////////////////////////////////////////
/// Returns the X maximum

Double_t TGraph2D::GetXmax() const
{
   Double_t v = fX[0];
   for (Int_t i = 1; i < fNpoints; i++) if (fX[i] > v) v = fX[i];
   return v;
}


////////////////////////////////////////////////////////////////////////////////
/// Returns the X minimum

Double_t TGraph2D::GetXmin() const
{
   Double_t v = fX[0];
   for (Int_t i = 1; i < fNpoints; i++) if (fX[i] < v) v = fX[i];
   return v;
}


////////////////////////////////////////////////////////////////////////////////
/// Returns the Y maximum

Double_t TGraph2D::GetYmax() const
{
   Double_t v = fY[0];
   for (Int_t i = 1; i < fNpoints; i++) if (fY[i] > v) v = fY[i];
   return v;
}


////////////////////////////////////////////////////////////////////////////////
/// Returns the Y minimum

Double_t TGraph2D::GetYmin() const
{
   Double_t v = fY[0];
   for (Int_t i = 1; i < fNpoints; i++) if (fY[i] < v) v = fY[i];
   return v;
}


////////////////////////////////////////////////////////////////////////////////
/// Returns the Z maximum

Double_t TGraph2D::GetZmax() const
{
   Double_t v = fZ[0];
   for (Int_t i = 1; i < fNpoints; i++) if (fZ[i] > v) v = fZ[i];
   return v;
}


////////////////////////////////////////////////////////////////////////////////
/// Returns the Z minimum

Double_t TGraph2D::GetZmin() const
{
   Double_t v = fZ[0];
   for (Int_t i = 1; i < fNpoints; i++) if (fZ[i] < v) v = fZ[i];
   return v;
}

////////////////////////////////////////////////////////////////////////////////
/// Get x, y and z values for point number i.
/// The function returns -1 in case of an invalid request or the point number otherwise

Int_t TGraph2D::GetPoint(Int_t i, Double_t &x, Double_t &y, Double_t &z) const
{
   if (i < 0 || i >= fNpoints) return -1;
   if (!fX || !fY || !fZ) return -1;
   x = fX[i];
   y = fY[i];
   z = fZ[i];
   return i;
}

////////////////////////////////////////////////////////////////////////////////
/// Finds the z value at the position (x,y) thanks to
/// the Delaunay interpolation.

Double_t TGraph2D::Interpolate(Double_t x, Double_t y)
{
   if (fNpoints <= 0) {
      Error("Interpolate", "Empty TGraph2D");
      return 0;
   }

   if (!fHistogram) GetHistogram("empty");
   if (!fDelaunay) {
      TList *hl = fHistogram->GetListOfFunctions();
      if (!TestBit(kOldInterpolation) ) {
         fDelaunay = hl->FindObject("TGraphDelaunay2D");
         if (!fDelaunay) fDelaunay =  hl->FindObject("TGraphDelaunay");
      }
      else {
         // if using old implementation
         fDelaunay = hl->FindObject("TGraphDelaunay");
         if (!fDelaunay) fDelaunay =  hl->FindObject("TGraphDelaunay2D");
      }
   }

   if (!fDelaunay) return TMath::QuietNaN();

   if (fDelaunay->IsA() == TGraphDelaunay2D::Class() )
      return ((TGraphDelaunay2D*)fDelaunay)->ComputeZ(x, y);
   else if (fDelaunay->IsA() == TGraphDelaunay::Class() )
      return ((TGraphDelaunay*)fDelaunay)->ComputeZ(x, y);

   // cannot be here
   assert(false);
   return TMath::QuietNaN();
}


////////////////////////////////////////////////////////////////////////////////
/// Paints this 2D graph with its current attributes

void TGraph2D::Paint(Option_t *option)
{
   if (fNpoints <= 0) {
      Error("Paint", "Empty TGraph2D");
      return;
   }

   TString opt = option;
   opt.ToLower();
   if (opt.Contains("p") && !opt.Contains("tri")) {
      if (!opt.Contains("pol") &&
          !opt.Contains("sph") &&
          !opt.Contains("psr")) opt.Append("tri0");
   }

   if (opt.Contains("line") && !opt.Contains("tri")) opt.Append("tri0");

   if (opt.Contains("err")  && !opt.Contains("tri")) opt.Append("tri0");

   if (opt.Contains("tri0")) {
      GetHistogram("empty");
   } else if (opt.Contains("old")) {
      GetHistogram("old");
   } else  {
      GetHistogram();
   }

   fHistogram->SetLineColor(GetLineColor());
   fHistogram->SetLineStyle(GetLineStyle());
   fHistogram->SetLineWidth(GetLineWidth());
   fHistogram->SetFillColor(GetFillColor());
   fHistogram->SetFillStyle(GetFillStyle());
   fHistogram->SetMarkerColor(GetMarkerColor());
   fHistogram->SetMarkerStyle(GetMarkerStyle());
   fHistogram->SetMarkerSize(GetMarkerSize());
   fHistogram->Paint(opt.Data());
}


////////////////////////////////////////////////////////////////////////////////
/// Print 2D graph values.

void TGraph2D::Print(Option_t *) const
{
   for (Int_t i = 0; i < fNpoints; i++) {
      printf("x[%d]=%g, y[%d]=%g, z[%d]=%g\n", i, fX[i], i, fY[i], i, fZ[i]);
   }
}


////////////////////////////////////////////////////////////////////////////////
/// Projects a 2-d graph into 1 or 2-d histograms depending on the option parameter.
/// option may contain a combination of the characters x,y,z:
///
///  - option = "x" return the x projection into a TH1D histogram
///  - option = "y" return the y projection into a TH1D histogram
///  - option = "xy" return the x versus y projection into a TH2D histogram
///  - option = "yx" return the y versus x projection into a TH2D histogram

TH1 *TGraph2D::Project(Option_t *option) const
{
   if (fNpoints <= 0) {
      Error("Project", "Empty TGraph2D");
      return nullptr;
   }

   TString opt = option;
   opt.ToLower();

   Int_t pcase = 0;
   if (opt.Contains("x"))  pcase = 1;
   if (opt.Contains("y"))  pcase = 2;
   if (opt.Contains("xy")) pcase = 3;
   if (opt.Contains("yx")) pcase = 4;

   // Create the projection histogram
   TH1D *h1 = nullptr;
   TH2D *h2 = nullptr;
   Int_t nch = strlen(GetName()) + opt.Length() + 2;
   char *name = new char[nch];
   snprintf(name, nch, "%s_%s", GetName(), option);
   nch = strlen(GetTitle()) + opt.Length() + 2;
   char *title = new char[nch];
   snprintf(title, nch, "%s_%s", GetTitle(), option);

   Double_t hxmin = GetXmin();
   Double_t hxmax = GetXmax();
   Double_t hymin = GetYmin();
   Double_t hymax = GetYmax();

   switch (pcase) {
      case 1:
         // "x"
         h1 = new TH1D(name, title, fNpx, hxmin, hxmax);
         break;
      case 2:
         // "y"
         h1 = new TH1D(name, title, fNpy, hymin, hymax);
         break;
      case 3:
         // "xy"
         h2 = new TH2D(name, title, fNpx, hxmin, hxmax, fNpy, hymin, hymax);
         break;
      case 4:
         // "yx"
         h2 = new TH2D(name, title, fNpy, hymin, hymax, fNpx, hxmin, hxmax);
         break;
   }

   delete [] name;
   delete [] title;
   TH1 *h = h1;
   if (h2) h = h2;
   if (h == nullptr) return nullptr;

   // Fill the projected histogram
   Double_t entries = 0;
   for (Int_t n = 0; n < fNpoints; n++) {
      switch (pcase) {
         case 1:
            // "x"
            h1->Fill(fX[n], fZ[n]);
            break;
         case 2:
            // "y"
            h1->Fill(fY[n], fZ[n]);
            break;
         case 3:
            // "xy"
            h2->Fill(fX[n], fY[n], fZ[n]);
            break;
         case 4:
            // "yx"
            h2->Fill(fY[n], fX[n], fZ[n]);
            break;
      }
      entries += fZ[n];
   }
   h->SetEntries(entries);
   return h;
}


////////////////////////////////////////////////////////////////////////////////
/// Deletes duplicated points.
///
/// The Delaunay triangulation algorithm assumes that each (x, y) coordinate corresponds to a unique z value,
/// meaning duplicate (x, y) points are not allowed. Consequently, when using drawing options that rely on this
/// algorithm (e.g., TRI, SURF, etc.), a warning may appear instructing you to remove duplicates.
/// This function provides a way to handle such duplicates.
///
/// Example:
/// ~~~ {.cpp}
/// g->RemoveDuplicates();
/// g->Draw("TRI1");
/// ~~~

Int_t TGraph2D::RemoveDuplicates()
{
   for (int i = 0; i < fNpoints; i++) {
      double x = fX[i];
      double y = fY[i];
      for (int j = i + 1; j < fNpoints; j++) {
         if (x == fX[j] && y == fY[j]) {
            RemovePoint(j);
            j--;
         }
      }
   }

   return fNpoints;
}


////////////////////////////////////////////////////////////////////////////////
/// Recursively remove object from the list of functions

void TGraph2D::RecursiveRemove(TObject *obj)
{
   if (fFunctions) {
      if (!fFunctions->TestBit(kInvalidObject))
         fFunctions->RecursiveRemove(obj);
   }
   if (fHistogram == obj)
      fHistogram = nullptr;
}


////////////////////////////////////////////////////////////////////////////////
/// Deletes point number ipoint

Int_t TGraph2D::RemovePoint(Int_t ipoint)
{
   if (ipoint < 0) return -1;
   if (ipoint >= fNpoints) return -1;
   for (Int_t i = ipoint; i < fNpoints - 1; i++) {
      fX[i] = fX[i+1];
      fY[i] = fY[i+1];
      fZ[i] = fZ[i+1];
   }
   fNpoints--;
   if (fHistogram) {
      delete fHistogram;
      fHistogram = nullptr;
      fDelaunay = nullptr;
   }
   return ipoint;
}


////////////////////////////////////////////////////////////////////////////////
/// Saves primitive as a C++ statement(s) on output stream out

void TGraph2D::SavePrimitive(std::ostream &out, Option_t *option)
{
   TString arrx = SavePrimitiveVector(out, "graph2d_x", fNpoints, fX, kTRUE);
   TString arry = SavePrimitiveVector(out, "graph2d_y", fNpoints, fY);
   TString arrz = SavePrimitiveVector(out, "graph2d_z", fNpoints, fZ);

   SavePrimitiveConstructor(out, Class(), "graph2d",
                            TString::Format("%d, %s.data(), %s.data(), %s.data()", fNpoints, arrx.Data(), arry.Data(), arrz.Data()), kFALSE);

   if (strcmp(GetName(), "Graph2D"))
      out << "   graph2d->SetName(\"" << TString(GetName()).ReplaceSpecialCppChars() << "\");\n";

   TString title = GetTitle();
   if (fHistogram)
      title = TString(fHistogram->GetTitle()) + ";" + fHistogram->GetXaxis()->GetTitle() + ";" +
              fHistogram->GetYaxis()->GetTitle() + ";" + fHistogram->GetZaxis()->GetTitle();

   out << "   graph2d->SetTitle(\"" << title.ReplaceSpecialCppChars() << "\");\n";

   if (!fDirectory)
      out << "   graph2d->SetDirectory(nullptr);\n";

   SaveFillAttributes(out, "graph2d", 0, 1001);
   SaveLineAttributes(out, "graph2d", 1, 1, 1);
   SaveMarkerAttributes(out, "graph2d", 1, 1, 1);

   TH1::SavePrimitiveFunctions(out, "graph2d", fFunctions);

   SavePrimitiveDraw(out, "graph2d", option);
}

////////////////////////////////////////////////////////////////////////////////
/// Multiply the values of a TGraph2D by a constant c1.
///
/// If option contains "x" the x values are scaled
/// If option contains "y" the y values are scaled
/// If option contains "z" the z values are scaled
/// If option contains "xyz" all three x, y and z values are scaled

void TGraph2D::Scale(Double_t c1, Option_t *option)
{
   TString opt = option; opt.ToLower();
   if (opt.Contains("x")) {
      for (Int_t i=0; i<GetN(); i++)
         GetX()[i] *= c1;
   }
   if (opt.Contains("y")) {
      for (Int_t i=0; i<GetN(); i++)
         GetY()[i] *= c1;
   }
   if (opt.Contains("z")) {
      for (Int_t i=0; i<GetN(); i++)
         GetZ()[i] *= c1;
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Set number of points in the 2D graph.
/// Existing coordinates are preserved.
/// New coordinates above fNpoints are preset to 0.

void TGraph2D::Set(Int_t n)
{
   if (n < 0) n = 0;
   if (n == fNpoints) return;
   if (n >  fNpoints) SetPoint(n, 0, 0, 0);
   fNpoints = n;
}


////////////////////////////////////////////////////////////////////////////////
/// By default when an 2D graph is created, it is added to the list
/// of 2D graph objects in the current directory in memory.
/// This method removes reference to this 2D graph from current directory and add
/// reference to new directory dir. dir can be 0 in which case the
/// 2D graph does not belong to any directory.

void TGraph2D::SetDirectory(TDirectory *dir)
{
   if (fDirectory == dir) return;
   if (fDirectory) fDirectory->Remove(this);
   fDirectory = dir;
   if (fDirectory) fDirectory->Append(this);
}


////////////////////////////////////////////////////////////////////////////////
/// Sets the histogram to be filled.
/// If the 2D graph needs to be save in a TFile the following set should be
/// followed to read it back:
/// 1. Create TGraph2D
/// 2. Call g->SetHistogram(h), and do whatever you need to do
/// 3. Save g and h to the TFile, exit
/// 4. Open the TFile, retrieve g and h
/// 5. Call h->SetDirectory(0)
/// 6. Call g->SetHistogram(h) again
/// 7. Carry on as normal
///
/// By default use the new interpolation routine based on Triangles
/// If the option "old" the old interpolation is used

void TGraph2D::SetHistogram(TH2 *h, Option_t *option)
{
   TString opt = option;
   opt.ToLower();
   Bool_t oldInterp = opt.Contains("old");

   fUserHisto = kTRUE;
   fHistogram = (TH2D*)h;
   fNpx       = h->GetNbinsX();
   fNpy       = h->GetNbinsY();
   CreateInterpolator(oldInterp);
}


////////////////////////////////////////////////////////////////////////////////
/// Sets the extra space (in %) around interpolated area for the 2D histogram

void TGraph2D::SetMargin(Double_t m)
{
   if (m < 0 || m > 1) {
      Warning("SetMargin", "The margin must be >= 0 && <= 1, fMargin set to 0.1");
      fMargin = 0.1;
   } else {
      fMargin = m;
   }
   if (fHistogram) {
      delete fHistogram;
      fHistogram = nullptr;
      fDelaunay = nullptr;
   }
}


////////////////////////////////////////////////////////////////////////////////
/// Sets the histogram bin height for points lying outside the TGraphDelaunay
/// convex hull ie: the bins in the margin.

void TGraph2D::SetMarginBinsContent(Double_t z)
{
   fZout = z;
   if (fHistogram) {
      delete fHistogram;
      fHistogram = nullptr;
      fDelaunay = nullptr;
   }
}


////////////////////////////////////////////////////////////////////////////////
/// Set maximum.

void TGraph2D::SetMaximum(Double_t maximum)
{
   fMaximum = maximum;
   TH1 * h = GetHistogram();
   if (h) h->SetMaximum(maximum);
}


////////////////////////////////////////////////////////////////////////////////
/// Set minimum.

void TGraph2D::SetMinimum(Double_t minimum)
{
   fMinimum = minimum;
   TH1 * h = GetHistogram();
   if (h) h->SetMinimum(minimum);
}


////////////////////////////////////////////////////////////////////////////////
/// Changes the name of this 2D graph

void TGraph2D::SetName(const char *name)
{
   //  2D graphs are named objects in a THashList.
   //  We must update the hashlist if we change the name
   if (fDirectory) fDirectory->Remove(this);
   fName = name;
   if (fDirectory) fDirectory->Append(this);
}


////////////////////////////////////////////////////////////////////////////////
/// Change the name and title of this 2D graph
///

void TGraph2D::SetNameTitle(const char *name, const char *title)
{
   //  2D graphs are named objects in a THashList.
   //  We must update the hashlist if we change the name
   if (fDirectory) fDirectory->Remove(this);
   fName  = name;
   SetTitle(title);
   if (fDirectory) fDirectory->Append(this);
}


////////////////////////////////////////////////////////////////////////////////
/// Sets the number of bins along X used to draw the function

void TGraph2D::SetNpx(Int_t npx)
{
   if (npx < 4) {
      Warning("SetNpx", "Number of points must be >4 && < 500, fNpx set to 4");
      fNpx = 4;
   } else if (npx > 500) {
      Warning("SetNpx", "Number of points must be >4 && < 500, fNpx set to 500");
      fNpx = 500;
   } else {
      fNpx = npx;
   }
   if (fHistogram) {
      delete fHistogram;
      fHistogram = nullptr;
      fDelaunay = nullptr;
   }
}


////////////////////////////////////////////////////////////////////////////////
/// Sets the number of bins along Y used to draw the function

void TGraph2D::SetNpy(Int_t npy)
{
   if (npy < 4) {
      Warning("SetNpy", "Number of points must be >4 && < 500, fNpy set to 4");
      fNpy = 4;
   } else if (npy > 500) {
      Warning("SetNpy", "Number of points must be >4 && < 500, fNpy set to 500");
      fNpy = 500;
   } else {
      fNpy = npy;
   }
   if (fHistogram) {
      delete fHistogram;
      fHistogram = nullptr;
      fDelaunay = nullptr;
   }
}


////////////////////////////////////////////////////////////////////////////////
/// Sets point number n.
/// If n is greater than the current size, the arrays are automatically
/// extended.

void TGraph2D::SetPoint(Int_t n, Double_t x, Double_t y, Double_t z)
{
   if (n < 0) return;

   if (!fX || !fY || !fZ || n >= fSize) {
      // re-allocate the object
      Int_t newN = TMath::Max(2 * fSize, n + 1);
      Double_t *savex = new Double_t [newN];
      Double_t *savey = new Double_t [newN];
      Double_t *savez = new Double_t [newN];
      if (fX && fSize) {
         memcpy(savex, fX, fSize * sizeof(Double_t));
         memset(&savex[fSize], 0, (newN - fSize)*sizeof(Double_t));
         delete [] fX;
      }
      if (fY && fSize) {
         memcpy(savey, fY, fSize * sizeof(Double_t));
         memset(&savey[fSize], 0, (newN - fSize)*sizeof(Double_t));
         delete [] fY;
      }
      if (fZ && fSize) {
         memcpy(savez, fZ, fSize * sizeof(Double_t));
         memset(&savez[fSize], 0, (newN - fSize)*sizeof(Double_t));
         delete [] fZ;
      }
      fX    = savex;
      fY    = savey;
      fZ    = savez;
      fSize = newN;
   }
   fX[n]    = x;
   fY[n]    = y;
   fZ[n]    = z;
   fNpoints = TMath::Max(fNpoints, n + 1);
}


////////////////////////////////////////////////////////////////////////////////
/// Sets the 2D graph title.
///
/// This method allows to change the global title and the axis' titles of a 2D
/// graph. If `g` is the 2D graph one can do:
///
/// ~~~ {.cpp}
/// g->SetTitle("Graph title; X axis title; Y axis title; Z axis title");
/// ~~~

void TGraph2D::SetTitle(const char* title)
{
   fTitle = title;
   if (fHistogram) fHistogram->SetTitle(title);
}


////////////////////////////////////////////////////////////////////////////////
/// Stream a class object

void TGraph2D::Streamer(TBuffer &b)
{
   if (b.IsReading()) {
      UInt_t R__s, R__c;
      Version_t R__v = b.ReadVersion(&R__s, &R__c);
      b.ReadClassBuffer(TGraph2D::Class(), this, R__v, R__s, R__c);

      ResetBit(kMustCleanup);
   } else {
      b.WriteClassBuffer(TGraph2D::Class(), this);
   }
}

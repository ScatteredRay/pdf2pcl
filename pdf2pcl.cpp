// Copyright (C) 2007 David Barton (davebarton@cityinthesky.co.uk)
// <http://www.cityinthesky.co.uk/>

// Copyright (C) 2007 Matthew Flaschen (matthew.flaschen@gatech.edu)
// Updated to allow conversion of all pages at once.

// Copyright (C) 2009 Immigrant Software
// Based pdf2pcl on pdf2svg code, Large sections rewritten.

//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

// Definitely works with Cairo v1.2.6 and Poppler 0.5.4

#include <optional>
#include <glib.h>
#include <PDFDoc.h>
#include <glib/poppler.h>
#include <stdio.h>
#include <string.h>
#include <OutputDev.h>
#include <fstream>
#include <GfxState.h>
#include <cmath>
#include <GfxFont.h>
#include <cstring>
#include <GlobalParams.h>
#include <UnicodeMap.h>
#include <UnicodeMapTables.h>
#include <algorithm>

const double PCLEpislon = 0.1;
static bool UsePerChar = false;

struct _PopplerDocument
{
  GObject parent_instance;
  PDFDoc *doc;
  void* output_dev;
  /*
#if defined (HAVE_CAIRO)
  CairoOutputDev *output_dev;
#elif defined (HAVE_SPLASH)
  SplashOutputDev *output_dev;
#endif
  */
};

struct _PopplerPage
{
  GObject parent_instance;
  PopplerDocument *document;
  Page *page;
  int index;
  /*TextOutputDev*/void *text_dev;
  Gfx *gfx;
  Annots *annots;
};

double intocm(double in)
{
  return in*2.54;
}

const char* FontToImproFont(const GfxFont* Font)
{

  if(!Font->getFamily())
  {
	if(!Font->getName())
	{
	  fprintf(stderr, "Error: Font has no name!\n");
	  return "STMS";
	}
	const char* N = Font->getName()->c_str();
	fprintf(stderr, "  Warning: Font has no family!\n");
	fprintf(stderr, "\tFont String:%s\n", N);
	fprintf(stderr, "\tTrying best guess...\n");
	if(strstr(N, "Arial"))
	{
		fprintf(stderr, "\tFound Arial font.\n");
		return "SARIAL";
	}\
	else if(strstr(N, "TimesNewRoman") ||
            strstr(N, "Times-Roman"))
	{
		fprintf(stderr, "\tFound Times font.\n");
		return "STMS";
	}
    else if(strstr(N, "Helvetica"))
    {
        fprintf(stderr, "\tFound Helvetica, using Arial font.\n");
        return "SARIAL";
    }
	else
	{
		fprintf(stderr, "Error: Guess Failed on font; Name: %s. Using Times font\n",
				Font->getName()->c_str());
		return "STMS";
	}
  }

  const char* F = Font->getFamily()->c_str();
  if(strcmp(F, "Times New Roman") == 0)
    return "STMS";
  else if(strcmp(F, "Arial") == 0)
    return "SARIAL";
  else if(strcmp(F, "Courier New") == 0)
    return "STMS";
  else if(strcmp(F, "Myriad Pro") == 0)
    return "SARIAL";
  else 
  {
    fprintf(stderr, "Error: No conversion for font '%s'.\n", F);
    return "STMS";
  }
  
}

double GetFontSingleSpaceWidth(const char* font, const char* modifiers, int FontSize)
{
  const double PPInch = 72.0;
  const double HorizSpaceWidth = 0.2731;
  return FontSize*HorizSpaceWidth/PPInch;
}

const char* GetFontModifiers(const GfxFont* Font)
{
  bool b = Font->isBold() || strstr(Font->getName()->c_str(), "Bold");
  bool i = Font->isItalic() || strstr(Font->getName()->c_str(), "Italic");

  if(b && i)
    return "BI";
  else if(b)
    return "B";
  else if(i)
    return "I";
  else
    return "";
}

bool GetRectCoords(GfxState* state, double* ox1, double* oy1, double* ox2, double* oy2)
{

  using namespace std;
  
  const GfxPath* path = state->getPath();
  int pointcount = 0;
  double minx, maxx, miny, maxy;
  double iX, iY;
  bool bInit = true;
  
  for(int p = 0; p < path->getNumSubpaths(); p++)
  {
    const GfxSubpath* SP = path->getSubpath(p);
    for(int n=0; n < SP->getNumPoints()-1; n++)
    {
      double x1, y1, x2, y2;
      x1 = SP->getX(n);
      y1 = SP->getY(n);
      x2 = SP->getX(n+1);
      y2 = SP->getY(n+1);

      state->transform(x1, y1, &x1, &y1);
      state->transform(x2, y2, &x2, &y2);

      pointcount++;

      if(bInit)
      {
	iX = x1;
	iY = y1;
	minx = min(x1, x2);
	miny = min(y1, y2);
	maxx = max(x1, x2);
	maxy = max(y1, y2);
	bInit = false;
      }

      minx = min(minx, x1);
      minx = min(minx, x2);
      miny = min(miny, y1);
      miny = min(miny, y2);

      maxx = max(maxx, x1);
      maxx = max(maxx, x2);
      maxy = max(maxy, y1);
      maxy = max(maxy, y2);

      if(p == path->getNumSubpaths()-1 && n == SP->getNumPoints()-2)
      {
	// x2/y2 is last point
	if(abs(x2 - iX) > PCLEpislon || abs(y2 - iY) > PCLEpislon)
	{
	  return false;
	}
      }
      
    }
  }
  
  if(pointcount < 4)
    return false;

  *ox1 = minx;
  *ox2 = maxx;
  *oy1 = miny;
  *oy2 = maxy;

  return true;
}

class PCLOutputDev : public OutputDev
{
public:
  FILE* pcl;
  const GfxFont* SelectedFont;
  int SelectedFontSize;
  double LineWidth;
  const char* ImproFont;
  const char* FontModifiers; 
  UnicodeMap* UMap;
  char CurrentStr[255];
  char* CurrentChar;
  double CurrHoriLength;
  double CurrX, CurrY;
  double curr_space_width;
  PCLOutputDev(const char* PCLFile)
  {
    // For some reason, "w" was getting file types confused between unix and dos style line endings...
    // Should investigate, perhaps BOM related? "wb" works fine since we are always writing '\r\n'
    pcl = fopen(PCLFile, "wb");
    SelectedFont = NULL;
    SelectedFontSize = 0;
    LineWidth = 0.0;
    FontModifiers = "";
    ImproFont = "";
    UMap = new UnicodeMap("ascii7", true, ascii7UnicodeMapRanges, ascii7UnicodeMapLen);
    CurrentChar = 0;
  }
  ~PCLOutputDev()
  {
    // Class pointers aren't managed here, don't need to get cleaned up
    // SelectedFont is never used, just a pointer test.
    // FontModifiers only points to constants
    delete UMap;
    fclose(pcl);
  }
  bool upsideDown()
  {
    return true;
  }
  bool useDrawChar()
  {
    return UsePerChar;
  }
  bool interpretType3Chars()
  {
    return false;
  }
  virtual void updateFont(GfxState* state) override
  {
    const GfxFont* Font = state->getFont().get(); // Is the scope long enough?
    const double* Mat = state->getTextMat();
    double Size = state->getFontSize() * Mat[0];
    if(!Font)
      return;
    const char* FM = GetFontModifiers(Font);

    if(SelectedFont == Font && SelectedFontSize == (int)Size && FontModifiers == FM)
      return;
    
    SelectedFont = Font;
    SelectedFontSize = (int)Size;
    FontModifiers = FM;
    ImproFont = FontToImproFont(Font);

    fprintf(pcl, "font\t%s%d%s\r\n", ImproFont, SelectedFontSize, FM);
  }
  virtual void updateTextMat(GfxState* state) override
  {
    updateFont(state);
  }
  virtual void drawString(GfxState* state, const GooString* S) override
  {
    double x1, y1;
    const char* S1 = S->c_str();
	if(S1 == NULL)
		fprintf(stderr, "\tWarning: Null String\n");
	else if(*S1 == '\0')
		fprintf(stderr, "\tWarning: Empty String\n");
    double space_width = GetFontSingleSpaceWidth(ImproFont, FontModifiers, SelectedFontSize);
    x1 = state->getCurX();
    y1 = state->getCurY() + state->getRise();
    state->transform(x1, y1, &x1, &y1);
    if(S1[0] < 0 && false)
    {
      switch((unsigned char)S1[0])
      {
      case 147: case 148:
        '"';
      case 167:
        '§';
      }
      fprintf(stderr, "  Warning: Found Extended Ascii '%u' in : %s\n", ((unsigned char)S1[0]), S1);
      S1++;
    }
    while(*S1 == ' ')
    {
      S1++;
      x1 += space_width;
    }
    x1 = intocm(x1);
    y1 = intocm(y1);
	if(*S1)
		fprintf(pcl, "text\t%f\t%f\t%s\r\n", (float) x1, (float) y1, S1);
  }
  virtual void updateLineWidth(GfxState* state) override
  {
    // Impro line width 
    // cm_width = (pcl_width/300.0)*2.54
    double Width = (state->getTransformedLineWidth());
    
    if(Width == LineWidth)
      return;

    LineWidth = Width;
    fprintf(pcl, "lwid\t%d\r\n", (int)(Width*300.0));
  }
  virtual void beginString(GfxState* state, const GooString* S) override
  {
	if(CurrentChar)
	  endString(state);
	memset(CurrentStr, '\0', sizeof(CurrentStr));
	CurrentChar = CurrentStr;
	CurrX = 0.0;//state->getCurX();
	CurrY = 0.0;//state->getCurY() + state->getRise();
	//state->transform(CurrX, CurrY, &CurrX, &CurrY);
	curr_space_width = GetFontSingleSpaceWidth(ImproFont, FontModifiers, SelectedFontSize);
	CurrHoriLength = 0;
  }
  virtual void drawChar(GfxState * state, double x, double y,
			double dx, double dy,
			double /*originX*/, double /*originY*/,
			CharCode code, int /*nBytes*/, const Unicode * u, int uLen) override
  {
	if(!CurrentChar)
	  beginString(state, NULL);

	if(CurrX == 0.0)
	{
		CurrX = x;
		CurrY = y;
		state->transform(CurrX, CurrY, &CurrX, &CurrY);
	}

	char buffer[8];
	memset(buffer, '\0', 8);
	UMap->mapUnicode(*u, buffer, 255);

	if((CurrentChar - CurrentStr) >= sizeof(CurrentStr))
	{
		endString(state);
		beginString(state, NULL);

		assert((CurrentChar - CurrentStr) >= sizeof(CurrentStr));
	}

	state->transformDelta(dx, dy, &dx, &dy);
	
	if((*buffer == ' ' || *buffer == '\0') && CurrentChar == CurrentStr)
	  CurrX += dx;
	else if(*buffer == '\0')
	{
	  *(CurrentChar++) = ' ';
	  CurrHoriLength += dx;
	}
	else
	{
	  *(CurrentChar++) = *buffer;
	  CurrHoriLength += dx;
	}

  }
  virtual void endString(GfxState* state) override
  {
	if(*CurrentStr)
		fprintf(pcl, "textl\t%f\t%f\t%f\t%s\r\n", (float)intocm(CurrX), (float)intocm(CurrY), (float)intocm(CurrHoriLength), CurrentStr);
	CurrentChar = 0;
  }
  virtual void stroke(GfxState* state) override
  {
    double x1, y1, x2, y2;
    if(GetRectCoords(state, &x1, &y1, &x2, &y2))
    {
      fprintf(pcl, "box\t%f\t%f\t%f\t%f\r\n", intocm(x1), intocm(y1), intocm(x2), intocm(y2));
      return;
    }

    const GfxPath* path = state->getPath();
    for(int p = 0; p < path->getNumSubpaths(); p++)
    {
      const GfxSubpath* SP = path->getSubpath(p);
      for(int n=0; n < SP->getNumPoints()-1; n++)
      {
	x1 = SP->getX(n);
	y1 = SP->getY(n);
	x2 = SP->getX(n+1);
	y2 = SP->getY(n+1);

	state->transform(x1, y1, &x1, &y1);
	state->transform(x2, y2, &x2, &y2);
	double hLw = LineWidth/2;
	// determine line direction
	if(fabs(x2-x1) > fabs(y2-y1))
	{
	  // horizontal
	  bool bReversed = x2 < x1;
	  fprintf(pcl, "hlin\t%f\t%f\t%f\r\n",
		  (float)intocm((bReversed ? x2 : x1)),
		  (float)intocm(y1-hLw),
		  (float)intocm(fabs(x2-x1)));
	}
	else
	{
	  // vertical
	  bool bReversed = y2 < y1;
	  fprintf(pcl, "vlin\t%f\t%f\t%f\r\n",
		  (float)intocm(x1-hLw),
		  (float)intocm((bReversed ? y2 : y1)),
		  (float)intocm(fabs(y2-y1)));
	}
      }
    }
  }

  virtual void fill(GfxState* state) override
  {
    double x1, y1, x2, y2;
    if(GetRectCoords(state, &x1, &y1, &x2, &y2))
    {
      GfxGray gray;
      state->getFillGray(&gray);
      // Used to have this weird math previously not sure where it came from...
      //int ImmGray = (int)(100.0-((double)gray)/655.36);
      // 1 is white, 100 is black.. but oddly 0 renders as black.
      // It's not super clear to me that pdf 1.0 is white, but we're going off that assumption.
      int ImmGray = (int)(100.0*(1.0-colToDbl(gray)));
      if(ImmGray > 0)
        fprintf(pcl, "shade\t%f\t%f\t%f\t%f\t%d\r\n", intocm(x1), intocm(y1), intocm(x2), intocm(y2), ImmGray);
    }
    else
      fprintf(stderr, "Error: Degenerate Fill path, not closed!\n");
  }

};

// Begin theft from ePDFview (Copyright (C) 2006 Emma's Software) under the GPL
gchar *getAbsoluteFileName(const gchar *fileName)
{
    gchar *absoluteFileName = NULL;
    if (g_path_is_absolute(fileName)) {
        absoluteFileName = g_strdup(fileName);
    }
    else {
        gchar *currentDir = g_get_current_dir();
        absoluteFileName = g_build_filename(currentDir, fileName, NULL);
        g_free(currentDir);
    }
    return absoluteFileName;
}
// End theft from ePDFview


int convertPage(PopplerPage *page, const char* svgFilename)
{
    // Poppler stuff 
    double width, height;

    if (page == NULL) {
        fprintf(stderr, "Page does not exist\n");
        return -1;
    }

    //globalParams->setPrintCommands(gTrue);

    PCLOutputDev* output_dev = new PCLOutputDev(svgFilename);
    page->page->displaySlice(output_dev,
			     1.0, 1.0, 0,
			     false, true,
			     -1, -1, -1, -1,
			     false,
			     NULL, NULL,
			     NULL, NULL);

    // Close the PDF file
    g_object_unref(page);
    
    return 0;     
}

int main(int argn, char *args[])
{
    // Poppler stuff
    PopplerDocument *pdffile;
    PopplerPage *page;

    // Initialise the GType library
    g_type_init ();

    // Get command line arguments
    if ((argn < 3)||(argn > 5)) {
        printf("Usage: pdf2pcl <in file.pdf> <out file.svg> [<page no>] [-U]\n");
        return -2;
    }
    gchar *absoluteFileName = getAbsoluteFileName(args[1]);
    gchar *filename_uri = g_filename_to_uri(absoluteFileName, NULL, NULL);
    gchar *pageLabel = NULL;

    char* svgFilename = args[2];

    g_free(absoluteFileName);
    if (argn >= 4) {
		if(strcmp(args[3], "-U") == 0)
			UsePerChar = true;
		else // Get page number
			pageLabel = g_strdup(args[3]); 
    }

	if(argn >= 5 && strcmp(args[4], "-U") == 0)
			UsePerChar = true;

    // Open the PDF file
    pdffile = poppler_document_new_from_file(filename_uri, NULL, NULL);
    g_free(filename_uri);
    if (pdffile == NULL) {
        fprintf(stderr, "Unable to open file\n");
        return -3;
    }

    int conversionErrors = 0;
    // Get the page

    if(pageLabel == NULL)
    {
	 page = poppler_document_get_page(pdffile, 0);
	 conversionErrors = convertPage(page, svgFilename);
    }
    else
    {
	 if(strcmp(pageLabel, "all") == 0)
	 {
	      int curError;
	      int pageCount = poppler_document_get_n_pages(pdffile);
	      
	      if(pageCount > 9999999) 
	      {
		   fprintf(stderr, "Too many pages (>9,999,999)\n");
		   return -5;
	      }
	 
	      char pageCountBuffer[8]; // 9,999,999 page limit
	      sprintf(pageCountBuffer, "%d", pageCount);

	      char* svgFilenameBuffer = (char*)malloc(strlen(svgFilename) + strlen(pageCountBuffer));

	      int pageInd;
	      for(pageInd = 0; pageInd < pageCount; pageInd++)
	      {
		   sprintf(svgFilenameBuffer, svgFilename, pageInd + 1);
		   page = poppler_document_get_page(pdffile, pageInd);
		   curError = convertPage(page, svgFilenameBuffer);
		   if(curError != 0) {
			conversionErrors = -1;
		   }
	      }

	      free(svgFilenameBuffer);
	 }
	 else
	 {
	      page = poppler_document_get_page_by_label(pdffile, pageLabel);
	      conversionErrors = convertPage(page, svgFilename);
	      g_free(pageLabel);
	 }
    }

    g_object_unref(pdffile);

    if(conversionErrors != 0) {
	 return -4;
    }

    else {
	 return 0;
    }

}

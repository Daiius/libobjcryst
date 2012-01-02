/*  ObjCryst++ Object-Oriented Crystallographic Library
    (c) 2000-2011 Vincent Favre-Nicolin vincefn@users.sourceforge.net
        2000-2001 University of Geneva (Switzerland)
        2008-2010 Jan Rohlicek - Inst. of Chemical Technology, Prague

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef __WX__CRYST__
   // For compilers that support precompilation, includes "wx/wx.h".
   #ifndef __DARWIN__ // work around MacOSX type_info bug (??)
      #include "wx/wxprec.h"
   #endif

   #ifdef __BORLANDC__
       #pragma hdrstop
   #endif

   // for all others, include the necessary headers (this file is usually all you
   // need because it includes almost all "standard" wxWindows headers)
   #ifndef WX_PRECOMP
       #include "wx/wx.h"
   #endif

   #include "wx/tooltip.h"
   #include "wx/notebook.h"
   #include "wx/wfstream.h"
   #include "wx/zstream.h"
   #include "wx/fileconf.h"
   #include "wx/filesys.h"
   #include <wx/fs_inet.h>
   #include <wx/txtstrm.h>
   #include <wx/minifram.h>
   #include <wx/dirdlg.h>
#endif

#include <cstdlib>
#include <locale.h>
#include <sstream>
#include <list>
#include <cstring>

#include "ObjCryst/ObjCryst/General.h"
#include "ObjCryst/Quirks/Chronometer.h"
#include "ObjCryst/ObjCryst/IO.h"
#include "ObjCryst/ObjCryst/Crystal.h"
#include "ObjCryst/ObjCryst/PowderPattern.h"
#include "ObjCryst/ObjCryst/DiffractionDataSingleCrystal.h"
#include "ObjCryst/ObjCryst/Polyhedron.h"
#include "ObjCryst/ObjCryst/test.h"
#include "ObjCryst/ObjCryst/CIF.h"
//#include "ObjCryst/ObjCryst/PDF.h"
#include "ObjCryst/RefinableObj/GlobalOptimObj.h"
#include "ObjCryst/Quirks/VFNStreamFormat.h"

#ifdef __WX__CRYST__
   #include "ObjCryst/wxCryst/wxCrystal.h"
//FOXGrid
   #include "WXGridWindow.h"
   #if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMAC__) || defined(__WXMGL__) || defined(__WXX11__)
      #include "Fox.xpm"
   #endif
  #if !wxUSE_UNICODE
  #define _T(x) x
  #endif
#else
#define _T(x) x
#endif

using namespace ObjCryst;
using namespace std;

// Rough version number - must be updated at least for every major version or critical update
// This is used to check for updates...
//:TODO: supply __FOXREVISION__ from the command line (at least under Linux)
#define __FOXREVISION__ 1350

static std::string foxVersion;

// ----------------------------------------------------------------------------
// Speed test
// ----------------------------------------------------------------------------
void standardSpeedTest();

#ifdef __WX__CRYST__
// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class MyApp : public wxApp
{
public:
    virtual bool OnInit();
    virtual int OnExit();
};

// WXCrystScr
class WXCrystScrolledWindow:public wxScrolledWindow
{
   public:
      WXCrystScrolledWindow(wxWindow* parent);
      virtual bool Layout();
      void SetChild(wxWindow* pChild);
#if (wxVERSION_NUMBER>=2808) && (wxVERSION_NUMBER<=2811)
      void OnWXCrystChildFocus(wxChildFocusEvent& event);
#endif
   private:
      wxWindow* mpChild;
      int mHeight,mWidth;
      wxBoxSizer *mpSizer;
#if (wxVERSION_NUMBER>=2808) && (wxVERSION_NUMBER<=2811)
    DECLARE_EVENT_TABLE()
#endif
};

#if (wxVERSION_NUMBER>=2808) && (wxVERSION_NUMBER<=2811)
BEGIN_EVENT_TABLE(WXCrystScrolledWindow, wxScrolledWindow)
   EVT_CHILD_FOCUS(WXCrystScrolledWindow::OnWXCrystChildFocus)
END_EVENT_TABLE()
#endif

// main frame
class WXCrystMainFrame : public wxFrame
{
public:
   WXCrystMainFrame(const wxString& title, const wxPoint& pos, const wxSize& size,
                    const bool splashscreen=true);
   void OnQuit(wxCommandEvent& WXUNUSED(event));
   void OnAbout(wxCommandEvent& WXUNUSED(event));
   void OnLoad(wxCommandEvent& event);
   void Load(const wxString &filename);
   void OnBrowse(wxCommandEvent& event);
   void OnBrowseSelect(wxCommandEvent& event);
   void OnMenuClose(wxCommandEvent& event);
   void Close(const bool safe=true);
   void OnClose(wxCloseEvent& event);
   void SafeQuit();
   void OnSave(wxCommandEvent& WXUNUSED(event));
   void OnAddCrystal(wxCommandEvent& WXUNUSED(event));
   void OnAddPowderPattern(wxCommandEvent& WXUNUSED(event));
   void OnAddSingleCrystalData(wxCommandEvent& WXUNUSED(event));
   void OnAddGlobalOptimObj(wxCommandEvent& WXUNUSED(event));
   void OnAddGeneticAlgorithm(wxCommandEvent& WXUNUSED(event));
   void OnDebugTest(wxCommandEvent& event);
   void OnSetDebugLevel(wxCommandEvent& event);
   void OnUpdateUI(wxUpdateUIEvent& event);
   void OnToggleTooltips(wxCommandEvent& event);
   void OnPreferences(wxCommandEvent& event);
   void OnCheckUpdate(wxCommandEvent& event);
   //FoxGrid////////////////////////////////////////
   void OnStartGridServer(wxCommandEvent &event);
   void OnStartGridClient(wxCommandEvent &event);

    //FoxGrid////////////////////////////////////////
   WXGrigWindow *mpGridWindow;
private:
    DECLARE_EVENT_TABLE()
    RefinableObjClock mClockLastSave;
    wxNotebook *mpNotebook;
    /// List of available updates
   std::map<unsigned int,pair<int,wxString> > mvUpdates;
   /// Are we during an autocheck ?
   bool mvUpdatesAutoCheck;
   /// Browsing dir
   wxString mBrowseDir;
   /// List of files in browsing dir
   wxListBox *mpBrowseList;
};

// ----------------------------------------------------------------------------
// For messaging the user
// ----------------------------------------------------------------------------
wxFrame *pMainFrameForUserMessage;

void WXCrystInformUserStdOut(const string &str)
{
   if(wxThread::IsMain()) pMainFrameForUserMessage->SetStatusText(wxString::FromAscii(str.c_str()));
   else cout<<"Message for user (outside main thread):"<<str<<endl;
}

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------
static const long MENU_FILE_QUIT=                      WXCRYST_ID();
static const long MENU_HELP_ABOUT=                     WXCRYST_ID();
static const long MENU_HELP_TOGGLETOOLTIP=             WXCRYST_ID();
static const long MENU_HELP_UPDATE=                    WXCRYST_ID();
static const long MENU_PREFS_PREFERENCES=              WXCRYST_ID();
static const long MENU_FILE_LOAD=                      WXCRYST_ID();
static const long MENU_FILE_BROWSE=                    WXCRYST_ID(); 
static const long MENU_FILE_CLOSE=                     WXCRYST_ID();
static const long MENU_FILE_SAVE=                      WXCRYST_ID();
static const long MENU_OBJECT_CREATE_CRYSTAL=          WXCRYST_ID();
static const long MENU_OBJECT_CREATE_POWDERSPECTRUM=   WXCRYST_ID();
static const long MENU_OBJECT_CREATE_SINGLECRYSTALDATA=WXCRYST_ID();
static const long MENU_OBJECT_CREATE_GLOBALOPTOBJ=     WXCRYST_ID();
static const long MENU_OBJECT_CREATE_GENETICALGORITHM= WXCRYST_ID();
static const long MENU_DEBUG_LEVEL0=                   WXCRYST_ID();
static const long MENU_DEBUG_LEVEL1=                   WXCRYST_ID();
static const long MENU_DEBUG_LEVEL2=                   WXCRYST_ID();
static const long MENU_DEBUG_LEVEL3=                   WXCRYST_ID();
static const long MENU_DEBUG_LEVEL4=                   WXCRYST_ID();
static const long MENU_DEBUG_LEVEL5=                   WXCRYST_ID();
static const long MENU_DEBUG_LEVEL6=                   WXCRYST_ID();
static const long MENU_DEBUG_LEVEL7=                   WXCRYST_ID();
static const long MENU_DEBUG_LEVEL8=                   WXCRYST_ID();
static const long MENU_DEBUG_LEVEL9=                   WXCRYST_ID();
static const long MENU_DEBUG_LEVEL10=                  WXCRYST_ID();
static const long MENU_DEBUG_TEST1=                    WXCRYST_ID();
static const long MENU_DEBUG_TEST2=                    WXCRYST_ID();
static const long MENU_DEBUG_TEST3=                    WXCRYST_ID();
static const long ID_ABOUT_FOX_BUTTON_UPDATE=          WXCRYST_ID();
static const long ID_FOX_BROWSE=                       WXCRYST_ID(); 

//FoxGrid///////////////////////////////////////////////////////////
static const long MENU_GRID_SERVER_RUN=                WXCRYST_ID();
static const long MENU_GRID_CLIENT_START=              WXCRYST_ID();


/// Separate thread to check for updates////////////////////////////////////////////////////////////
static const long ID_FOX_UPDATES_RESULT=               WXCRYST_ID();

class WXThreadCheckUpdates:public wxThread
{
   public:
      WXThreadCheckUpdates(std::map<unsigned int,pair<int,wxString> > &vUpdates,WXCrystMainFrame &caller):
      wxThread(wxTHREAD_DETACHED),mpvUpdates(&vUpdates),mpCaller(&caller)
      {}
      wxThread::ExitCode Entry()
      {
         //cout<<"WXThreadCheckUpdates:: OnEntry()"<<endl;
         mpvUpdates->clear();
         if(!(wxFileSystem::HasHandlerForPath(wxString::FromAscii("http://objcryst.sourceforge.net/FoxUpdates.txt"))))
            wxFileSystem::AddHandler(new wxInternetFSHandler);
         wxFileSystem fs;
         wxFSFile *fp= NULL;
         fp= fs.OpenFile(wxString::FromAscii("http://objcryst.sourceforge.net/FoxUpdates.txt"),wxFS_READ);
         if(fp!=NULL)
         {
            wxInputStream *fstream = fp->GetStream();
            wxTextInputStream txtis(*fstream);
            txtis.ReadLine();//first line
            while(!fstream->Eof())
            {
               unsigned int revisionfix=txtis.Read16();
               unsigned int revisionbug=txtis.Read16();
               unsigned int severity=txtis.Read16();
               wxString reason=txtis.ReadLine();
               if((revisionfix>__FOXREVISION__)&&(__FOXREVISION__>revisionbug))
               {
                  //cout<<"Revision:"<<revisionfix<<", severity="<<severity<<",reason="<<reason<<endl;
                  (*mpvUpdates)[revisionfix]=make_pair(severity,reason);
               }
            }
         }
         return NULL;
      }
      void OnExit()
      {
         //cout<<"WXThreadCheckUpdates:: OnExit()"<<endl;
         wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,ID_FOX_UPDATES_RESULT);
         wxPostEvent(mpCaller,event);
      }
   private:
      std::map<unsigned int,pair<int,wxString> > *mpvUpdates;
      WXCrystMainFrame *mpCaller;
};

// ----------------------------------------------------------------------------
// event tables and other macros for wxWindows
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(WXCrystMainFrame, wxFrame)
   EVT_MENU(MENU_FILE_QUIT,                        WXCrystMainFrame::OnQuit)
   EVT_MENU(MENU_HELP_ABOUT,                       WXCrystMainFrame::OnAbout)
   EVT_MENU(MENU_HELP_TOGGLETOOLTIP,               WXCrystMainFrame::OnToggleTooltips)
   EVT_MENU(MENU_HELP_UPDATE,                      WXCrystMainFrame::OnCheckUpdate)
   EVT_MENU(ID_FOX_UPDATES_RESULT,                 WXCrystMainFrame::OnCheckUpdate)
   EVT_MENU(MENU_PREFS_PREFERENCES,                WXCrystMainFrame::OnPreferences)
   EVT_MENU(MENU_FILE_LOAD,                        WXCrystMainFrame::OnLoad)
   EVT_MENU(MENU_FILE_BROWSE,                      WXCrystMainFrame::OnBrowse)
   EVT_LISTBOX(ID_FOX_BROWSE,                      WXCrystMainFrame::OnBrowseSelect)
   EVT_LISTBOX_DCLICK(ID_FOX_BROWSE,               WXCrystMainFrame::OnBrowseSelect)
   EVT_MENU(MENU_FILE_CLOSE,                       WXCrystMainFrame::OnMenuClose)
   EVT_CLOSE(                                      WXCrystMainFrame::OnClose)
   EVT_MENU(MENU_FILE_SAVE,                        WXCrystMainFrame::OnSave)
   EVT_MENU(MENU_OBJECT_CREATE_CRYSTAL,            WXCrystMainFrame::OnAddCrystal)
   EVT_MENU(MENU_OBJECT_CREATE_POWDERSPECTRUM,     WXCrystMainFrame::OnAddPowderPattern)
   EVT_MENU(MENU_OBJECT_CREATE_SINGLECRYSTALDATA,  WXCrystMainFrame::OnAddSingleCrystalData)
   EVT_MENU(MENU_OBJECT_CREATE_GLOBALOPTOBJ,       WXCrystMainFrame::OnAddGlobalOptimObj)
   EVT_MENU(MENU_OBJECT_CREATE_GENETICALGORITHM,   WXCrystMainFrame::OnAddGeneticAlgorithm)
   EVT_MENU(MENU_DEBUG_LEVEL0,                     WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL1,                     WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL2,                     WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL3,                     WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL4,                     WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL5,                     WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL6,                     WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL7,                     WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL8,                     WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL9,                     WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL10,                    WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_TEST1,                      WXCrystMainFrame::OnDebugTest)
   EVT_MENU(MENU_DEBUG_TEST2,                      WXCrystMainFrame::OnDebugTest)
   EVT_MENU(MENU_DEBUG_TEST3,                      WXCrystMainFrame::OnDebugTest)
   EVT_UPDATE_UI(ID_CRYST_UPDATEUI,                WXCrystMainFrame::OnUpdateUI)
   //FoxGrid///////////////////////////////////////////////////////////////////////////////
   EVT_MENU(MENU_GRID_SERVER_RUN, WXCrystMainFrame::OnStartGridServer)
   EVT_MENU(MENU_GRID_CLIENT_START, WXCrystMainFrame::OnStartGridClient)
END_EVENT_TABLE()

IMPLEMENT_APP(MyApp)

// ============================================================================
// implementation
// ============================================================================

int STRCMP(wxChar* s1,wxChar* s2) {return wxStrcmp(s1,s2);}
int STRCMP(wxChar* s1,wxString s2) {return wxStrcmp(s1,s2.c_str());}

// 'Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
#else

int STRCMP(const char* s1,const char* s2){ return strcmp(s1,s2);}

int main (int argc, char *argv[])
#endif
{
   TAU_PROFILE("main()","int()",TAU_DEFAULT);
   TAU_PROFILE_SET_NODE(0);
   //set locale settings to standard
   setlocale(LC_NUMERIC,"C");
   
   {// Fox version
      char verBuf[200];
      sprintf(verBuf,"1.9.8-#%d",__FOXREVISION__);
      foxVersion=verBuf;
   }
   bool useGUI(true);
   long nbTrial(1000000);
   long nbRun(1);
   double finalCost=0.;
   bool silent=false;
   string outfilename("Fox-out.xml");
   string working_dir("");
   long filenameInsertCost=-1;
   bool randomize(false);
   bool only3D(false);
   bool loadFourierGRD(false);
   list<string> vFourierFilenameGRD;
   bool loadFourierDSN6(false);
   list<string> vFourierFilenameDSN6;
   bool cif2pattern=false,cif2patternN=false;
   double cif2patternWavelength=1.54056;
   double cif2patternPeakWidth=0.01;
   long cif2patternNbPoint=1000;
   double cif2patternMax2Theta=M_PI*.9;
   bool exportfullprof=false;
   bool fitprofile=false;
    //FoxGrid
   bool runclient(false);
   long nbCPUs = -1;
   string IP;
   bool testLSQ=false;
   bool testMC=false;
   bool testSPEED=false;
   for(int i=1;i<argc;i++)
   {
       #ifdef __WX__CRYST__
      //FoxGrid
      if(STRCMP(_T("--runclient"),argv[i])==0)
      {
         
         if(!useGUI) {
            cout << "Client output: Run client with GUI only!"<<endl;
            cout << "i.e. Fox --runclient 10.0.0.1 --CPUs 4 --working_dir c:\\FOXGrid"<<endl;
            exit(0);
         }
         runclient = true;
         i++;
         IP = string(wxString(argv[i]).ToAscii());
         //get nb of CPUs to use
         if(STRCMP(_T("--CPUs"),argv[i+1])==0) {
             i=i+2;
             wxString(argv[i]).ToLong(&nbCPUs);
         }
         continue;
      }
      #endif
      if(STRCMP(_T("--working_dir"),argv[i])==0)
      {
          i++;
          working_dir = string(wxString(argv[i]).ToAscii());
          cout << "Working directory is: "<<working_dir<<endl;
          continue;
      }
      if(STRCMP(_T("--nogui"),argv[i])==0)
      {
         useGUI=false;
         cout << "Running Fox without GUI"<<endl;
         continue;  
      }
      if(STRCMP(_T("--randomize"),argv[i])==0)
      {
         randomize=true;
         cout << "Randomizing parameters before running"<<endl;
         continue;  
      }
      if(STRCMP(_T("--silent"),argv[i])==0)
      {
         silent=true;
         cout << "Running Fox quietly"<<endl;
         continue;  
      }
      if(STRCMP(_T("--finalcost"),argv[i])==0)
      {
         ++i;
         #ifdef __WX__CRYST__
         wxString(argv[i]).ToDouble(&finalCost);
         #else
         stringstream sstr(argv[i]);
         sstr >> finalCost;
         #endif
         cout << "Fox will stop after reaching cost:"<<finalCost<<endl;
         continue;  
      }
      if(STRCMP(_T("-n"),argv[i])==0)
      {
         ++i;
         #ifdef __WX__CRYST__
         wxString(argv[i]).ToLong(&nbTrial);
         #else
         stringstream sstr(argv[i]);
         sstr >> nbTrial;
         #endif
         cout << "Fox will run for "<<nbTrial<<" trials"<<endl;
         continue;
      }
      if(STRCMP(_T("--nbrun"),argv[i])==0)
      {
         ++i;
         #ifdef __WX__CRYST__
         wxString(argv[i]).ToLong(&nbRun);
         #else
         stringstream sstr(argv[i]);
         sstr >> nbRun;
         #endif
         cout << "Fox will do "<<nbRun<<" runs, randomizing before each run"<<endl;
         continue;
      }
      if((STRCMP(_T("--cif2pattern"),argv[i])==0) || (STRCMP(_T("--cif2patternN"),argv[i])==0))
      {
         if(STRCMP(_T("--cif2patternN"),argv[i])==0) cif2patternN=true;
         ++i;
         cif2pattern=true;
         {
            #ifdef __WX__CRYST__
            wxString(argv[i]).ToDouble(&cif2patternWavelength);
            #else
            stringstream sstr(argv[i]);
            sstr >> cif2patternWavelength;
            #endif
         }
         ++i;
         {
            #ifdef __WX__CRYST__
            wxString(argv[i]).ToDouble(&cif2patternMax2Theta);
            #else
            stringstream sstr(argv[i]);
            sstr >> cif2patternMax2Theta;
            #endif
            cif2patternMax2Theta*=DEG2RAD;
            if(cif2patternMax2Theta>M_PI) cif2patternMax2Theta=M_PI;
         }
         ++i;
         {
            #ifdef __WX__CRYST__
            wxString(argv[i]).ToLong(&cif2patternNbPoint);
            #else
            stringstream sstr(argv[i]);
            sstr >> cif2patternNbPoint;
            #endif
         }
         ++i;
         {
            #ifdef __WX__CRYST__
            wxString(argv[i]).ToDouble(&cif2patternPeakWidth);
            #else
            stringstream sstr(argv[i]);
            sstr >> cif2patternPeakWidth;
            #endif
            cif2patternPeakWidth*=DEG2RAD;
         }
         continue;
      }
      if(STRCMP(_T("-i"),argv[i])==0)
      {// Obsolete, just ignore
         ++i;
         continue;
      }
      if(STRCMP(_T("-o"),argv[i])==0)
      {
         ++i;
         #ifdef __WX__CRYST__
         outfilename=string(wxString(argv[i]).ToAscii());
         #else
         outfilename=argv[i];
         #endif
         filenameInsertCost = outfilename.find("#cost",0);
         cout <<"Fox:#cost, pos="<<filenameInsertCost<<","<<string::npos<<endl;
         if((long)(string::npos)==filenameInsertCost) filenameInsertCost=-1;
         continue;
      }
      if(STRCMP(_T("--loadfouriergrd"),argv[i])==0)
      {
         ++i;
         loadFourierGRD=true;
         #ifdef __WX__CRYST__
         vFourierFilenameGRD.push_back(string(wxString(argv[i]).ToAscii()));
         #else
         vFourierFilenameGRD.push_back(argv[i]);
         #endif
         continue;
      }
      if(STRCMP(_T("--loadfourierdsn6"),argv[i])==0)
      {
         ++i;
         loadFourierDSN6=true;
         #ifdef __WX__CRYST__
         vFourierFilenameDSN6.push_back(string(wxString(argv[i]).ToAscii()));
         #else
         vFourierFilenameDSN6.push_back(argv[i]);
         #endif
         continue;
      }
      if(STRCMP(_T("--only3d"),argv[i])==0)
      {
         only3D=true;
         continue;
      }
      if(STRCMP(_T("--speedtest"),argv[i])==0)
      {
         testSPEED=true;
         continue;
      }
      if(STRCMP(_T("--test-lsq"),argv[i])==0)
      {
         testLSQ=true;
         continue;
      }
      if(STRCMP(_T("--test-mc"),argv[i])==0)
      {
         testMC=true;
         continue;
      }
      if(STRCMP(_T("--exportfullprof"),argv[i])==0)
      {
         exportfullprof=true;
         continue;
      }
      if(STRCMP(_T("--fitprofile"),argv[i])==0)
      {
         fitprofile=true;
         continue;
      }
      if(STRCMP(_T("--index"),argv[i])==0)
      {
         ++i;
         #ifdef __WX__CRYST__
         wxString tmp_argv(argv[i]);
         ifstream f(tmp_argv.ToAscii());
         #else
         ifstream f(argv[i]);
         #endif
         if(!f) 
         {
            cout<<"Cannot find file to index:"<<argv[i]<<endl;
            exit(0);
         }
         PeakList pl;
         pl.ImportDhklDSigmaIntensity(f);
         f.close();
         // Set uncertainty of position lines to 1/4 of sigma ... or 0
         for(vector<PeakList::hkl>::iterator pos=pl.mvHKL.begin();pos!=pl.mvHKL.end();++pos)
         //{   pos->d2obsmin=(3*pos->d2obs+pos->d2obsmin)/4; pos->d2obsmax=(3*pos->d2obs+pos->d2obsmax)/4;}
         { pos->d2obsmin=pos->d2obs; pos->d2obsmax=pos->d2obs;}
         
         CellExplorer cx(pl,TRICLINIC,0);
         cx.SetAngleMinMax((float)90*DEG2RAD,(float)120*DEG2RAD);
         
         // Use at most 20 lines ?
         if(pl.GetPeakList().size()>20) pl.GetPeakList().resize(20);
         unsigned int nb=pl.GetPeakList().size();
         if(nb>20) nb=20;// Use at most 20 peaks to estimate cell volume
         const float dmin=pl.GetPeakList()[nb-1].dobs;
         const float dmax=pl.GetPeakList()[0].dobs/10;// /10: assume no peaks at lower resolution

         const float vmin=EstimateCellVolume(dmin,dmax,nb,TRICLINIC  ,LATTICE_P,1.2);
         const float vmax=EstimateCellVolume(dmin,dmax,nb,TRICLINIC  ,LATTICE_P,0.2);
         
         float lengthmax=pow(vmax,(float)(1/3.0))*4;
         if(lengthmax<25)lengthmax=25;
         //if(lengthmax>(2.1/pl.GetPeakList()[0].dobs)) lengthmax=2.1/pl.GetPeakList()[0].dobs;
         
         cx.SetVolumeMinMax(vmin,vmax);
         cx.SetLengthMinMax(3,lengthmax);
         
         cx.DicVol(10,4,50,4);
         /*
         for(unsigned int i=0;;++i)
         {
            cout<<i<<endl;
            cx.Evolution(100,true,0.7,0.5,50);
            if(cx.GetBestScore()>40) break;
         }
         */
         TAU_REPORT_STATISTICS();
         exit(0);
      }
      if(STRCMP(_T("--index-test"),argv[i])==0)
      {
         ofstream out("indexing-results.txt");
         srand(time(NULL));
         for(unsigned int k=0;k<100;++k)
         {
            PeakList pl;
            float a,b,c,alpha,beta,gamma;
            while(true)
            {
               a=4+rand()/float(RAND_MAX)*20,
               b=4+rand()/float(RAND_MAX)*20,
               c=4+rand()/float(RAND_MAX)*20,
               alpha=50+rand()/float(RAND_MAX)*80,
               beta =50+rand()/float(RAND_MAX)*80,
               gamma=50+rand()/float(RAND_MAX)*80;
               
               //a=21.611; b= 4.407; c=16.848; alpha= 93.27; beta= 71.47; gamma= 87.13; //V= 1514.99   ;                                                        

               if( (alpha<(beta+gamma-5)) && (beta<(alpha+gamma-5)) && (gamma<(beta+alpha-5)) && ((alpha+beta+gamma)<355)) break;
            }
            const float missing=0.2;
            const float sigma=1e-4;
            const unsigned int nb=20;
            const unsigned int nbspurious=0;
            const float v=pl.Simulate(0,a,b,c,alpha,beta,gamma,true,nb,nbspurious,1e-4,missing,true);
            //v=pl.Simulate(0,10.317,9.414,13.178,87.90,89.76,74.10,true,20,0,0.);
            //v=pl.Simulate(0,10.451,12.884,7.072,86.91,96.07,83.36,true,20,0,0.);
            //v=pl.Simulate(21.611,4.407,16.848,93.27,71.47,87.13,1514.99,true,20,0,1e-4,0.2,true);                                                 

            pl.Print(cout);
            
            CellExplorer cx(pl,TRICLINIC,LATTICE_P);
            cx.SetAngleMinMax((float)90*DEG2RAD,(float)120*DEG2RAD);
            
            const float dmin=pl.GetPeakList()[nb-1].dobs;
            const float dmax=pl.GetPeakList()[0].dobs;// /10: assume no peaks at lower resolution

            const float vmin=EstimateCellVolume(dmin,dmax,nb,TRICLINIC  ,LATTICE_P,1.2);
            const float vmax=EstimateCellVolume(dmin,dmax,nb,TRICLINIC  ,LATTICE_P,0.5);
            
            float lengthmax=pow(vmax,(float)(1/3.0))*4;
            if(lengthmax<25)lengthmax=25;
            //if(lengthmax>(2.1/pl.GetPeakList()[0].dobs)) lengthmax=2.1/pl.GetPeakList()[0].dobs;
            cout<<"Indexing using TRICLINIC lattice, latt=3.0->"<<lengthmax<<"A, V="<<vmin<<"->"<<vmax<<"A^3"<<endl;
            
            cx.SetVolumeMinMax(vmin,vmax);
            //cx.SetVolumeMinMax(861.06299999999987,1599.117);
            //cx.SetVolumeMinMax(938.4*0.7,938.4*1.3);
            cx.SetLengthMinMax(3,lengthmax);
            Chronometer chrono;
            cx.DicVol(10,4,50,4);
            pl.Simulate(0,a,b,c,alpha,beta,gamma,true,20,0,0.);// Just to write the cell parameters
            const std::list< std::pair< RecUnitCell, float > >::const_iterator pos=cx.GetSolutions().begin();
            float score=0,vsol=0;
            if(pos!=cx.GetSolutions().end())
            {
               score=pos->second;
               vsol=pos->first.DirectUnitCell()[6];
            }
            char buf[200];
            sprintf(buf,"a=%6.3f b=%6.3f c=%6.3f alpha=%6.2f beta=%6.2f gamma=%6.2f V=%8.2f Vsol=%8.2f %d Score=%5.0f dt=%6.1fs (V=%5.0f->%5.0f, L=%4.1f->%4.1f, nb=%2d, spurious=%2d, missing=%2.0f%%, sigma=%5.3f%%)",
                    a,b,c,alpha,beta,gamma,v,vsol,int((abs(vsol-v)/v)<0.005),score,chrono.seconds(),vmin,vmax,3.0,lengthmax,nb,nbspurious,missing*100,sigma*100);
            out<<buf<<endl;
            /*
            for(unsigned int i=0;;++i)
            {
                cout<<i<<endl;
                cx.Evolution(100,true,0.7,0.5,50);
                if(cx.GetBestScore()>40) break;
            }
            */
            }
         TAU_REPORT_STATISTICS();
         exit(0);
      }
      #ifdef __DEBUG__
      if(STRCMP(_T("--debuglevel"),argv[i])==0)
      {
         long level;
         ++i;
         #ifdef __WX__CRYST__
         wxString(argv[i]).ToLong(&level);
         #else
         stringstream sstr(argv[i]);
         sstr >> level;
         #endif
         VFN_DEBUG_GLOBAL_LEVEL(level);
         continue;
      }
      #endif
      #ifdef __WX__CRYST__
      if(wxString(argv[i]).find(_T(".xml"))!=wxNOT_FOUND)
      #else
      if(string(argv[i]).find(string(".xml"))!=string::npos)
      #endif
      {
         #ifdef __WX__CRYST__
         cout<<"Loading: "<<wxString(argv[i]).ToAscii()<<endl;
         wxString name(argv[i]);
         if(name.size()>4)
            if(name.Mid(name.size()-4)==wxString(_T(".xml")))
            {
               wxFileInputStream is(name);
               stringstream sst;
               if(is.GetSize()>0)
               {
                  char * tmpbuf=new char[is.GetSize()+1];
                  is.Read(tmpbuf,is.GetSize());
                  sst<<tmpbuf;
                  delete[] tmpbuf;
               }
               else while (!is.Eof()) sst<<(char)is.GetC();
               try{XMLCrystFileLoadAllObject(sst);}
               catch(const ObjCrystException &except)
               {
                 wxMessageDialog d(NULL,_T("Failed loading file:\n")+name,_T("Error"),wxOK|wxICON_ERROR);
                 d.ShowModal();
              };
            }
            else
              if(name.size()>7)
                  if(name.Mid(name.size()-7)==wxString(_T(".xml.gz")))
                  {//compressed file
                      wxFileInputStream is(name);
                      wxZlibInputStream zstream(is);
                      stringstream sst;
                      while (!zstream.Eof()) sst<<(char)zstream.GetC();
                      try{XMLCrystFileLoadAllObject(sst);}
                      catch(const ObjCrystException &except)
                      {
                        wxMessageDialog d(NULL,_T("Failed loading file:\n")+name,_T("Error"),wxOK|wxICON_ERROR);
                        d.ShowModal();
                      };
                  }
         #else
         cout<<"Loading: "<<argv[i]<<endl;
         XMLCrystFileLoadAllObject(argv[i]);
         #endif
         if(!cif2pattern)continue;
         cout<<"Loading: Done"<<endl;
      }
      #ifdef __WX__CRYST__
      if(wxString(argv[i]).find(_T(".cif"))!=wxNOT_FOUND)
      #else
      if(string(argv[i]).find(string(".cif"))!=string::npos)
      #endif
      {
         #ifdef __WX__CRYST__
         cout<<"Loading: "<<wxString(argv[i]).ToAscii()<<endl;
         wxFileInputStream is(argv[i]);
         stringstream in;
         if(is.GetSize()>0)
         {
            char * tmpbuf=new char[is.GetSize()+1];
            is.Read(tmpbuf,is.GetSize());
            in<<tmpbuf;
            delete[] tmpbuf;
         }
         else while (!is.Eof()) in<<(char)is.GetC();
         #else
         cout<<"Loading: "<<argv[i]<<endl;
         ifstream in (argv[i]);
         #endif
         ObjCryst::CIF cif(in,true,true);
         CreateCrystalFromCIF(cif);
         CreatePowderPatternFromCIF(cif);
         CreateSingleCrystalDataFromCIF(cif);
         if(!cif2pattern)continue;
      }
      #ifdef __WX__CRYST__
      if(wxString(argv[i]).find(_T(".grd"))!=wxNOT_FOUND)
      #else
      if(string(argv[i]).find(string(".grd"))!=string::npos)
      #endif
      {
         loadFourierGRD=true;
         #ifdef __WX__CRYST__
         vFourierFilenameGRD.push_back(string(wxString(argv[i]).ToAscii()));
         #else
         vFourierFilenameGRD.push_back(argv[i]);
         #endif
         continue;
      }
      #ifdef __WX__CRYST__
      if( (wxString(argv[i]).find(_T(".dsn6"))!=wxNOT_FOUND)    || (wxString(argv[i]).find(_T(".dn6"))!=wxNOT_FOUND) )
      #else
      if( (string(argv[i]).find(string(".dsn6"))!=string::npos) || (string(argv[i]).find(string(".dn6"))!=string::npos) )
      #endif
      {
         loadFourierDSN6=true;
         #ifdef __WX__CRYST__
         vFourierFilenameDSN6.push_back(string(wxString(argv[i]).ToAscii()));
         #else
         vFourierFilenameDSN6.push_back(argv[i]);
         #endif
         continue;
      }
      if(cif2pattern || cif2patternN)
      {
         for(int j=0;j<gCrystalRegistry.GetNb();++j)
         {
            PowderPattern data;
            if(cif2patternN) data.SetRadiationType(RAD_NEUTRON);
            else data.SetRadiationType(RAD_XRAY);
            data.SetWavelength(cif2patternWavelength);
            data.SetPowderPatternPar(0,cif2patternMax2Theta/cif2patternNbPoint,cif2patternNbPoint);
            //add CaF2 as a Crystalline phase
            PowderPatternDiffraction * diffData=new PowderPatternDiffraction;
            diffData->SetCrystal(gCrystalRegistry.GetObj(j));
            diffData->SetReflectionProfilePar(PROFILE_PSEUDO_VOIGT,cif2patternPeakWidth*cif2patternPeakWidth);
            diffData->GetCrystal().SetUseDynPopCorr(true);
            data.SetMaxSinThetaOvLambda(50.0);
            data.AddPowderPatternComponent(*diffData);
            //we don't have data, so just simulate (0->Pi/2)..
            //give a constant 'obs pattern of unit intensity
            CrystVector_REAL obs(cif2patternNbPoint);
            obs=1;
            data.SetPowderPatternObs(obs);
            data.Prepare();
            // Save the powder pattern in text format
            stringstream sst;
            #ifdef __WX__CRYST__
            sst<<wxString(argv[i]).ToAscii()<<"_"<<j<<".dat";
            #else
            sst<<argv[i]<<"_"<<j<<".dat";
            #endif
            cout<<"Auto-simulating powder pattern:("<<cif2patternN<<")"<<endl
                <<"   Crystal #"<<j<<": "<<gCrystalRegistry.GetObj(j).GetName()<<endl
                <<"   Wavelength: "<<cif2patternWavelength<<endl
                <<"   2theta: 0->"<<cif2patternMax2Theta*RAD2DEG<<"?("<<cif2patternNbPoint<<" points)"<<endl
                <<"   peak width: "<<cif2patternPeakWidth*RAD2DEG<<"?"<<endl
                <<"   to FILE:"<<sst.str()<<endl;
            ofstream out(sst.str().c_str());
            CrystVector_REAL ttheta,icalc;
            icalc=data.GetPowderPatternCalc();
            icalc*=100/icalc.max();
            ttheta=data.GetPowderPatternX();
            if(data.GetRadiation().GetWavelengthType()!=WAVELENGTH_TOF) ttheta *= RAD2DEG;
            out << "#Simulated data for crystal:"<<gCrystalRegistry.GetObj(j).GetName() << endl;
            out << "#    2Theta/TOF    ICalc" << endl;
            out << FormatVertVector<REAL>(ttheta,icalc,12,4);
            out.close();
         }
         // Erase every data in memory ?
         //gOptimizationObjRegistry.DeleteAll();
         //gDiffractionDataSingleCrystalRegistry.DeleteAll();
         //gPowderPatternRegistry.DeleteAll();
         //gCrystalRegistry.DeleteAll();
         break;
      }
      cout <<"command-line arguments:"<<endl
           <<"   in.xml: input 'in.xml' file"<<endl
           <<"   structure.cif: input 'structure.cif' CIF file"<<endl
           <<"   --loadfouriergrd map.grd: load and display 'map.grd' fourier map with (first) crystal structure"<<endl
           <<"                             the --loadfouriergrd keyword can be omitted if the file extension is .grd"<<endl
           <<"   --loadfourierdsn6 map.DN6: load and display a DSN6 fourier map with (first) crystal structure"<<endl
           <<"                             the --loadfourierdsn6 keyword can be omitted if the file extension is .dsn6 or .dn6"<<endl
           <<"   --nogui: run without GUI, automatically launches optimization"<<endl
           <<"      options with --nogui:"<<endl
           <<"         -n 10000     : run for 10000 trials at most (default: 1000000)"<<endl
           <<"         --nbrun 5     : do 5 runs, randomizing before each run (default: 1), use -1 to run indefinitely"<<endl
           <<"         -o out.xml   : output in 'out.xml'"<<endl
           <<"         --randomize  : randomize initial configuration"<<endl
           <<"         --silent     : (almost) no text output"<<endl
           <<"         --finalcost 0.15 : run optimization until cost < 0.15"<<endl
           <<"         --cif2pattern 1.5406 170 5000 .1 outfile:"<<endl
           <<"                               simulate pattern for input crystal, wavelength=1.5406"<<endl
           <<"                               up to 170deg with 5000 points and a peak width of 0.1 deg"<<endl
           <<"                               and save to file outfile%d.dat"<<endl
           <<endl<<endl<<"           EXAMPLES :"<<endl<<endl
           <<"Load file 'silicon.xml' and launch GUI:"<<endl<<endl
           <<"    Fox silicon.xml"<<endl<<endl

           <<"Load file 'alumina.cif' and launch GUI:"<<endl<<endl
           <<"    Fox alumina.xml"<<endl<<endl

           <<"Load file 'alumina.cif', import Fourier map from the 'alumina.grd' file, and launch GUI with the automatic 3D display:"<<endl<<endl
           <<"    Fox alumina.cif alumina.grd"<<endl<<endl
           
           <<"Load file 'ktartrate.xml', randomize, then make 1 optimization of "<<endl
           <<"1 million trials, and save the best structure in 'best.xml' :"<<endl<<endl
           <<"    Fox Cimetidine-powder.xml --nogui --randomize -n 1000000 -o best.xml"<<endl<<endl

           <<"Load file 'Cimetidine-powder.xml', then make 10 runs (starting from "<<endl
           <<"a random structure) of 10 million trials (each run saves one xml file)"<<endl
           <<", and save the best structure in 'best.xml' :"<<endl<<endl
           <<"    Fox Cimetidine-powder.xml --nogui --randomize -n 10000000 --nbrun 10 -o best.xml"<<endl<<endl
           
           <<"Load file 'Cimetidine-powder.xml', then make 10 silent runs of 10 million trials"<<endl
           <<" (each run saves one xml file), and save the best structure in 'best.xml'."<<endl
           <<" For each run, the optimization stops if the cost goes below 200000."<<endl<<endl
           <<"    Fox Cimetidine-powder.xml --nogui --silent --randomize -n 10000000 --nbrun 10 --finalcost 200000 -o best.xml"<<endl<<endl
           <<endl;
      exit(0);  
   }
   if(fitprofile)
   {
      // Do a Le Bail + profile fitting on all powder patterns which have at least one crystalline phase
      const unsigned int nbLeBail=2;//2*5
      for(unsigned int i=0;i<gPowderPatternRegistry.GetNb();++i)
      {
        set<PowderPatternDiffraction *> vpDiff;
        // Multiple phases ?
        unsigned int nbcomp=gPowderPatternRegistry.GetObj(i).GetNbPowderPatternComponent();
        for(unsigned int k=0;k<nbcomp;++k)
          if(gPowderPatternRegistry.GetObj(i).GetPowderPatternComponent(k).GetClassName()==string("PowderPatternDiffraction"))
          {
            PowderPatternDiffraction *pDiff=dynamic_cast<PowderPatternDiffraction*>(&(gPowderPatternRegistry.GetObj(i).GetPowderPatternComponent(k)));
            if(pDiff!=0) vpDiff.insert(pDiff);
          }
        LSQNumObj lsq;
        lsq.SetRefinedObj(gPowderPatternRegistry.GetObj(i),0,true,true);
        lsq.PrepareRefParList(true);
        lsq.SetParIsUsed(gpRefParTypeScatt,false);
        lsq.SetParIsUsed(gpRefParTypeScattPow,false);

        bool fitzero=false,fitwidth0=false,fitwidth=false,fiteta=false,
             fitasym=false,fitdispltransp=false,fitbackgd=false,fitcell=false,
             fitTOFInstWidth=false,fitTOFBroadening=false;
        if(gPowderPatternRegistry.GetObj(i).GetRadiation().GetWavelengthType()==WAVELENGTH_TOF)
        {
          fitzero=true;
          fitTOFInstWidth=true;
          fitTOFBroadening=true;
          fitbackgd=true;
          fitcell=true;
        }
        else
        {
          fitzero=true;
          fitwidth0=true;
          fitwidth=true;
          fiteta=true;
          fitasym=true;
          fitdispltransp=true;
          fitbackgd=true;
          fitcell=true;
        }
        cout<<endl<<endl<<"Performing Le Bail + Profile Fit on:"<<gPowderPatternRegistry.GetObj(i).GetName()<<",   including the crystalline phases:"<<endl;
        for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
           cout<<"    "<<(*pos)->GetName()<<"(Crystal:"<<(*pos)->GetCrystal().GetName()<<")"<<endl;
        try
        {
          lsq.SetParIsFixed(gpRefParTypeScattDataScale,false);
          
          if(fitzero) lsq.SetParIsFixed(gPowderPatternRegistry.GetObj(i).GetPar("Zero"),false);
          if(fitwidth0) 
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              lsq.SetParIsFixed((*pos)->GetProfile().GetPar("W"),false);
          if(fitzero||fitwidth0)
          {
            lsq.Refine(5,true,true);
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              (*pos)->ExtractLeBail(2);
          }
          if(fitwidth) 
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              lsq.SetParIsFixed((*pos)->GetProfile().GetPar("U"),false);
          if(fitwidth) 
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              lsq.SetParIsFixed((*pos)->GetProfile().GetPar("V"),false);
          if(fiteta) 
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              lsq.SetParIsFixed((*pos)->GetProfile().GetPar("Eta0"),false);
          if(fitwidth||fiteta)
          {
              lsq.Refine(5,true,true);
              for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
                (*pos)->ExtractLeBail(2);
          }
          
          if(fitTOFInstWidth)
          {// TOF
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
            {
              lsq.SetParIsFixed((*pos)->GetProfile().GetPar("Alpha1"),false);
              lsq.SetParIsFixed((*pos)->GetProfile().GetPar("Beta0"),false);
              lsq.SetParIsFixed((*pos)->GetProfile().GetPar("Beta1"),false);
            }
            lsq.Refine(5,true,true);
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              (*pos)->ExtractLeBail(2);
          }
          if(fitTOFBroadening)
          {// TOF
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
            {
              lsq.SetParIsFixed((*pos)->GetProfile().GetPar("GaussianSigma1"),false);
              //lsq.SetParIsFixed(pos->GetProfile().GetPar("LorentzianGamma2"),false);
              //lsq.SetParIsFixed(pos->GetProfile().GetPar("GaussianSigma1"),0,1e6);
              //lsq.SetParIsFixed(pos->GetProfile().GetPar("LorentzianGamma2"),0,1e6);
            }
            lsq.Refine(5,true,true);
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              (*pos)->ExtractLeBail(2);
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              lsq.SetParIsFixed((*pos)->GetProfile().GetPar("GaussianSigma1"),true);
          }
          
          if(fiteta) 
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              lsq.SetParIsFixed((*pos)->GetProfile().GetPar("Eta1"),false);
          if(fiteta)
          {
            lsq.Refine(5,true,true);
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              (*pos)->ExtractLeBail(2);
          }
          
          if(fitasym) 
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              lsq.SetParIsFixed((*pos)->GetProfile().GetPar("Asym0"),false);
          if(fitasym) 
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              lsq.SetParIsFixed((*pos)->GetProfile().GetPar("Asym1"),false);
          if(fitasym) 
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              lsq.SetParIsFixed((*pos)->GetProfile().GetPar("Asym2"),false);
          if(fitdispltransp) 
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              lsq.SetParIsFixed((*pos)->GetParentPowderPattern().GetPar("2ThetaDispl"),false);
          if(fitdispltransp)  
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              lsq.SetParIsFixed((*pos)->GetParentPowderPattern().GetPar("2ThetaTransp"),false);
          if(fitdispltransp||fitasym)
          {
            lsq.Refine(5,true,true);
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              (*pos)->ExtractLeBail(2);
          }
          
          if(fitbackgd)
          {
            lsq.SetParIsFixed(gpRefParTypeScattDataBackground,false);
            // Make sure points beyond max resolution are not optimized
            for(unsigned int k=0;k<nbcomp;++k)
              if(gPowderPatternRegistry.GetObj(i).GetPowderPatternComponent(k).GetClassName()=="PowderPatternBackground")
              {
                PowderPatternBackground *pback=dynamic_cast<PowderPatternBackground *> (&(gPowderPatternRegistry.GetObj(i).GetPowderPatternComponent(k)));
                pback->FixParametersBeyondMaxresolution(lsq.GetCompiledRefinedObj());
              }
  
            lsq.Refine(5,true,true);
            for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
              (*pos)->ExtractLeBail(2);
          }
          
          if(fitcell)
          {
              lsq.SetParIsFixed(gpRefParTypeUnitCell,false);
              
              lsq.Refine(5,true,true);
              for(set<PowderPatternDiffraction *>::iterator pos=vpDiff.begin();pos!=vpDiff.end();++pos)
                (*pos)->ExtractLeBail(2);
          }
        }
        catch(const ObjCrystException &except)
        {
           cout<<"Oups: automatic profile fit + Le Bail went wrong, please try manual fit within GUI"<<endl;
        }
      }
      /*
      XMLCrystFileSaveGlobal(outfilename);
      #ifdef __WX__CRYST__
      this->OnExit();
      #else
      TAU_REPORT_STATISTICS();
      #endif
      exit(0);
      */
   }
   if(exportfullprof)
   {
      // Find every powder pattern, export to fullprof
      for(unsigned int i=0;i<gPowderPatternRegistry.GetNb();++i)
      {
         gPowderPatternRegistry.GetObj(i).ExportFullprof("test");
      }
      #ifdef __WX__CRYST__
      this->OnExit();
      #endif
      exit(0);
   }
   
   if(cif2pattern)
   {
      #ifdef __WX__CRYST__
      this->OnExit();
      #endif
      exit(0);
   }
   if(randomize)
      for(int i=0;i<gOptimizationObjRegistry.GetNb();i++)
         gOptimizationObjRegistry.GetObj(i).RandomizeStartingConfig();
   
#ifndef __WX__CRYST__
   useGUI=false;
#endif
   if(testLSQ)
   {
      for(int i=0;i<gOptimizationObjRegistry.GetNb();i++)
      {
         MonteCarloObj *pMonteCarloObj=dynamic_cast<MonteCarloObj *>(&(gOptimizationObjRegistry.GetObj(i)));
         Chronometer chrono;
         pMonteCarloObj->InitLSQ(false);
         cout<<"Fox: running LSQ test for profiling"<<endl;
         chrono.start();
         pMonteCarloObj->GetLSQObj().Refine(100,true,true,true);
         cout<<" LSQ tests - SUCCESS -, elapsed time for 100 cycles (full pattern=true ):"<<chrono.seconds()<<endl;
      }
      //TAU_REPORT_STATISTICS();
      #ifdef __WX__CRYST__
      this->OnExit();
      #endif
      return 0;
   }
   if(testMC)
   {
      for(int i=0;i<gOptimizationObjRegistry.GetNb();i++)
      {
         Chronometer chrono;
         cout<<"Fox: running LSQ test for profiling"<<endl;
         chrono.start();
         long nbtrial=10000;
         gOptimizationObjRegistry.GetObj(i).Optimize(nbtrial,true,0);
         cout<<" MC tests - SUCCESS -, elapsed time for 10000 trials:"<<chrono.seconds()<<endl;
      }
      //TAU_REPORT_STATISTICS();
      #ifdef __WX__CRYST__
      this->OnExit();
      #endif
      return 0;
   }
   if(testSPEED)
   {
      standardSpeedTest();
      //TAU_REPORT_STATISTICS();
      #ifdef __WX__CRYST__
      this->OnExit();
      #endif
      return 0;
   }
   if(!useGUI)
   {
      if(nbTrial!=0)
      {
         if(nbRun==1)
         {
            for(int i=0;i<gOptimizationObjRegistry.GetNb();i++)
                  gOptimizationObjRegistry.GetObj(i).Optimize(nbTrial,silent,finalCost);
         }
         else
         {
            for(int i=0;i<gOptimizationObjRegistry.GetNb();i++)
               gOptimizationObjRegistry.GetObj(i).GetXMLAutoSaveOption().SetChoice(5);
            for(int i=0;i<gOptimizationObjRegistry.GetNb();i++)
               gOptimizationObjRegistry.GetObj(i).MultiRunOptimize(nbRun,nbTrial,silent,finalCost);
         }
      }
      string tmpstr=outfilename;
      if(filenameInsertCost>=0)
      {
         char costAsChar[50];
         sprintf(costAsChar,"-Cost-%f",gOptimizationObjRegistry.GetObj(0).GetLogLikelihood());
         string tmpstr2=costAsChar;
         tmpstr.replace(filenameInsertCost,5,tmpstr2,0,tmpstr2.length());
      }
      XMLCrystFileSaveGlobal(tmpstr);
      cout <<"End of Fox execution. Bye !"<<endl;
      //TAU_REPORT_STATISTICS();
      #ifdef __WX__CRYST__
      this->OnExit();
      #endif
      exit(0);
   }
#ifdef __WX__CRYST__
   this->SetVendorName(_T("http://objcryst.sf.net/Fox"));
   this->SetAppName(_T("FOX-Free Objects for Crystallography"));
   // Read (and automatically create if necessary) global Fox preferences
   // We explicitely use a wxFileConfig, to avoid the registry under Windows
   wxConfigBase::Set(new wxFileConfig(_T("FOX-Free Objects for Crystallography")));

   if(wxConfigBase::Get()->HasEntry(_T("Fox/BOOL/Enable tooltips")))
   {
       bool tooltip_enabled;
       wxConfigBase::Get()->Read(_T("Fox/BOOL/Enable tooltips"), &tooltip_enabled);
       wxToolTip::Enable(tooltip_enabled);
       wxToolTip::SetDelay(500);
   }
   else wxConfigBase::Get()->Write(_T("Fox/BOOL/Enable tooltips"), true);
   
   if(!wxConfigBase::Get()->HasEntry(_T("Fox/BOOL/Ask confirmation before exiting Fox")))
      wxConfigBase::Get()->Write(_T("Fox/BOOL/Ask confirmation before exiting Fox"), true);
   
   if(!wxConfigBase::Get()->HasEntry(_T("Fox/BOOL/Use compressed file format (.xml.gz)")))
      wxConfigBase::Get()->Write(_T("Fox/BOOL/Use compressed file format (.xml.gz)"), true);

   if(!wxConfigBase::Get()->HasEntry(_T("Fox/BOOL/Check for Fox updates")))
      wxConfigBase::Get()->Write(_T("Fox/BOOL/Check for Fox updates"), true);

   WXCrystMainFrame *frame ;
   string title(string("FOX: Free Objects for Xtal structures v")+foxVersion);
   frame = new WXCrystMainFrame(wxString::FromAscii(title.c_str()),
                                 wxPoint(50, 50), wxSize(600, 600),
                                 !(loadFourierGRD||loadFourierDSN6||runclient));
   // Use the main frame status bar to pass messages to the user
      pMainFrameForUserMessage=frame;
      fpObjCrystInformUser=&WXCrystInformUserStdOut;
   
   WXCrystal *pWXCryst;
   if(loadFourierGRD || loadFourierDSN6)
   {
      //wxFrame *pWXFrame= new wxFrame((wxFrame *)NULL, -1, "FOX", wxPoint(50, 50), wxSize(550, 400));
      //wxScrolledWindow *pWXScWin=new wxScrolledWindow(pWXFrame,-1);
      //WXCrystal * pWXCrystal=new WXCrystal(pWXScWin,&(gCrystalRegistry.GetObj(0)));
      pWXCryst=dynamic_cast<WXCrystal*> (gCrystalRegistry.GetObj(0).WXGet());
      wxCommandEvent com;
      pWXCryst->OnMenuCrystalGL(com);
   }
   if(loadFourierGRD)
   {
      list<string>::iterator pos;
      for(pos=vFourierFilenameGRD.begin();pos!=vFourierFilenameGRD.end();++pos)
      {
         UnitCellMap *pMap=new UnitCellMap(pWXCryst->GetCrystal());
         cout<<"Reading Fourier file:"<<*pos<<endl;
         if (pMap->ImportGRD(*pos) == 0)
         {
            cout<<"Error reading Fourier file:"<< *pos<<endl;
            return FALSE;
         }
         pWXCryst->GetCrystalGL()->AddFourier(pMap);
      }
   }
   if(loadFourierDSN6)
   {
      list<string>::iterator pos;
      for(pos=vFourierFilenameDSN6.begin();pos!=vFourierFilenameDSN6.end();++pos)
      {
         UnitCellMap *pMap=new UnitCellMap(pWXCryst->GetCrystal());
         cout<<"Reading Fourier file:"<<*pos<<endl;
         if (pMap->ImportDSN6(*pos) == 0)
         {
            cout<<"Error reading Fourier file:"<< *pos<<endl;
            return FALSE;
         }
         pWXCryst->GetCrystalGL()->AddFourier(pMap);
      }
   }
   if(runclient)
   { 
      wxString dir = wxPathOnly(argv[0]);
      wxSetWorkingDirectory(dir);
      wxCommandEvent com;
      //frame->OnStartGridClient(com);   
      frame->mpGridWindow->m_working_dir = working_dir;
      frame->mpGridWindow->StartClientWindow();    

      if(nbCPUs!=-1) {
          frame->mpGridWindow->m_WXFoxClient->setNbCPU(nbCPUs);
      }
      frame->mpGridWindow->m_WXFoxClient->m_IPWindow->SetValue(wxString::FromAscii(IP.c_str()));
      frame->mpGridWindow->m_WXFoxClient->OnConnectClient(com);
   }

   return TRUE;
#else
   return 0;
#endif
}
#ifdef __WX__CRYST__
int MyApp::OnExit()
{
   delete wxConfigBase::Set((wxConfigBase *) NULL);
   TAU_REPORT_STATISTICS();
   return this->wxApp::OnExit();
}

// ----------------------------------------------------------------------------
// WXCrystScrolledWindow
// ----------------------------------------------------------------------------
WXCrystScrolledWindow::WXCrystScrolledWindow(wxWindow* parent):
wxScrolledWindow(parent),mpChild((wxWindow*)0),mHeight(-1),mWidth(-1)
{
   mpSizer=new wxBoxSizer(wxHORIZONTAL);
   this->SetSizer(mpSizer);
   this->FitInside();
   this->SetScrollRate(10,10);
}

bool WXCrystScrolledWindow::Layout()
{
   //this->Scroll(0,0);//workaround bug ?
   return this->wxScrolledWindow::Layout();
}

void WXCrystScrolledWindow::SetChild(wxWindow* pChild)
{
   mpChild=pChild;
   mpSizer->Add(mpChild);
   // Initialize scrollbars
   //this->SetScrollbars(40,40,2,2);
}

#if (wxVERSION_NUMBER>=2808) && (wxVERSION_NUMBER<=2811)
void WXCrystScrolledWindow::OnWXCrystChildFocus(wxChildFocusEvent& event)
{
   // Workaround for wx 2.8.8+ bug
   //event.Skip();
}
#endif
// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

WXCrystMainFrame::WXCrystMainFrame(const wxString& title, const wxPoint& pos, const wxSize& size,
                                   const bool splashscreen)
       : wxFrame((wxFrame *)NULL, -1, title, pos, size),mvUpdatesAutoCheck(false)
{
#ifdef __WXMAC__
   // we need this in order to allow the about menu relocation, since ABOUT is
   // not the default id of the about menu
   wxApp::s_macAboutMenuItemId = MENU_HELP_ABOUT;
#endif

   // create a menu bar
      wxMenu *menuFile = new wxMenu;//
         menuFile->Append(MENU_FILE_LOAD, _T("&Open .xml or .cif\tCtrl-O"), _T("Open Fox (.xml, .xml.gz) or CIF file"));
         menuFile->Append(MENU_FILE_CLOSE, _T("Close\tCtrl-W"), _T("Close all"));
         menuFile->Append(MENU_FILE_SAVE, _T("&Save\tCtrl-S"), _T("Save Everything..."));
         menuFile->Append(MENU_FILE_QUIT, _T("E&xit\tCtrl-Q"), _T("Quit "));
         menuFile->AppendSeparator();
         menuFile->Append(MENU_FILE_BROWSE, _T("Browse .xml, .xml.gz or .cif files..."), _T("Browse .xml, .xml.gz or .cif files..."));
      
      wxMenu *objectMenu = new wxMenu(_T(""), wxMENU_TEAROFF);
         objectMenu->Append(MENU_OBJECT_CREATE_CRYSTAL, _T("New Crystal"),
                           _T("Add a new Crystal structure"));
         objectMenu->Append(MENU_OBJECT_CREATE_POWDERSPECTRUM, _T("New PowderPattern"),
                           _T("Add a new PowderPattern Object"));
         objectMenu->Append(MENU_OBJECT_CREATE_SINGLECRYSTALDATA, _T("New Single Crystal Diffraction"),
                           _T("Add a new Single Crystal Diffraction Object"));
         objectMenu->Append(MENU_OBJECT_CREATE_GLOBALOPTOBJ, _T("New Monte-Carlo Object"),
                           _T("Add a new Monte-Carlo Object"));

         //FoxGrid////////////////////////////////////////////////////////////////////
      wxMenu *gridMenu = new wxMenu;
         gridMenu->Append(MENU_GRID_SERVER_RUN, _T("&Run Server"), _T("Start Fox Grid Server"));
         gridMenu->AppendSeparator();
         gridMenu->Append(MENU_GRID_CLIENT_START, _T("&Start Client"), _T("Start Fox Grid Client"));
      
      wxMenu *prefsMenu = new wxMenu;
         prefsMenu->Append(MENU_PREFS_PREFERENCES, _T("&Preferences..."), _T("Fox Preferences..."));
      
      wxMenu *helpMenu = new wxMenu;
         helpMenu->Append(MENU_HELP_ABOUT, _T("&About..."), _T("About ObjCryst..."));
         helpMenu->Append(MENU_HELP_TOGGLETOOLTIP, _T("Toggle Tooltips"), _T("Set Tooltips on/off"));
         helpMenu->Append(MENU_HELP_UPDATE, _T("Check for Updates"), _T("Check for a newer version of Fox"));
      wxMenuBar *menuBar = new wxMenuBar();
         menuBar->Append(menuFile,  _T("&File"));
         menuBar->Append(objectMenu,_T("&Objects"));
         //FoxGrid/////////////////////////////
         menuBar->Append(gridMenu,_T("&FOXGrid"));
         menuBar->Append(prefsMenu, _T("&Preferences"));
         menuBar->Append(helpMenu,  _T("&Help"));
         #ifdef __DEBUG__
         wxMenu *debugMenu = new wxMenu;
            debugMenu->Append(MENU_DEBUG_TEST1, _T("Test #1"));
            debugMenu->Append(MENU_DEBUG_TEST2, _T("Test #2"));
            debugMenu->Append(MENU_DEBUG_TEST3, _T("Test #3"));
            debugMenu->Append(MENU_DEBUG_LEVEL0, _T("Debug level 0 (lots of messages)"));
            debugMenu->Append(MENU_DEBUG_LEVEL1, _T("Debug level 1"));
            debugMenu->Append(MENU_DEBUG_LEVEL2, _T("Debug level 2"));
            debugMenu->Append(MENU_DEBUG_LEVEL3, _T("Debug level 3"));
            debugMenu->Append(MENU_DEBUG_LEVEL4, _T("Debug level 4"));
            debugMenu->Append(MENU_DEBUG_LEVEL5, _T("Debug level 5"));
            debugMenu->Append(MENU_DEBUG_LEVEL6, _T("Debug level 6"));
            debugMenu->Append(MENU_DEBUG_LEVEL7, _T("Debug level 7"));
            debugMenu->Append(MENU_DEBUG_LEVEL8, _T("Debug level 8"));
            debugMenu->Append(MENU_DEBUG_LEVEL9, _T("Debug level 9"));
            debugMenu->Append(MENU_DEBUG_LEVEL10,_T("Debug level 10 (few messages)"));
         menuBar->Append(debugMenu,  _T("&Debug"));
         #endif

   SetMenuBar(menuBar);

#if wxUSE_STATUSBAR
   CreateStatusBar(1);
   SetStatusText(_T("Welcome to FOX/ObjCryst++!"));
#endif // wxUSE_STATUSBAR

   //Splash Screen
   if(true==splashscreen)
   {
      wxCommandEvent event;
      this->OnAbout(event);
   }
   // Create the notebook

      mpNotebook = new wxNotebook(this, -1);

      wxLayoutConstraints* c = new wxLayoutConstraints;
      c->left.SameAs(this, wxLeft, 2);
      c->right.SameAs(this, wxRight, 2);
      c->top.SameAs(this, wxTop, 2);
      c->bottom.SameAs(this, wxBottom, 2);

      mpNotebook->SetConstraints(c);

   // First window -Crystals
      WXCrystScrolledWindow *mpWin1 = new WXCrystScrolledWindow(mpNotebook);
      mpWin1->SetChild(gCrystalRegistry.WXCreate(mpWin1));
      mpWin1->Layout();
      mpNotebook->AddPage(mpWin1, _T("Crystals"), TRUE);

   // Second window - PowderPattern
      WXCrystScrolledWindow *mpWin2 = new WXCrystScrolledWindow(mpNotebook);
      mpWin2->SetChild(gPowderPatternRegistry.WXCreate(mpWin2));
      mpWin2->Layout();
      mpNotebook->AddPage(mpWin2,_T("Powder Diffraction"),true);
      
   // Third window - SingleCrystal
      WXCrystScrolledWindow *mpWin3 = new WXCrystScrolledWindow(mpNotebook);
      mpWin3->SetChild(gDiffractionDataSingleCrystalRegistry.WXCreate(mpWin3));
      mpWin3->Layout();
      mpNotebook->AddPage(mpWin3,_T("Single Crystal Diffraction"),true);
      
   // Fourth window - Global Optimization
      WXCrystScrolledWindow *mpWin4 = new WXCrystScrolledWindow(mpNotebook);
      mpWin4->SetChild(gOptimizationObjRegistry.WXCreate(mpWin4));
      mpWin4->Layout();
      mpNotebook->AddPage(mpWin4,_T("Global Optimization"),true);
       // Fift window - FoxGrid
      WXCrystScrolledWindow *mpWin5 = new WXCrystScrolledWindow(mpNotebook);
      mpGridWindow = new WXGrigWindow(mpWin5);
      mpWin5->SetChild(mpGridWindow);
      mpWin5->Layout();
      mpNotebook->AddPage(mpWin5,_T("FOXGrid"),true);


   this->SetIcon(wxICON(Fox));
   this->Show(TRUE);
   this->Layout();
   mpNotebook->SetSelection(0);
   
   // Set tooltip delay
   wxToolTip::SetDelay(500);
   // Reset "last save" clock, in the case we loaded an xml file on startup
   mClockLastSave.Click();
   
   #if 0
   // Check for updates in a separate thread
   bool check;
   wxConfigBase::Get()->Read("Fox/BOOL/Check for Fox updates", &check);
   if(check || ((rand()%10)==0))
   {
      WXThreadCheckUpdates *pThreadCheckUpdates = new WXThreadCheckUpdates(mvUpdates,*this);
      if(pThreadCheckUpdates->Create() != wxTHREAD_NO_ERROR) 
         wxLogError("Can't create updates check thread");
      else pThreadCheckUpdates->Run();
      mvUpdatesAutoCheck=true;
   }
   #endif
}

void WXCrystMainFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
   this->SafeQuit();
}

class WXDialogFoxAbout:public wxDialog
{
  public:
    WXDialogFoxAbout(wxWindow* parent);
    void OnButtonCheckUpdate(wxCommandEvent &ev);
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(WXDialogFoxAbout, wxDialog)
   EVT_BUTTON(ID_ABOUT_FOX_BUTTON_UPDATE,             WXDialogFoxAbout::OnButtonCheckUpdate)
END_EVENT_TABLE()

WXDialogFoxAbout::WXDialogFoxAbout(wxWindow* parent):
wxDialog(parent,-1,_T("About Fox"),wxDefaultPosition,wxDefaultSize,wxCAPTION|wxSTAY_ON_TOP|wxCLOSE_BOX)
{
   wxBoxSizer *sizer=new wxBoxSizer(wxVERTICAL);
   string msg(string("F.O.X. - Free Objects for Xtallography\n")
              +"Version "+ foxVersion +" \n\n"
              +"(c) 2000-2011 Vincent FAVRE-NICOLIN, vincefn@users.sourceforge.net\n"
              +"                                   , University of Grenoble\n"
              +"    2000-2001 Radovan CERNY, University of Geneva\n"
              +"    2009-2010 Jan Rohlicek, Michal Husak (Inst. Chem. Tech, Prague)\n\n"
              +"http://objcryst.sourceforge.net\n\n"
              +"FOX comes with ABSOLUTELY NO WARRANTY. It is free software, and you are\n"
              +"welcome to redistribute it under certain conditions. \n"
              +"See the LICENSE file for details.\n\n");
   wxStaticText *txt=new wxStaticText(this,-1,wxString::FromAscii(msg.c_str()),wxDefaultPosition,wxDefaultSize,wxALIGN_LEFT);
   sizer->Add(txt);
   wxBoxSizer *sizer2=new wxBoxSizer(wxHORIZONTAL);
   sizer2->Add(this->CreateButtonSizer(wxOK));
   sizer2->Add(new wxButton(this,ID_ABOUT_FOX_BUTTON_UPDATE,_T("Check for Updates")) );
   sizer->Add(0,4,0);
   sizer->Add(sizer2);
   this->SetSizer(sizer);
   this->Fit();
}

void WXDialogFoxAbout::OnButtonCheckUpdate(wxCommandEvent &ev)
{
   this->EndModal(int(ID_ABOUT_FOX_BUTTON_UPDATE));
}

void WXCrystMainFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
   WXDialogFoxAbout ab(this);
   if(ab.ShowModal()==ID_ABOUT_FOX_BUTTON_UPDATE)
   {
      wxCommandEvent ev(0,MENU_HELP_UPDATE);
      this->OnCheckUpdate(ev);
   }
}
void WXCrystMainFrame::OnLoad(wxCommandEvent& event)
{
   // First record if any object already in memory need to be saved
   bool saved=true;
   for(int i=0;i<gRefinableObjRegistry.GetNb();i++)
      if(gRefinableObjRegistry.GetObj(i).GetClockMaster()>mClockLastSave)
      {
         saved=false;
         break;
      }

   wxFileDialog *open;
   if(event.GetId()==MENU_FILE_LOAD)
   {
      open= new wxFileDialog(this,_T("Choose File :"),
                             _T(""),_T(""),_T("FOX files (*.xml,*.xml.gz) or CIF (*.cif)|*.xml;*.xml.gz;*.cif"),wxFD_OPEN | wxFD_FILE_MUST_EXIST);
      if(open->ShowModal() != wxID_OK) return;
      wxString name=open->GetPath();
      this->Load(name);
   }
   open->Destroy();
   if(saved) mClockLastSave.Click();
}

void WXCrystMainFrame::Load(const wxString &filename)
{
  if(filename.size()>4)
    if(filename.Mid(filename.size()-4)==wxString(_T(".xml")))
    {
        wxFileInputStream is(filename);
        stringstream in;
        if(is.GetSize()>0)
        {
          char * tmpbuf=new char[is.GetSize()];
          is.Read(tmpbuf,is.GetSize());
          in<<tmpbuf;
          delete[] tmpbuf;
        }
        else while (!is.Eof()) in<<(char)is.GetC();
        try{XMLCrystFileLoadAllObject(in);}
        catch(const ObjCrystException &except)
        {
          wxMessageDialog d(this,_T("Failed loading file1:\n")+filename,_T("Error loading file"),wxOK|wxICON_ERROR);
          d.ShowModal();
          return;
        };
        //FoxGrid
        mpGridWindow->DataLoaded();
    }
    else
      if(filename.Mid(filename.size()-4)==wxString(_T(".cif")))
      {
        wxFileInputStream is(filename);
        stringstream in;
        if(is.GetSize()>0)
        {
            char * tmpbuf=new char[is.GetSize()];
            is.Read(tmpbuf,is.GetSize());
            in<<tmpbuf;
            delete[] tmpbuf;
        }
        else while (!is.Eof()) in<<(char)is.GetC();
        ObjCryst::CIF cif(in,true,true);
        CreateCrystalFromCIF(cif);
        CreatePowderPatternFromCIF(cif);
        CreateSingleCrystalDataFromCIF(cif);
        //FoxGrid
        mpGridWindow->DataLoaded();
      }
      else
        if(filename.size()>6)
            if(filename.Mid(filename.size()-6)==wxString(_T("xml.gz")))
            {//compressed file
              wxFileInputStream is(filename);
              wxZlibInputStream zstream(is);
              stringstream sst;
              while (!zstream.Eof()) sst<<(char)zstream.GetC();
              try{XMLCrystFileLoadAllObject(sst);}
              catch(const ObjCrystException &except)
              {
                wxMessageDialog d(this,_T("Failed loading file2:\n")+filename,_T("Error loading file"),wxOK|wxICON_ERROR);
                d.ShowModal();
                return;
              };
              //FoxGrid
              mpGridWindow->DataLoaded();
            }
}

void WXCrystMainFrame::OnBrowse(wxCommandEvent& event)
{
  wxDirDialog dirdialog(this,_T("Choose directory with Fox .xml files"));
  const int result=dirdialog.ShowModal();
  if(result==wxID_CANCEL) return;
  mBrowseDir=dirdialog.GetPath();
  wxDir wxBrowseDir;
  if(wxBrowseDir.Open(mBrowseDir)==false) return;
  wxMiniFrame *frame= new wxMiniFrame(this,ID_FOX_BROWSE, _T("Fox .xml file browsing"),
                                      wxDefaultPosition,wxSize(500,500),wxCLOSE_BOX|wxCAPTION);
  wxArrayString choices;
  mpBrowseList=new wxListBox(frame, ID_FOX_BROWSE, wxDefaultPosition, 
                             wxDefaultSize, choices,
                             wxLB_SINGLE|wxLB_NEEDED_SB, wxDefaultValidator,
                             _T("listBox"));
  wxString filename;
  bool cont = wxBrowseDir.GetFirst(&filename,_T(""), wxDIR_FILES);
  while(cont)
  {
    if(wxNOT_FOUND==filename.Find(_T("~")))
    {
      if(wxNOT_FOUND!=filename.Find(_T(".xml"))) 
      {
        mpBrowseList->Append(filename);
      }
      if(wxNOT_FOUND!=filename.Find(_T(".cif")))
      {
        mpBrowseList->Append(filename);
      }
    }
    cont = wxBrowseDir.GetNext(&filename);
  }
  mpBrowseList->SetEventHandler(this);
  frame->Show(true);
}

void WXCrystMainFrame::OnBrowseSelect(wxCommandEvent &event)
{
  if(false==event.IsSelection()) return;
  this->Close(false);
  wxArrayInt selections;
  mpBrowseList->GetSelections(selections);
  for(int i=0;i<selections.GetCount();++i)
  {
    #ifdef WIN32
    this->Load(mBrowseDir+_T("\\")+mpBrowseList->GetString(selections.Item(i)));
    #else
    this->Load(mBrowseDir+_T("/")+mpBrowseList->GetString(selections.Item(i)));
    #endif
  }
}

void WXCrystMainFrame::OnMenuClose(wxCommandEvent& event)
{
  bool safe;
  wxConfigBase::Get()->Read(_T("Fox/BOOL/Ask confirmation before exiting Fox"),&safe);
  this->Close(safe);
}

void WXCrystMainFrame::Close(bool safe)
{
   if(safe)
   {
      bool saved=true;
      for(int i=0;i<gRefinableObjRegistry.GetNb();i++)
         if(gRefinableObjRegistry.GetObj(i).GetClockMaster()>mClockLastSave)
         {
            saved=false;
            break;
         }
      if(!saved)
      {
         wxString msg;
         msg.Printf( _T("Some objects have not been saved\n")
                  _T("Do you really want to close all ?"));
   
         wxMessageDialog d(this,msg, _T("Really Close ?"), wxYES | wxNO);
         if(wxID_YES!=d.ShowModal()) return;
      }
   }
   cout<<"Removing all Optimization objects..."<<endl;
   gOptimizationObjRegistry.DeleteAll();
   cout<<"Removing all DiffractionDataSingleCrystal objects..."<<endl;
   gDiffractionDataSingleCrystalRegistry.DeleteAll();
   cout<<"Removing all PowderPattern objects..."<<endl;
   gPowderPatternRegistry.DeleteAll();
   cout<<"Removing all Crystal objects..."<<endl;
   gCrystalRegistry.DeleteAll();
}


void WXCrystMainFrame::OnClose(wxCloseEvent& event)
{
   this->SafeQuit();
}
void WXCrystMainFrame::SafeQuit()
{
   if(mpGridWindow->m_WXFoxServer!=NULL)
   {
      wxMessageDialog d(this,_T("You are trying to close server. Are you sure?"), _T(""), wxYES | wxNO);
      if(wxID_YES!=d.ShowModal()) return;
   }
   if(mpGridWindow->m_WXFoxClient!=NULL){
       wxMessageDialog d(this,_T("You are trying to close client. Are you sure?"), _T(""), wxYES | wxNO);
       if(wxID_YES!=d.ShowModal()) return;
       mpGridWindow->m_WXFoxClient->CloseClient();
   }
   bool safe=true;
   wxConfigBase::Get()->Read(_T("Fox/BOOL/Ask confirmation before exiting Fox"),&safe);
   if(!safe)
   {
      this->Destroy();
      return;
   }

   bool saved=true;
   for(int i=0;i<gRefinableObjRegistry.GetNb();i++)
      if(gRefinableObjRegistry.GetObj(i).GetClockMaster()>mClockLastSave)
      {
         saved=false;
         break;
      }
   if(!saved)
   {
      wxString msg;
      msg.Printf( _T("Some objects have not been saved\n")
               _T("Do you really want to Exit ?"));

      wxMessageDialog d(this,msg, _T("Really Exit ?"), wxYES | wxNO);
      if(wxID_YES!=d.ShowModal()) return;
   }
   cout<<"Removing all Optimization objects..."<<endl;
   gOptimizationObjRegistry.DeleteAll();
   cout<<"Removing all DiffractionDataSingleCrystal objects..."<<endl;
   gDiffractionDataSingleCrystalRegistry.DeleteAll();
   cout<<"Removing all PowderPattern objects..."<<endl;
   gPowderPatternRegistry.DeleteAll();
   cout<<"Removing all Crystal objects..."<<endl;
   gCrystalRegistry.DeleteAll();
   //FoxGrid
   mpGridWindow->Clear();
   this->Destroy();
   mpGridWindow->Destroy();
}
void WXCrystMainFrame::OnSave(wxCommandEvent& WXUNUSED(event))
{
   WXCrystValidateAllUserInput();
   bool compressed;
   wxConfigBase::Get()->Read(_T("Fox/BOOL/Use compressed file format (.xml.gz)"),&compressed);
   if(compressed)
   {
      wxFileDialog open(this,_T("Choose File to save all objects:"),
                        _T(""),_T(""),_T("FOX compressed files (*.xml.gz)|*.xml.gz"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
      if(open.ShowModal() != wxID_OK) return;
      wxString name=open.GetPath();
      if(name.substr(name.size()-7,7)!=_T(".xml.gz"))
      {
         cout<<name<<" -> "<<name+_T(".gz")<<endl;
         if(name.substr(name.size()-4,4)==_T(".xml")) name=name+_T(".gz");
         else name=name+_T(".xml.gz");
      }
      stringstream sst;
      XMLCrystFileSaveGlobal(sst);
      wxFileOutputStream ostream(name.c_str());
      wxZlibOutputStream zstream(ostream,-1,wxZLIB_GZIP);
      zstream.Write(sst.str().c_str(),sst.str().size());
   }
   else
   {
      wxFileDialog open(this,_T("Choose File to save all objects:"),
                        _T(""),_T(""),_T("*.xml"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
      if(open.ShowModal() != wxID_OK) return;
      wxString name=open.GetPath();
      if(name.substr(name.size()-4,4)!=_T(".xml"))
      {
         cout<<name<<" -> "<<name+_T(".xml")<<endl;
         name=name+_T(".xml");
      }
      stringstream sst;
      XMLCrystFileSaveGlobal(sst);
      wxFileOutputStream ostream(name.c_str());
      ostream.Write(sst.str().c_str(),sst.str().size());
   }
   mClockLastSave.Click();
}
void WXCrystMainFrame::OnAddCrystal(wxCommandEvent& WXUNUSED(event))
{
   Crystal* obj;
   obj=new Crystal;
   stringstream s;s<<"Crystal #"<<gCrystalRegistry.GetNb();
   obj->SetName(s.str());
   if(!wxConfigBase::Get()->HasEntry(_T("Crystal/BOOL/Default-use Dynamical Occupancy Correction")))
      wxConfigBase::Get()->Write(_T("Crystal/BOOL/Default-use Dynamical Occupancy Correction"), true);
   else
   {
      bool doc;
      wxConfigBase::Get()->Read(_T("Crystal/BOOL/Default-use Dynamical Occupancy Correction"), &doc);
      if(doc) obj->GetOption(0).SetChoice(0);
      else obj->GetOption(0).SetChoice(1);
   }
   obj->UpdateDisplay();
   mpNotebook->SetSelection(0);
   
   // Fake pdf for linking ?
   //PDF pdf;
   //pdf.GetPDFR();
}
void WXCrystMainFrame::OnAddPowderPattern(wxCommandEvent& WXUNUSED(event))
{
   PowderPattern* obj;
   obj=new PowderPattern;
   stringstream s;s<<"Powder Pattern #"<<gPowderPatternRegistry.GetNb();
   obj->SetName(s.str());
   obj->SetMaxSinThetaOvLambda(0.4);
   obj->UpdateDisplay();
   mpNotebook->SetSelection(1);
}

void WXCrystMainFrame::OnAddSingleCrystalData(wxCommandEvent& WXUNUSED(event))
{
   WXCrystValidateAllUserInput();
   Crystal *cryst;
   if(gCrystalRegistry.GetNb()==1) cryst=&(gCrystalRegistry.GetObj(0));
   else
   {
      int choice;
      cryst=dynamic_cast<Crystal*>
         (WXDialogChooseFromRegistry(gCrystalRegistry,(wxWindow*)this,
            "Choose a Crystal Structure:",choice));
      if(0==cryst) return;
   }

   DiffractionDataSingleCrystal* obj;
   obj=new DiffractionDataSingleCrystal(*cryst);
   stringstream s;s<<"Diffraction data for "<<cryst->GetName();
   obj->SetName(s.str());
   obj->SetMaxSinThetaOvLambda(0.4);
   obj->UpdateDisplay();
   mpNotebook->SetSelection(2);
}
void WXCrystMainFrame::OnAddGlobalOptimObj(wxCommandEvent& WXUNUSED(event))
{
   stringstream s;s<<"OptimizationObj #"<<gOptimizationObjRegistry.GetNb();
   MonteCarloObj* obj=new MonteCarloObj(s.str());
   mpNotebook->SetSelection(3);
}
void WXCrystMainFrame::OnAddGeneticAlgorithm(wxCommandEvent& WXUNUSED(event))
{
   //GeneticAlgorithm* obj;
   //obj=new GeneticAlgorithm("Change Me!");
}
//FOXGrid
void WXCrystMainFrame::OnStartGridServer(wxCommandEvent &event)
{
   VFN_DEBUG_ENTRY("WXCrystMainFrame::OnStartGridServer()",10)
   wxDirDialog dlg(this, _T("Choose working directory (check write and read permissions!)"));
   if(dlg.ShowModal()!=wxID_OK) return;
   wxString dirName;
#ifdef WIN32
   dirName = dlg.GetPath() + _T("\\GridRslt");
#else
   dirName = dlg.GetPath() + _T("/GridRslt");
#endif
   if(!wxDirExists(dirName)) wxMkdir(dirName);
   mpGridWindow->m_working_dir = dlg.GetPath();
   if(mpGridWindow->StartServer()==NULL) return;
   VFN_DEBUG_EXIT("WXCrystMainFrame::OnStartGridServer()",10)
   mpGridWindow->Layout();
   mpNotebook->SetSelection(4);
}
void WXCrystMainFrame::OnStartGridClient(wxCommandEvent &event)
{
   wxDirDialog dlg(this, _T("Choose working directory (check write and read permissions!)"));
   if(dlg.ShowModal()!=wxID_OK) return;
   mpGridWindow->m_working_dir = dlg.GetPath();
   if(mpGridWindow->StartClientWindow()==NULL) return;
   mpNotebook->SetSelection(4);
}
void WXCrystMainFrame::OnSetDebugLevel(wxCommandEvent& event)
{
   if(event.GetId()== MENU_DEBUG_LEVEL0 ){VFN_DEBUG_GLOBAL_LEVEL(0);}
   if(event.GetId()== MENU_DEBUG_LEVEL1 ){VFN_DEBUG_GLOBAL_LEVEL(1);}
   if(event.GetId()== MENU_DEBUG_LEVEL2 ){VFN_DEBUG_GLOBAL_LEVEL(2);}
   if(event.GetId()== MENU_DEBUG_LEVEL3 ){VFN_DEBUG_GLOBAL_LEVEL(3);}
   if(event.GetId()== MENU_DEBUG_LEVEL4 ){VFN_DEBUG_GLOBAL_LEVEL(4);}
   if(event.GetId()== MENU_DEBUG_LEVEL5 ){VFN_DEBUG_GLOBAL_LEVEL(5);}
   if(event.GetId()== MENU_DEBUG_LEVEL6 ){VFN_DEBUG_GLOBAL_LEVEL(6);}
   if(event.GetId()== MENU_DEBUG_LEVEL7 ){VFN_DEBUG_GLOBAL_LEVEL(7);}
   if(event.GetId()== MENU_DEBUG_LEVEL8 ){VFN_DEBUG_GLOBAL_LEVEL(8);}
   if(event.GetId()== MENU_DEBUG_LEVEL9 ){VFN_DEBUG_GLOBAL_LEVEL(9);}
   if(event.GetId()== MENU_DEBUG_LEVEL10){VFN_DEBUG_GLOBAL_LEVEL(10);}
}
void WXCrystMainFrame::OnDebugTest(wxCommandEvent& event)
{
   WXCrystValidateAllUserInput();
   static long saveId=-1;
   static long saveId2=-1;
   if(event.GetId()== MENU_DEBUG_TEST1)
   {
      if(saveId==-1) saveId=gScattererRegistry.GetObj(0).CreateParamSet();
      else gScattererRegistry.GetObj(0).SaveParamSet(saveId);
      gScattererRegistry.GetObj(0).GlobalOptRandomMove(1);
      gCrystalRegistry.GetObj(0).UpdateDisplay();
      if(saveId2==-1) saveId2=gScattererRegistry.GetObj(0).CreateParamSet();
      else gScattererRegistry.GetObj(0).SaveParamSet(saveId2);
   }
   if(event.GetId()== MENU_DEBUG_TEST2)
   {
      Crystal *cryst=new Crystal(25.,30.,35.,"P1");
      ScatteringPowerAtom *ScattPowS=new ScatteringPowerAtom("S" ,"S",0.74);
      ScatteringPowerAtom *ScattPowO=new ScatteringPowerAtom("O","O",1.87);
      cryst->AddScatteringPower(ScattPowS);
      cryst->AddScatteringPower(ScattPowO);
      Molecule *mol;
      mol=MakeOctahedron(*cryst,"SO6",ScattPowS,ScattPowO,1.5);
      cryst->AddScatterer(mol);
      mol->CreateCopy();
   }
   if(event.GetId()== MENU_DEBUG_TEST3)
   {
      Crystal *cryst=new Crystal(25.,30.,35.,"P1");
      ScatteringPowerAtom *ScattPowS=new ScatteringPowerAtom("S" ,"S",0.74);
      ScatteringPowerAtom *ScattPowO=new ScatteringPowerAtom("O","O",1.87);
      cryst->AddScatteringPower(ScattPowS);
      cryst->AddScatteringPower(ScattPowO);
      Molecule *mol;
      mol=MakeTetrahedron(*cryst,"SO4",ScattPowS,ScattPowO,1.5);
      mol->RestraintStatus(cout);cryst->AddScatterer(mol);
      
      mol=MakeOctahedron(*cryst,"SO6",ScattPowS,ScattPowO,1.5);
      mol->RestraintStatus(cout);cryst->AddScatterer(mol);
      
      mol=MakeSquarePlane(*cryst,"SO6",ScattPowS,ScattPowO,1.5);
      mol->RestraintStatus(cout);cryst->AddScatterer(mol);
      
      mol=MakeCube(*cryst,"SO8",ScattPowS,ScattPowO,1.5);
      mol->RestraintStatus(cout);cryst->AddScatterer(mol);
      
      mol=MakePrismTrigonal(*cryst,"SO6",ScattPowS,ScattPowO,1.5);
      mol->RestraintStatus(cout);cryst->AddScatterer(mol);
      
      mol=MakeIcosahedron(*cryst,"SO12",ScattPowS,ScattPowO,1.5);
      mol->RestraintStatus(cout);cryst->AddScatterer(mol);
      
      mol=MakeTriangle(*cryst,"SO3",ScattPowS,ScattPowO,1.5);
      mol->RestraintStatus(cout);cryst->AddScatterer(mol);
      
      mol=MakeAntiPrismTetragonal(*cryst,"SO8",ScattPowS,ScattPowO,1.5);
      mol->RestraintStatus(cout);cryst->AddScatterer(mol);

      cryst->RandomizeConfiguration();
      WXCrystal *pWXCryst=dynamic_cast<WXCrystal*> (gCrystalRegistry.GetObj(0).WXGet());
      wxCommandEvent com;
      pWXCryst->OnMenuCrystalGL(com);
   }
}

void WXCrystMainFrame::OnUpdateUI(wxUpdateUIEvent& event)
{
   VFN_DEBUG_MESSAGE("WXCrystMainFrame::OnUpdateUI(): Uncaught event !!",10)
}
void WXCrystMainFrame::OnToggleTooltips(wxCommandEvent& event)
{
   bool tooltip_enabled;
   wxConfigBase::Get()->Read(_T("Fox/BOOL/Enable tooltips"), &tooltip_enabled);
   tooltip_enabled = !tooltip_enabled;
   wxToolTip::Enable(tooltip_enabled);
   wxConfigBase::Get()->Write(_T("Fox/BOOL/Enable tooltips"), tooltip_enabled);
   VFN_DEBUG_MESSAGE("WXCrystMainFrame::OnToggleTooltips(): Tooltips= "<<tooltip_enabled,10)
}

void GetRecursiveConfigEntryList(list<pair<wxString,wxString> > &l)
{
   wxString str;
   wxString path=wxConfigBase::Get()->GetPath();
   if(path==_T(""))path=_T("/")+path;
   long entry;
   bool bCont = wxConfigBase::Get()->GetFirstEntry(str, entry);
   while(bCont)
   {
      //cout<<__FILE__<<":"<<__LINE__<<"Entry:"<<path+"/"+str<<endl;
      l.push_back(make_pair(path,str));
      bCont = wxConfigBase::Get()->GetNextEntry(str, entry);
   }
   long group;
   bCont = wxConfigBase::Get()->GetFirstGroup(str, group);
   while(bCont)
   {
      wxConfigBase::Get()->SetPath(path+_T("/")+str);
      GetRecursiveConfigEntryList(l);
      wxConfigBase::Get()->SetPath(_T(".."));
      bCont = wxConfigBase::Get()->GetNextGroup(str, group);
   }
}

enum FOX_PREF_TYPE {PREF_BOOL,PREF_STRING,PREF_LONG} ;
struct FoxPref
{
   FoxPref(wxString &comp,FOX_PREF_TYPE &t,wxString &e,wxWindow *w):
   component(comp),type(t),entry(e),win(w)
   {}
   wxString component;
   FOX_PREF_TYPE type;
   wxString entry;
   wxWindow  *win;
};

class WXFoxPreferences:public wxDialog
{
   public:
      WXFoxPreferences(wxWindow *parent);
      ~WXFoxPreferences();
      void OnClose(wxCloseEvent& event);
   private:
      list<FoxPref> l;
   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(WXFoxPreferences, wxDialog)
   EVT_CLOSE(WXFoxPreferences::OnClose)
END_EVENT_TABLE()

WXFoxPreferences::WXFoxPreferences(wxWindow *parent):
wxDialog(parent,-1,_T("FOX Preferences: "),wxDefaultPosition,wxSize(400,300),wxDEFAULT_DIALOG_STYLE)
{
   wxScrolledWindow *sw=new wxScrolledWindow(this);
   sw->FitInside();
   sw->SetScrollRate(10,10);
   //sw=this;
   wxBoxSizer *sizer=new wxBoxSizer(wxVERTICAL);
   sw->SetSizer(sizer);
   list<pair<wxString,wxString> > ltmp;
   GetRecursiveConfigEntryList(ltmp);
   
   wxWindow *w;
   list<FoxPref> l2;
   for(list<pair<wxString,wxString> >::const_iterator pos=ltmp.begin();pos!=ltmp.end();++pos)
   {
      wxString component,entry;
      FOX_PREF_TYPE type;
      
      size_t tmp=pos->first.find(_T("/"),1);
      component=pos->first.substr(1,tmp-1);
      
      entry=pos->second;
      
      if(pos->first.find(_T("BOOL")  ,1)!=wxString::npos) type=PREF_BOOL;
      if(pos->first.find(_T("STRING"),1)!=wxString::npos) type=PREF_STRING;
      if(pos->first.find(_T("LONG")  ,1)!=wxString::npos) type=PREF_LONG;
      
      switch(type)
      {
         case PREF_BOOL:
         {
            wxCheckBox *win=new wxCheckBox(sw,-1,component+_T(":")+entry);
            bool val;
            wxConfigBase::Get()->Read(_T("/")+component+_T("/BOOL/")+entry,&val);
            win->SetValue(val);
            sizer->Add(win,0,wxLEFT);
            w=win;
            break;
         }
         case PREF_STRING:
         {
            w=new wxWindow(sw,-1);
            wxBoxSizer *s=new wxBoxSizer(wxHORIZONTAL);
            w->SetSizer(s);
            wxStaticText *txt=new wxStaticText(w,-1,component+_T(":")+entry);
            wxString val;
            wxConfigBase::Get()->Read(_T("/")+component+_T("/STRING/")+entry,&val);
            wxTextCtrl *win=new wxTextCtrl(w,-1,val);
            s->Add(txt,0,wxALIGN_CENTER);
            s->Add(win,0,wxALIGN_CENTER);
            w->Layout();
            sizer->Add(w,0,wxLEFT);
            w=win;
            break;
         }
         case PREF_LONG:
         {
            w=new wxWindow(sw,-1);
            wxBoxSizer *s=new wxBoxSizer(wxHORIZONTAL);
            w->SetSizer(s);
            wxStaticText *txt=new wxStaticText(w,-1,component+_T(":")+entry);
            wxString val;
            wxConfigBase::Get()->Read(_T("/")+component+_T("/LONG/")+entry,&val);
            wxTextCtrl *win=new wxTextCtrl(w,-1,val,wxDefaultPosition,wxDefaultSize,0,wxTextValidator(wxFILTER_NUMERIC));
            s->Add(txt,0,wxALIGN_CENTER);
            s->Add(win,0,wxALIGN_CENTER);
            w->Layout();
            sizer->Add(w,0,wxLEFT);
            w=win;
            break;
         }
      }
      l.push_back(FoxPref(component,type,entry,w));
   }
   sw->Layout();
   sizer->Fit(sw);
   this->Layout();
}
WXFoxPreferences::~WXFoxPreferences()
{
   cout<<"WXFoxPreferences::~WXFoxPreferences()"<<endl;
}

void WXFoxPreferences::OnClose(wxCloseEvent& event)
{
   cout<<"WXFoxPreferences::OnClose()"<<endl;
   for(list<FoxPref>::const_iterator pos=l.begin();pos!=l.end();++pos)
   {
      switch(pos->type)
      {
         case PREF_BOOL:
         {
            wxString full=_T("/")+pos->component+_T("/BOOL/")+pos->entry;
            bool val;
            wxConfigBase::Get()->Read(full,&val);
            wxCheckBox *w=dynamic_cast<wxCheckBox *>(pos->win);
            val=w->GetValue();
            wxConfigBase::Get()->Write(full,val);
            break;
         }
         case PREF_STRING:
         {
            wxString full=_T("/")+pos->component+_T("/STRING/")+pos->entry;
            wxString val;
            wxConfigBase::Get()->Read(full,&val);
            wxTextCtrl *w=dynamic_cast<wxTextCtrl *>(pos->win);
            val=w->GetValue();
            wxConfigBase::Get()->Write(full,val);
            break;
         }
         case PREF_LONG:
         {
            wxString full=_T("/")+pos->component+_T("/LONG/")+pos->entry;
            wxString s;
            long val;
            wxConfigBase::Get()->Read(full,&val);
            wxTextCtrl *w=dynamic_cast<wxTextCtrl *>(pos->win);
            s=w->GetValue();
            s.ToLong(&val);
            wxConfigBase::Get()->Write(full,val);
            break;
         }
      }
   }
   this->Destroy();
}


void WXCrystMainFrame::OnPreferences(wxCommandEvent& event)
{
   WXFoxPreferences *prefs= new WXFoxPreferences(this);
   prefs->ShowModal();
}

void WXCrystMainFrame::OnCheckUpdate(wxCommandEvent& event)
{
   cout<<"WXCrystMainFrame::OnCheckUpdate"<<endl;
   if(event.GetId()==ID_FOX_UPDATES_RESULT)
   {
      unsigned int nbminorfeature=0,nbmajorfeature=0,nbrelease=0,nbminorbug=0,nbmajorbug=0,nbcritical=0;
      for(map<unsigned int,pair<int,wxString> >::const_iterator pos=mvUpdates.begin();pos!=mvUpdates.end();++pos)
      {//Critical fixes
         if(pos->second.first==0) nbminorfeature++;
         if(pos->second.first==1) nbmajorfeature++;
         if(pos->second.first==2) nbrelease++;
         if(pos->second.first==10) nbminorbug++;
         if(pos->second.first==11) nbmajorbug++;// Major bug but calculations are still OK
         if(pos->second.first==12) nbcritical++;// A mistake was made, giving erroneous results
      }
      if(mvUpdatesAutoCheck)
      {// Autocheck => only critical updates
         if(nbcritical>0)
         {
            wxString msg;
            msg.Printf( _T("A new version of Fox is available, including CRITICAL bug fixes\n")
                        _T("It is strongly recommended to update to a new version\n\n Major changes: \n"));
   
            for(map<unsigned int,pair<int,wxString> >::const_iterator pos=mvUpdates.begin();pos!=mvUpdates.end();++pos)
            {
               if(pos->second.first==12)
                  msg=msg+wxString::Format(_T("#%d (CRITICAL): %s\n"),pos->first,pos->second.second.c_str());
            }
            wxMessageDialog d(this,msg, _T("! CRITICAL update available !"), wxOK|wxICON_EXCLAMATION);
            d.ShowModal();
         }
      }
      else
      {// User asked for this, so give the full version
         wxFrame *frame=new wxFrame(this,-1,_T("FOX Updates"),wxDefaultPosition,wxSize(800,250),wxSTAY_ON_TOP | wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN);
         wxTextCtrl *wUpdates=new wxTextCtrl(frame,-1,_T(""),wxDefaultPosition,wxDefaultSize,wxTE_MULTILINE|wxTE_READONLY|wxTE_DONTWRAP);
         wUpdates->SetFont(wxFont(10,wxROMAN,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD));
         if(mvUpdates.size()>0)
         {
            if(nbcritical>0)
            {
               cout<<wxString::Format(_T("\n%d CRITICAL updates available:\n"),nbcritical)<<endl;
               wUpdates->AppendText(wxString::Format(_T("WARNING: CRITICAL updates are available !\n\n Changes:\n")));
               for(map<unsigned int,pair<int,wxString> >::const_iterator pos=mvUpdates.begin();pos!=mvUpdates.end();++pos)
               {
                  if(pos->second.first==12)
                  {
                     wxString mess=wxString::Format(_T("  #%d: %s\n"),pos->first,pos->second.second.c_str());
                     wUpdates->AppendText(mess);
                  }
               }
            }
            else
            if(nbrelease>0)
            {
               cout<<wxString::Format(_T("\n%d new RELEASE available :\n"),nbrelease)<<endl;
               wUpdates->AppendText(wxString::Format(_T("A new Fox RELEASE available !\n\n Changes:\n")));
               for(map<unsigned int,pair<int,wxString> >::const_iterator pos=mvUpdates.begin();pos!=mvUpdates.end();++pos)
               {
                  if(pos->second.first==2)
                     wUpdates->AppendText(wxString::Format(_T("  #%d: %s\n"),pos->first,pos->second.second.c_str()));
               }
            }
            else
            if(nbmajorfeature>0)
            {
               cout<<wxString::Format(_T("\n%d major features updates available:\n"),nbmajorfeature)<<endl;
               wUpdates->AppendText(wxString::Format(_T("A new Fox version with new major features is available !\n\n Changes:\n")));
               for(map<unsigned int,pair<int,wxString> >::const_iterator pos=mvUpdates.begin();pos!=mvUpdates.end();++pos)
               {
                  if(pos->second.first==1)
                     wUpdates->AppendText(wxString::Format(_T("  #%d: %s\n"),pos->first,pos->second.second.c_str()));
               }
            }
            else
            if(nbminorfeature>0)
            {
               cout<<wxString::Format(_T("\n%d minor features updates available:\n"),nbminorfeature)<<endl;
               wUpdates->AppendText(wxString::Format(_T("A new Fox version with new minor features is available.\n\n Changes:\n")));
               for(map<unsigned int,pair<int,wxString> >::const_iterator pos=mvUpdates.begin();pos!=mvUpdates.end();++pos)
               {
                  if(pos->second.first==0)
                     wUpdates->AppendText(wxString::Format(_T("  #%d: %s\n"),pos->first,pos->second.second.c_str()));
               }
            }
            wUpdates->AppendText(wxString::Format(_T("\n\n => go to http://objcryst.sf.net/Fox/ to get the new version")));
            wUpdates->AppendText(wxString::Format(_T("\n\n See the full changelog at: http://objcryst.sf.net/Fox/Changelog")));
         }
         else
         {
            wUpdates->AppendText(wxString::Format(_T("No updates found !\n")));
         }
         frame->Show(true);
      }
   }
   else
   {
      mvUpdatesAutoCheck=false;
      WXThreadCheckUpdates *pThreadCheckUpdates = new WXThreadCheckUpdates(mvUpdates,*this);
      if(pThreadCheckUpdates->Create() != wxTHREAD_NO_ERROR) 
         wxLogError(_T("Can't create updates check thread"));
      else pThreadCheckUpdates->Run();
   }
}

#endif
///////////////////////////////////////// Speed Test////////////////////
void standardSpeedTest()
{
  cout << " Beginning Speed tests" << endl ;
   const float dt=2.5;
   std::list<SpeedTestReport> vReport;
   vReport.push_back(SpeedTest(20,4,"P1"  ,RAD_NEUTRON,100,0,dt));
   vReport.push_back(SpeedTest(20,4,"P-1" ,RAD_NEUTRON,100,0,dt));
   vReport.push_back(SpeedTest(20,4,"Pnma",RAD_NEUTRON,100,0,dt));
   vReport.push_back(SpeedTest(20,4,"Ia3d",RAD_NEUTRON,100,0,dt));
   vReport.push_back(SpeedTest(20,4,"P1"  ,RAD_XRAY   ,100,0,dt));
   vReport.push_back(SpeedTest(20,4,"P-1" ,RAD_XRAY   ,100,0,dt));
   vReport.push_back(SpeedTest(20,4,"Pnma",RAD_XRAY   ,100,0,dt));
   vReport.push_back(SpeedTest(20,4,"Ia3d",RAD_XRAY   ,100,0,dt));
   vReport.push_back(SpeedTest(100,4,"P21",RAD_XRAY   ,500,0,dt));
   vReport.push_back(SpeedTest(100,4,"P21/n",RAD_XRAY ,500,0,dt));

   vReport.push_back(SpeedTest(20,4,"P1"  ,RAD_NEUTRON,100,1,dt));
   vReport.push_back(SpeedTest(20,4,"P-1" ,RAD_NEUTRON,100,1,dt));
   vReport.push_back(SpeedTest(20,4,"Pnma",RAD_NEUTRON,100,1,dt));
   vReport.push_back(SpeedTest(20,4,"Ia3d",RAD_NEUTRON,100,1,dt));
   vReport.push_back(SpeedTest(20,4,"P1"  ,RAD_XRAY   ,100,1,dt));
   vReport.push_back(SpeedTest(20,4,"P-1" ,RAD_XRAY   ,100,1,dt));
   vReport.push_back(SpeedTest(20,4,"Pnma",RAD_XRAY   ,100,1,dt));
   vReport.push_back(SpeedTest(20,4,"Ia3d",RAD_XRAY   ,100,1,dt));
   vReport.push_back(SpeedTest(100,4,"P21",RAD_XRAY   ,500,1,dt));
   vReport.push_back(SpeedTest(100,4,"P21/n",RAD_XRAY ,500,1,dt));

   // Compared to results on a Core2 Quad Q6600 running @2.4 GHz, gcc 4.6.2 with an old version of Fox 1.9.0.2
   CrystVector_REAL vfnBogoMRAPS_n_201001(20);
   vfnBogoMRAPS_n_201001(0)=47;
   vfnBogoMRAPS_n_201001(1)=49;
   vfnBogoMRAPS_n_201001(2)=86;
   vfnBogoMRAPS_n_201001(3)=106;
   vfnBogoMRAPS_n_201001(4)=46;
   vfnBogoMRAPS_n_201001(5)=49;
   vfnBogoMRAPS_n_201001(6)=85;
   vfnBogoMRAPS_n_201001(7)=107;
   vfnBogoMRAPS_n_201001(8)=89;
   vfnBogoMRAPS_n_201001(9)=105;
   
   vfnBogoMRAPS_n_201001(10)=39;
   vfnBogoMRAPS_n_201001(11)=41;
   vfnBogoMRAPS_n_201001(12)=79;
   vfnBogoMRAPS_n_201001(13)=105;
   vfnBogoMRAPS_n_201001(14)=38;
   vfnBogoMRAPS_n_201001(15)=42;
   vfnBogoMRAPS_n_201001(16)=73;
   vfnBogoMRAPS_n_201001(17)=105;
   vfnBogoMRAPS_n_201001(18)=86;
   vfnBogoMRAPS_n_201001(19)=102;
   
   cout<<" Spacegroup NbAtoms NbAtType Radiation Type  NbRefl  BogoSPS    BogoMRAPS   BogoMRAPS(n)  relat%"<<endl;
   unsigned int i=0;
   REAL vfnCompar2010=0.;
   for(std::list<SpeedTestReport>::const_iterator pos=vReport.begin();
       pos != vReport.end();++pos)
   {
      cout<<"  "<<FormatString(pos->mSpacegroup,8)<<" "
          <<FormatInt(pos->mNbAtom)<<" "
          <<FormatInt(pos->mNbAtomType,6)<<"     ";
      switch(pos->mRadiation)
      {
         case(RAD_NEUTRON): cout<<FormatString("neutron",7)<<" ";break;
         case(RAD_XRAY): cout<<FormatString("X-ray",7)<<" ";break;
         case(RAD_ELECTRON): cout<<FormatString("electron",7)<<" ";break;
      }
      switch(pos->mDataType)
      {
         case(0): cout<<FormatString("Single",6)<<" ";break;
         case(1): cout<<FormatString("Powder",6)<<" ";break;
      }
      const REAL relat2010=pos->mBogoMRAPS_reduced/vfnBogoMRAPS_n_201001(i);
      cout<<FormatInt(pos->mNbReflections)<<" "
          <<FormatFloat(pos->mBogoSPS)<<" "
          <<FormatFloat(pos->mBogoMRAPS)<<" "
          <<FormatFloat(pos->mBogoMRAPS_reduced)<<" "
          <<FormatFloat(relat2010*100.,8,2)
          <<endl;
      vfnCompar2010+=relat2010;
      i++;
   }
   vfnCompar2010/=vfnBogoMRAPS_n_201001.numElements();
   cout<<endl<<"Your FOX/ObjCryst++ speed index is "<<FormatFloat(vfnCompar2010*100.,8,2)
       <<"% (100% = Core2 Q6600 (1 core@2.4GHz) with Linux/gcc 4.6.1 - Fox 1.9.0.2)"<<endl;
  cout << " End of Crystallographic Speeding !" << endl ;
}

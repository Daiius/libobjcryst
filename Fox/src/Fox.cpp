/*  ObjCryst++ Object-Oriented Crystallographic Library
    (c) 2000-2002 Vincent Favre-Nicolin vincefn@users.sourceforge.net
        2000-2001 University of Geneva (Switzerland)

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
#endif

#include <locale.h>
#include <sstream>
#include <list>

#include "ObjCryst/IO.h"
#include "ObjCryst/Crystal.h"
#include "ObjCryst/PowderPattern.h"
#include "ObjCryst/DiffractionDataSingleCrystal.h"
#include "ObjCryst/Polyhedron.h"
#include "ObjCryst/test.h"
#include "RefinableObj/GlobalOptimObj.h"
#include "Quirks/VFNStreamFormat.h"

#ifdef __WX__CRYST__
   #include "wxCryst/wxCrystal.h"

   #if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMAC__) || defined(__WXMGL__) || defined(__WXX11__)
      #include "Fox.xpm"
   #endif
#endif


using namespace ObjCryst;
using namespace std;

static const std::string foxVersion=std::string("1.6.99CVS - ")+__DATE__;

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
   private:
      wxWindow* mpChild;
      int mHeight,mWidth;
      wxBoxSizer *mpSizer;
};

// main frame
class WXCrystMainFrame : public wxFrame
{
public:
   WXCrystMainFrame(const wxString& title, const wxPoint& pos, const wxSize& size,
                    const bool splashscreen=true);
   void OnQuit(wxCommandEvent& WXUNUSED(event));
   void OnAbout(wxCommandEvent& WXUNUSED(event));
   void OnLoad(wxCommandEvent& event);
   void OnMenuClose(wxCommandEvent& event);
   void OnClose(wxCloseEvent& event);
   void SafeClose();
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
private:
    DECLARE_EVENT_TABLE()
    RefinableObjClock mClockLastSave;
};
// ----------------------------------------------------------------------------
// For messaging the user
// ----------------------------------------------------------------------------
wxFrame *pMainFrameForUserMessage;

void WXCrystInformUserStdOut(const string &str)
{
   pMainFrameForUserMessage->SetStatusText((wxString)str.c_str());
}

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------
static const long MENU_FILE_QUIT=                      WXCRYST_ID();
static const long MENU_HELP_ABOUT=                     WXCRYST_ID();
static const long MENU_HELP_TOGGLETOOLTIP=             WXCRYST_ID();
static const long MENU_PREFS_PREFERENCES=              WXCRYST_ID();
static const long MENU_FILE_LOAD=                      WXCRYST_ID();
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

// ----------------------------------------------------------------------------
// event tables and other macros for wxWindows
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(WXCrystMainFrame, wxFrame)
   EVT_MENU(MENU_FILE_QUIT,  WXCrystMainFrame::OnQuit)
   EVT_MENU(MENU_HELP_ABOUT, WXCrystMainFrame::OnAbout)
   EVT_MENU(MENU_HELP_TOGGLETOOLTIP, WXCrystMainFrame::OnToggleTooltips)
   EVT_MENU(MENU_PREFS_PREFERENCES, WXCrystMainFrame::OnPreferences)
   EVT_MENU(MENU_FILE_LOAD, WXCrystMainFrame::OnLoad)
   EVT_MENU(MENU_FILE_CLOSE, WXCrystMainFrame::OnMenuClose)
   EVT_CLOSE(                WXCrystMainFrame::OnClose)
   EVT_MENU(MENU_FILE_SAVE, WXCrystMainFrame::OnSave)
   EVT_MENU(MENU_OBJECT_CREATE_CRYSTAL, WXCrystMainFrame::OnAddCrystal)
   EVT_MENU(MENU_OBJECT_CREATE_POWDERSPECTRUM, WXCrystMainFrame::OnAddPowderPattern)
   EVT_MENU(MENU_OBJECT_CREATE_SINGLECRYSTALDATA, WXCrystMainFrame::OnAddSingleCrystalData)
   EVT_MENU(MENU_OBJECT_CREATE_GLOBALOPTOBJ, WXCrystMainFrame::OnAddGlobalOptimObj)
   EVT_MENU(MENU_OBJECT_CREATE_GENETICALGORITHM, WXCrystMainFrame::OnAddGeneticAlgorithm)
   EVT_MENU(MENU_DEBUG_LEVEL0, WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL1, WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL2, WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL3, WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL4, WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL5, WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL6, WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL7, WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL8, WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL9, WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_LEVEL10,WXCrystMainFrame::OnSetDebugLevel)
   EVT_MENU(MENU_DEBUG_TEST1,  WXCrystMainFrame::OnDebugTest)
   EVT_MENU(MENU_DEBUG_TEST2,  WXCrystMainFrame::OnDebugTest)
   EVT_MENU(MENU_DEBUG_TEST3,  WXCrystMainFrame::OnDebugTest)
   EVT_UPDATE_UI(ID_CRYST_UPDATEUI, WXCrystMainFrame::OnUpdateUI)
END_EVENT_TABLE()

IMPLEMENT_APP(MyApp)

// ============================================================================
// implementation
// ============================================================================

// 'Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
#else
int main (int argc, char *argv[])
#endif
{
   TAU_PROFILE_SET_NODE(0); // sequential code 
   TAU_PROFILE("main()","int()",TAU_DEFAULT);
   //set locale settings to standard
   setlocale(LC_NUMERIC,"C");
   
   bool useGUI(true);
   long nbTrial(1000000);
   long nbRun(1);
   REAL finalCost=0.;
   bool silent=false;
   string outfilename("Fox-out.xml");
   long filenameInsertCost=-1;
   bool randomize(false);
   bool only3D(false);
   bool loadFourierGRD(false);
   list<string> vFourierFilenameGRD;
   bool loadFourierDSN6(false);
   list<string> vFourierFilenameDSN6;
   for(int i=1;i<argc;i++)
   {
      if(string("--nogui")==string(argv[i]))
      {
         useGUI=false;
         cout << "Running Fox without GUI"<<endl;
         continue;  
      }
      if(string("--randomize")==string(argv[i]))
      {
         randomize=true;
         cout << "Randomizing parameters before running"<<endl;
         continue;  
      }
      if(string("--silent")==string(argv[i]))
      {
         silent=true;
         cout << "Running Fox quietly"<<endl;
         continue;  
      }
      if(string("--finalcost")==string(argv[i]))
      {
         ++i;
         stringstream sstr(argv[i]);
         sstr >> finalCost;
         cout << "Fox will stop after reaching cost:"<<finalCost<<endl;
         continue;  
      }
      if(string("-n")==string(argv[i]))
      {
         ++i;
         stringstream sstr(argv[i]);
         sstr >> nbTrial;
         cout << "Fox will run for "<<nbTrial<<" trials"<<endl;
         continue;
      }
      if(string("--nbrun")==string(argv[i]))
      {
         ++i;
         stringstream sstr(argv[i]);
         sstr >> nbRun;
         cout << "Fox will do "<<nbRun<<" runs, randomizing before each run"<<endl;
         continue;
      }
      if(string("-i")==string(argv[i]))
      {
         ++i;
         XMLCrystFileLoadAllObject(argv[i]);
         continue;
      }
      if(string("-o")==string(argv[i]))
      {
         ++i;
         outfilename=string(argv[i]);
         filenameInsertCost = outfilename.find("#cost",0);
         cout <<"Fox:#cost, pos="<<filenameInsertCost<<","<<string::npos<<endl;
         if((long)(string::npos)==filenameInsertCost) filenameInsertCost=-1;
         continue;
      }
      if(string("--loadfouriergrd")==string(argv[i]))
      {
         ++i;
         loadFourierGRD=true;
         vFourierFilenameGRD.push_back(string(argv[i]));
         continue;
      }
      if(string("--loadfourierdsn6")==string(argv[i]))
      {
         ++i;
         loadFourierDSN6=true;
         vFourierFilenameDSN6.push_back(string(argv[i]));
         continue;
      }
      if(string("--only3d")==string(argv[i]))
      {
         only3D=true;
         continue;
      }
      if(string("--speedtest")==string(argv[i]))
      {
         standardSpeedTest();
         exit(0);
      }
      #ifdef __DEBUG__
      if(string("--debuglevel")==string(argv[i]))
      {
         int level;
         ++i;
         stringstream sstr(argv[i]);
         sstr >> level;
         VFN_DEBUG_GLOBAL_LEVEL(level);
         continue;
      }
      #endif
      if(string(argv[i]).find(string(".xml"))!=string::npos)
      {
         cout<<"Loading: "<<string(argv[i])<<endl;
         XMLCrystFileLoadAllObject(argv[i]);
         continue;
      }
      cout <<"command-line arguments:"<<endl
           <<"   in.xml: input 'in.xml' file"<<endl
           <<"   --loadfouriergrd map.grd: load and display 'map.grd' fourier map with (first) crystal structure"<<endl
           <<"   --loadfourierdsn6 map.DN6: load and display a DSN6 fourier map with (first) crystal structure"<<endl
           <<"   --nogui: run without GUI, automatically launches optimization"<<endl
           <<"      options with --nogui:"<<endl
           <<"         -n 10000     : run for 10000 trials at most (default: 1000000)"<<endl
           <<"         --nbrun 5     : do 5 runs, randomizing before each run (default: 1), use -1 to run indefinitely"<<endl
           <<"         -o out.xml   : output in 'out.xml'"<<endl
           <<"         --randomize  : randomize initial configuration"<<endl
           <<"         --silent     : (almost) no text output"<<endl
           <<"         --finalcost 0.15 : run optimization until cost < 0.15"<<endl
           <<endl<<endl<<"           EXAMPLES :"<<endl<<endl
           <<"Load file 'silicon.xml' and launch GUI:"<<endl<<endl
           <<"    Fox silicon.xml"<<endl<<endl
           
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
   
   
   if(randomize)
      for(int i=0;i<gOptimizationObjRegistry.GetNb();i++)
         gOptimizationObjRegistry.GetObj(i).RandomizeStartingConfig();
   
#ifndef __WX__CRYST__
   useGUI=false;
#endif
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
      #ifdef __WX__CRYST__
      this->OnExit();
      exit(0);
      #endif
   }
#ifdef __WX__CRYST__
   this->SetVendorName(_T("http://objcryst.sf.net/Fox"));
   this->SetAppName(_T("FOX-Free Objects for Crystallography"));
   // Read (and automatically create if necessary) global Fox preferences
   // We explicitely use a wxFileConfig, to avoid the registry under Windows
   wxConfigBase::Set(new wxFileConfig("FOX-Free Objects for Crystallography"));

   if(wxConfigBase::Get()->HasEntry("Fox/BOOL/Enable tooltips"))
   {
       bool tooltip_enabled;
       wxConfigBase::Get()->Read("Fox/BOOL/Enable tooltips", &tooltip_enabled);
       wxToolTip::Enable(tooltip_enabled);
   }
   else wxConfigBase::Get()->Write("Fox/BOOL/Enable tooltips", true);
   
   if(!wxConfigBase::Get()->HasEntry("Fox/BOOL/Ask confirmation before exiting Fox"))
      wxConfigBase::Get()->Write("Fox/BOOL/Ask confirmation before exiting Fox", true);
   
   if(!wxConfigBase::Get()->HasEntry("Fox/BOOL/Use compressed file format (.xml.gz)"))
      wxConfigBase::Get()->Write("Fox/BOOL/Use compressed file format (.xml.gz)", true);

   WXCrystMainFrame *frame ;
   string title(string("FOX: Free Objects for Xtal structures v")+foxVersion);
   frame = new WXCrystMainFrame(title.c_str(),
                                 wxPoint(50, 50), wxSize(550, 400),
                                 !(loadFourierGRD||loadFourierDSN6));
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
         UnitCellMapImport *pMap=new UnitCellMapImport(pWXCryst->GetCrystal());
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
         UnitCellMapImport *pMap=new UnitCellMapImport(pWXCryst->GetCrystal());
         cout<<"Reading Fourier file:"<<*pos<<endl;
         if (pMap->ImportDSN6(*pos) == 0)
         {
            cout<<"Error reading Fourier file:"<< *pos<<endl;
            return FALSE;
         }
         pWXCryst->GetCrystalGL()->AddFourier(pMap);
      }
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

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

WXCrystMainFrame::WXCrystMainFrame(const wxString& title, const wxPoint& pos, const wxSize& size,
                                   const bool splashscreen)
       : wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
#ifdef __WXMAC__
   // we need this in order to allow the about menu relocation, since ABOUT is
   // not the default id of the about menu
   wxApp::s_macAboutMenuItemId = MENU_HELP_ABOUT;
#endif

   // create a menu bar
      wxMenu *menuFile = new wxMenu;//
         menuFile->Append(MENU_FILE_LOAD, "&Open\tCtrl-O", "Open .xml file");
         menuFile->Append(MENU_FILE_CLOSE, "Close\tCtrl-W", "Close all");
         menuFile->Append(MENU_FILE_SAVE, "&Save\tCtrl-S", "Save Everything...");
         menuFile->Append(MENU_FILE_QUIT, "E&xit\tCtrl-Q", "Quit ");
      
      wxMenu *objectMenu = new wxMenu("", wxMENU_TEAROFF);
         objectMenu->Append(MENU_OBJECT_CREATE_CRYSTAL, "New Crystal",
                           "Add a new Crystal structure");
         objectMenu->Append(MENU_OBJECT_CREATE_POWDERSPECTRUM, "New PowderPattern",
                           "Add a new PowderPattern Object");
         objectMenu->Append(MENU_OBJECT_CREATE_SINGLECRYSTALDATA, "New Single Crystal Diffraction",
                           "Add a new Single Crystal Diffraction Object");
         objectMenu->Append(MENU_OBJECT_CREATE_GLOBALOPTOBJ, "New Monte-Carlo Object",
                           "Add a new Monte-Carlo Object");
      
      wxMenu *prefsMenu = new wxMenu;
         prefsMenu->Append(MENU_PREFS_PREFERENCES, "&Preferences...", "Fox Preferences...");
      
      wxMenu *helpMenu = new wxMenu;
         helpMenu->Append(MENU_HELP_ABOUT, "&About...", "About ObjCryst...");
         helpMenu->Append(MENU_HELP_TOGGLETOOLTIP, "Toggle Tooltip", "Set Tooltips on/off");

      wxMenuBar *menuBar = new wxMenuBar();
         menuBar->Append(menuFile,  "&File");
         menuBar->Append(objectMenu,"&Objects");
         menuBar->Append(prefsMenu, "&Preferences");
         menuBar->Append(helpMenu,  "&Help");
         #ifdef __DEBUG__
         wxMenu *debugMenu = new wxMenu;
            debugMenu->Append(MENU_DEBUG_TEST1, "Test #1");
            debugMenu->Append(MENU_DEBUG_TEST2, "Test #2");
            debugMenu->Append(MENU_DEBUG_TEST3, "Test #3");
            debugMenu->Append(MENU_DEBUG_LEVEL0, "Debug level 0 (lots of messages)");
            debugMenu->Append(MENU_DEBUG_LEVEL1, "Debug level 1");
            debugMenu->Append(MENU_DEBUG_LEVEL2, "Debug level 2");
            debugMenu->Append(MENU_DEBUG_LEVEL3, "Debug level 3");
            debugMenu->Append(MENU_DEBUG_LEVEL4, "Debug level 4");
            debugMenu->Append(MENU_DEBUG_LEVEL5, "Debug level 5");
            debugMenu->Append(MENU_DEBUG_LEVEL6, "Debug level 6");
            debugMenu->Append(MENU_DEBUG_LEVEL7, "Debug level 7");
            debugMenu->Append(MENU_DEBUG_LEVEL8, "Debug level 8");
            debugMenu->Append(MENU_DEBUG_LEVEL9, "Debug level 9");
            debugMenu->Append(MENU_DEBUG_LEVEL10,"Debug level 10 (few messages)");
         menuBar->Append(debugMenu,  "&Debug");
         #endif

   SetMenuBar(menuBar);

#if wxUSE_STATUSBAR
   CreateStatusBar(1);
   SetStatusText("Welcome to FOX/ObjCryst++!");
#endif // wxUSE_STATUSBAR

   // Create the notebook

      wxNotebook *notebook = new wxNotebook(this, -1);

      wxLayoutConstraints* c = new wxLayoutConstraints;
      c->left.SameAs(this, wxLeft, 2);
      c->right.SameAs(this, wxRight, 2);
      c->top.SameAs(this, wxTop, 2);
      c->bottom.SameAs(this, wxBottom, 2);

      notebook->SetConstraints(c);

   // First window -Crystals
      WXCrystScrolledWindow *mpWin1 = new WXCrystScrolledWindow(notebook);
      mpWin1->SetChild(gCrystalRegistry.WXCreate(mpWin1));
      mpWin1->Layout();
      notebook->AddPage(mpWin1, "Crystals", TRUE);

   // Second window - PowderPattern
      WXCrystScrolledWindow *mpWin2 = new WXCrystScrolledWindow(notebook);
      mpWin2->SetChild(gPowderPatternRegistry.WXCreate(mpWin2));
      mpWin2->Layout();
      notebook->AddPage(mpWin2,"Powder Diffraction",true);
      
   // Third window - SingleCrystal
      WXCrystScrolledWindow *mpWin3 = new WXCrystScrolledWindow(notebook);
      mpWin3->SetChild(gDiffractionDataSingleCrystalRegistry.WXCreate(mpWin3));
      mpWin3->Layout();
      notebook->AddPage(mpWin3,"Single Crystal Diffraction",true);
      
   // Fourth window - Global Optimization
      WXCrystScrolledWindow *mpWin4 = new WXCrystScrolledWindow(notebook);
      mpWin4->SetChild(gOptimizationObjRegistry.WXCreate(mpWin4));
      mpWin4->Layout();
      notebook->AddPage(mpWin4,"Global Optimization",true);

   this->SetIcon(wxICON(Fox));
   this->Show(TRUE);
   this->Layout();
   notebook->SetSelection(0);
   //Splash Screen
   if(true==splashscreen)
   {
      wxCommandEvent event;
      this->OnAbout(event);
   }
   // Set tooltip delay
   wxToolTip::SetDelay(500);
   // Reset "last save" clock, in the case we loaded an xml file on startup
   mClockLastSave.Click();
}

void WXCrystMainFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
   this->SafeClose();
}

void WXCrystMainFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
   string msg(string("F.O.X. - Free Objects for Xtallography\n")
              +"Version "+ foxVersion +" \n\n"
              +"(c) 2000-2006 Vincent FAVRE-NICOLIN, vincefn@users.sourceforge.net\n"
              +"    2000-2001 Radovan CERNY, University of Geneva\n\n"
              +"http://objcryst.sourceforge.net\n\n"
              +"FOX comes with ABSOLUTELY NO WARRANTY. It is free software, and you are\n"
              +"welcome to redistribute it under certain conditions. \n"
              +"See the LICENSE file for details.");

   wxMessageBox(msg.c_str(), "About Fox", wxOK | wxICON_INFORMATION, this);
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
      open= new wxFileDialog(this,"Choose File :",
                             "","","FOX files (*.xml,*.xml.gz)|*.xml;*.xml.gz",wxOPEN | wxFILE_MUST_EXIST);
      if(open->ShowModal() != wxID_OK) return;
      wxString name=open->GetPath();
      cout<<name.Mid(name.size()-4)<<endl;
      if(name.Mid(name.size()-4)==wxString(".xml"))
         XMLCrystFileLoadAllObject(name.c_str());
      else
      {//compressed file
         wxFileInputStream is(name.c_str());
         wxZlibInputStream zstream(is);
         stringstream sst;
         while (!zstream.Eof()) sst<<zstream.GetC();
         XMLCrystFileLoadAllObject(sst);
      }
   }
   open->Destroy();
   if(saved) mClockLastSave.Click();
}
void WXCrystMainFrame::OnMenuClose(wxCommandEvent& event)
{
   this->SafeClose();
}
void WXCrystMainFrame::OnClose(wxCloseEvent& event)
{
   this->SafeClose();
}
void WXCrystMainFrame::SafeClose()
{
   bool safe=true;
   wxConfigBase::Get()->Read("Fox/BOOL/Ask confirmation before exiting Fox",&safe);
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

      wxMessageDialog d(this,msg, "Really Exit ?", wxYES | wxNO);
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
   this->Destroy();
}
void WXCrystMainFrame::OnSave(wxCommandEvent& WXUNUSED(event))
{
   WXCrystValidateAllUserInput();
   bool compressed;
   wxConfigBase::Get()->Read("Fox/BOOL/Use compressed file format (.xml.gz)",&compressed);
   if(compressed)
   {
      wxFileDialog open(this,"Choose File to save all objects:",
                        "","","FOX compressed files (*.xml.gz)|*.xml.gz", wxSAVE | wxOVERWRITE_PROMPT);
      if(open.ShowModal() != wxID_OK) return;
      stringstream sst;
      XMLCrystFileSaveGlobal(sst);
      wxFileOutputStream ostream(open.GetPath().c_str());
      wxZlibOutputStream zstream(ostream,-1,wxZLIB_GZIP);
      zstream.Write(sst.str().c_str(),sst.str().size());
   }
   else
   {
      wxFileDialog open(this,"Choose File to save all objects:",
                        "","","*.xml", wxSAVE | wxOVERWRITE_PROMPT);
      if(open.ShowModal() != wxID_OK) return;
      XMLCrystFileSaveGlobal(open.GetPath().c_str());
   }
   mClockLastSave.Click();
}
void WXCrystMainFrame::OnAddCrystal(wxCommandEvent& WXUNUSED(event))
{
   Crystal* obj;
   obj=new Crystal;
   stringstream s;s<<"Crystal #"<<gCrystalRegistry.GetNb();
   obj->SetName(s.str());
   if(!wxConfigBase::Get()->HasEntry("Crystal/BOOL/Default-use Dynamical Occupancy Correction"))
      wxConfigBase::Get()->Write("Crystal/BOOL/Default-use Dynamical Occupancy Correction", true);
   else
   {
      bool doc;
      wxConfigBase::Get()->Read("Crystal/BOOL/Default-use Dynamical Occupancy Correction", &doc);
      if(doc) obj->GetOption(0).SetChoice(0);
      else obj->GetOption(0).SetChoice(1);
   }
   obj->UpdateDisplay();
}
void WXCrystMainFrame::OnAddPowderPattern(wxCommandEvent& WXUNUSED(event))
{
   PowderPattern* obj;
   obj=new PowderPattern;
   stringstream s;s<<"Powder Pattern #"<<gPowderPatternRegistry.GetNb();
   obj->SetName(s.str());
   obj->SetMaxSinThetaOvLambda(0.4);
   obj->UpdateDisplay();
}

void WXCrystMainFrame::OnAddSingleCrystalData(wxCommandEvent& WXUNUSED(event))
{
   WXCrystValidateAllUserInput();
   int choice;
   Crystal *cryst=dynamic_cast<Crystal*>
      (WXDialogChooseFromRegistry(gCrystalRegistry,(wxWindow*)this,
         "Choose a Crystal Structure:",choice));
   if(0==cryst) return;

   DiffractionDataSingleCrystal* obj;
   obj=new DiffractionDataSingleCrystal(*cryst);
   stringstream s;s<<"Diffraction data for "<<cryst->GetName();
   obj->SetName(s.str());
   obj->SetMaxSinThetaOvLambda(0.4);
   obj->UpdateDisplay();
}
void WXCrystMainFrame::OnAddGlobalOptimObj(wxCommandEvent& WXUNUSED(event))
{
   stringstream s;s<<"OptimizationObj #"<<gOptimizationObjRegistry.GetNb();
   MonteCarloObj* obj=new MonteCarloObj(s.str());
}
void WXCrystMainFrame::OnAddGeneticAlgorithm(wxCommandEvent& WXUNUSED(event))
{
   //GeneticAlgorithm* obj;
   //obj=new GeneticAlgorithm("Change Me!");
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
   wxConfigBase::Get()->Read("Fox/BOOL/Enable tooltips", &tooltip_enabled);
   tooltip_enabled = !tooltip_enabled;
   wxToolTip::Enable(tooltip_enabled);
   wxConfigBase::Get()->Write("Fox/BOOL/Enable tooltips", tooltip_enabled);
   VFN_DEBUG_MESSAGE("WXCrystMainFrame::OnToggleTooltips(): Tooltips= "<<tooltip_enabled,10)
}

void GetRecursiveConfigEntryList(list<pair<wxString,wxString> > &l)
{
   wxString str;
   wxString path=wxConfigBase::Get()->GetPath();
   if(path=="")path="/"+path;
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
      wxConfigBase::Get()->SetPath(path+"/"+str);
      GetRecursiveConfigEntryList(l);
      wxConfigBase::Get()->SetPath("..");
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
wxDialog(parent,-1,"FOX Preferences: ",wxDefaultPosition,wxSize(400,300),wxCLOSE_BOX|wxRESIZE_BORDER|wxOK)
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
      
      size_t tmp=pos->first.find("/",1);
      component=pos->first.substr(1,tmp-1);
      
      entry=pos->second;
      
      if(pos->first.find("BOOL"  ,1)!=wxString::npos) type=PREF_BOOL;
      if(pos->first.find("STRING",1)!=wxString::npos) type=PREF_STRING;
      if(pos->first.find("LONG"  ,1)!=wxString::npos) type=PREF_LONG;
      
      switch(type)
      {
         case PREF_BOOL:
         {
            wxCheckBox *win=new wxCheckBox(sw,-1,component+":"+entry);
            bool val;
            wxConfigBase::Get()->Read("/"+component+"/BOOL/"+entry,&val);
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
            wxStaticText *txt=new wxStaticText(w,-1,component+":"+entry);
            wxString val;
            wxConfigBase::Get()->Read("/"+component+"/STRING/"+entry,&val);
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
            wxStaticText *txt=new wxStaticText(w,-1,component+":"+entry);
            wxString val;
            wxConfigBase::Get()->Read("/"+component+"/LONG/"+entry,&val);
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
            wxString full="/"+pos->component+"/BOOL/"+pos->entry;
            bool val;
            wxConfigBase::Get()->Read(full,&val);
            wxCheckBox *w=dynamic_cast<wxCheckBox *>(pos->win);
            val=w->GetValue();
            wxConfigBase::Get()->Write(full,val);
            break;
         }
         case PREF_STRING:
         {
            wxString full="/"+pos->component+"/STRING/"+pos->entry;
            wxString val;
            wxConfigBase::Get()->Read(full,&val);
            wxTextCtrl *w=dynamic_cast<wxTextCtrl *>(pos->win);
            val=w->GetValue();
            wxConfigBase::Get()->Write(full,val);
            break;
         }
         case PREF_LONG:
         {
            wxString full="/"+pos->component+"/LONG/"+pos->entry;
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
#endif
///////////////////////////////////////// Speed Test////////////////////
void standardSpeedTest()
{
	cout << " Beginning Speed tests" << endl ;
   
   std::list<SpeedTestReport> vReport;
   vReport.push_back(SpeedTest(20,4,"P1"  ,RAD_NEUTRON,100,0,5.));
   vReport.push_back(SpeedTest(20,4,"P-1" ,RAD_NEUTRON,100,0,5.));
   vReport.push_back(SpeedTest(20,4,"Pnma",RAD_NEUTRON,100,0,5.));
   vReport.push_back(SpeedTest(20,4,"Ia3d",RAD_NEUTRON,100,0,5.));
   vReport.push_back(SpeedTest(20,4,"P1"  ,RAD_XRAY   ,100,0,5.));
   vReport.push_back(SpeedTest(20,4,"P-1" ,RAD_XRAY   ,100,0,5.));
   vReport.push_back(SpeedTest(20,4,"Pnma",RAD_XRAY   ,100,0,5.));
   vReport.push_back(SpeedTest(20,4,"Ia3d",RAD_XRAY   ,100,0,5.));

   vReport.push_back(SpeedTest(20,4,"P1"  ,RAD_NEUTRON,100,1,5.));
   vReport.push_back(SpeedTest(20,4,"P-1" ,RAD_NEUTRON,100,1,5.));
   vReport.push_back(SpeedTest(20,4,"Pnma",RAD_NEUTRON,100,1,5.));
   vReport.push_back(SpeedTest(20,4,"Ia3d",RAD_NEUTRON,100,1,5.));
   vReport.push_back(SpeedTest(20,4,"P1"  ,RAD_XRAY   ,100,1,5.));
   vReport.push_back(SpeedTest(20,4,"P-1" ,RAD_XRAY   ,100,1,5.));
   vReport.push_back(SpeedTest(20,4,"Pnma",RAD_XRAY   ,100,1,5.));
   vReport.push_back(SpeedTest(20,4,"Ia3d",RAD_XRAY   ,100,1,5.));

   // Results from november 2003 on Vincent's Athlon TB 1.4 GHz,
   //with gcc 3.3.1 with -O3 -ffast-math -march=athlon -funroll-all-loops
   
   
   //Spacegroup NbAtoms NbAtType Radiation Type  NbRefl  BogoSPS     BogoMRAPS   BogoMRAPS(n)
   //P1          10       2      neutron Single   100  30359.2832     30.3593     30.3593
   //P-1         10       2      neutron Single   100  32215.5703     64.4311     32.2156
   //Pnma        10       4      neutron Single   100  11115.5381     88.9243     44.4622
   //Ia3d        10       4      neutron Single   100   2775.5906    266.4567     66.6142
   //P1          10       2      X-ray   Single   100  29940.1211     29.9401     29.9401
   //P-1         10       2      X-ray   Single   100  31437.1270     62.8743     31.4371
   //Pnma        10       4      X-ray   Single   100  10914.5127     87.3161     43.6581
   //Ia3d        10       4      X-ray   Single   100   2797.6191    268.5714     67.1429
   //P1          10       2      neutron Powder   100  12155.6895     12.1557     12.1557
   //P-1         10       2      neutron Powder   100  12584.4922     25.1690     12.5845
   //Pnma        10       4      neutron Powder   100   7157.0571     57.2565     28.6282
   //Ia3d        10       4      neutron Powder   100   2470.5884    237.1765     59.2941
   //P1          10       2      X-ray   Powder   100  11749.5029     11.7495     11.7495
   //P-1         10       2      X-ray   Powder   100  12250.9961     24.5020     12.2510
   //Pnma        10       4      X-ray   Powder   100   7097.4150     56.7793     28.3897
   //Ia3d        10       4      X-ray   Powder   100   2485.2070    238.5799     59.6450
   CrystVector_REAL vfnBogoMRAPS_n_200311(16);
   vfnBogoMRAPS_n_200311(0)=30.3593;
   vfnBogoMRAPS_n_200311(1)=32.2156;
   vfnBogoMRAPS_n_200311(2)=44.4622;
   vfnBogoMRAPS_n_200311(3)=66.6142;
   vfnBogoMRAPS_n_200311(4)=29.9401;
   vfnBogoMRAPS_n_200311(5)=31.4371;
   vfnBogoMRAPS_n_200311(6)=43.6581;
   vfnBogoMRAPS_n_200311(7)=67.1429;
   vfnBogoMRAPS_n_200311(8)=12.1557;
   vfnBogoMRAPS_n_200311(9)=12.5845;
   vfnBogoMRAPS_n_200311(10)=28.6282;
   vfnBogoMRAPS_n_200311(11)=59.2941;
   vfnBogoMRAPS_n_200311(12)=11.7495;
   vfnBogoMRAPS_n_200311(13)=12.2510;
   vfnBogoMRAPS_n_200311(14)=28.3897;
   vfnBogoMRAPS_n_200311(15)=59.6450;
   
   cout<<" Spacegroup NbAtoms NbAtType Radiation Type  NbRefl  BogoSPS     BogoMRAPS   BogoMRAPS(n)  relat"<<endl;
   unsigned int i=0;
   REAL vfnCompar=0.;
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
      const REAL relat=pos->mBogoMRAPS_reduced/vfnBogoMRAPS_n_200311(i++);
      cout<<FormatInt(pos->mNbReflections)<<" "
          <<FormatFloat(pos->mBogoSPS)<<" "
          <<FormatFloat(pos->mBogoMRAPS)<<" "
          <<FormatFloat(pos->mBogoMRAPS_reduced)<<" "
          <<FormatFloat(relat*100.,8,2)
          <<endl;
      vfnCompar+=relat;
   }
   vfnCompar/=vfnBogoMRAPS_n_200311.numElements();
   cout<<endl<<"Your FOX/ObjCryst++ speed index is "<<FormatFloat(vfnCompar*100.,8,2)
       <<"% (100% = Athlon 1.4GHz with Linux/gcc 3.3.1, november 2003)"<<endl;
	cout << " End of Crystallographic Speeding !" << endl ;
}


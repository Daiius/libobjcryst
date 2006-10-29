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
//#include <sstream> //for stringstream
#include <fstream>

// wx headers, with or without precompilation
#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "wxCryst/wxDiffractionSingleCrystal.h"
#include "wxCryst/wxRadiation.h"

//Fixes for Cygwin; where do those stupid macros come from ? Somewhere in wxMSW headers
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#ifdef DrawText
#undef DrawText
#endif
 
namespace ObjCryst
{
////////////////////////////////////////////////////////////////////////
//
//    WXDiffractionSingleCrystal
//
////////////////////////////////////////////////////////////////////////
static long ID_DIFFSINGLECRYST_MENU_SAVEHKLIOBSICALC=      WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_SAVEHKLFCALC=          WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_SIMULATE=              WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_IMPORT_HKLIOBS=        WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_IMPORT_HKLIOBSSIGMA=   WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_IMPORT_JANAM91=        WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_IMPORT_HKLIOBSGROUP=   WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_FITSCALE_R=            WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_FITSCALE_RW=           WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH=            WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH_XRAY=       WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH_NEUTRON=    WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH_ELECTRON=   WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET=        WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_AG=     WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_MO=     WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CU=     WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_FE=     WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CO=     WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CR=     WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_AGA1=   WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_MOA1=   WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CUA1=   WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_FEA1=   WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_COA1=   WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CRA1=   WXCRYST_ID(); 
static long ID_DIFFSINGLECRYST_CRYSTAL=                    WXCRYST_ID(); 
  
BEGIN_EVENT_TABLE(WXDiffractionSingleCrystal, wxWindow)
   EVT_BUTTON(ID_WXOBJ_COLLAPSE,                        WXCrystObj::OnToggleCollapse)
   EVT_MENU(ID_REFOBJ_MENU_OBJ_SAVE,                    WXRefinableObj::OnMenuSave)
   EVT_MENU(ID_REFOBJ_MENU_OBJ_LOAD,                    WXRefinableObj::OnMenuLoad)
   EVT_MENU(ID_REFOBJ_MENU_PAR_FIXALL,                  WXRefinableObj::OnMenuFixAllPar)
   EVT_MENU(ID_REFOBJ_MENU_PAR_UNFIXALL,                WXRefinableObj::OnMenuUnFixAllPar)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_SAVEHKLIOBSICALC,   WXDiffractionSingleCrystal::OnMenuSaveHKLIobsIcalc)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_SAVEHKLFCALC,       WXDiffractionSingleCrystal::OnMenuSaveHKLFcalc)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_SIMULATE,           WXDiffractionSingleCrystal::OnMenuSimulate)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_IMPORT_HKLIOBS,     WXDiffractionSingleCrystal::OnMenuImport)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_IMPORT_HKLIOBSSIGMA,WXDiffractionSingleCrystal::OnMenuImport)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_IMPORT_JANAM91,     WXDiffractionSingleCrystal::OnMenuImport)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_IMPORT_HKLIOBSGROUP,WXDiffractionSingleCrystal::OnMenuImport)
   EVT_BUTTON(ID_DIFFSINGLECRYST_CRYSTAL,               WXDiffractionSingleCrystal::OnChangeCrystal)
   //EVT_MENU(ID_DIFFSINGLECRYST_MENU_FITSCALE_R,         WXDiffractionSingleCrystal::OnMenuFitScaleForR)
   //EVT_MENU(ID_DIFFSINGLECRYST_MENU_FITSCALE_RW,        WXDiffractionSingleCrystal::OnMenuFitScaleForRw)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET,     WXDiffractionSingleCrystal::OnMenuSetWavelength)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_WAVELENGTH_XRAY,    WXDiffractionSingleCrystal::OnMenuSetWavelength)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_WAVELENGTH_NEUTRON, WXDiffractionSingleCrystal::OnMenuSetWavelength)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_WAVELENGTH_ELECTRON,WXDiffractionSingleCrystal::OnMenuSetWavelength)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_AG,  WXDiffractionSingleCrystal::OnMenuSetWavelength)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_MO,  WXDiffractionSingleCrystal::OnMenuSetWavelength)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CU,  WXDiffractionSingleCrystal::OnMenuSetWavelength)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_FE,  WXDiffractionSingleCrystal::OnMenuSetWavelength)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CO,  WXDiffractionSingleCrystal::OnMenuSetWavelength)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CR,  WXDiffractionSingleCrystal::OnMenuSetWavelength)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_AGA1,WXDiffractionSingleCrystal::OnMenuSetWavelength)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_MOA1,WXDiffractionSingleCrystal::OnMenuSetWavelength)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CUA1,WXDiffractionSingleCrystal::OnMenuSetWavelength)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_FEA1,WXDiffractionSingleCrystal::OnMenuSetWavelength)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_COA1,WXDiffractionSingleCrystal::OnMenuSetWavelength)
   EVT_MENU(ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CRA1,WXDiffractionSingleCrystal::OnMenuSetWavelength)
   EVT_UPDATE_UI(ID_CRYST_UPDATEUI,                     WXRefinableObj::OnUpdateUI)
END_EVENT_TABLE()

WXDiffractionSingleCrystal::WXDiffractionSingleCrystal(wxWindow *parent,
                                                       DiffractionDataSingleCrystal* data):
WXRefinableObj(parent,data),mpData(data)
{
   VFN_DEBUG_MESSAGE("WXDiffractionSingleCrystal::WXDiffractionSingleCrystal()",6)
   mpWXTitle->SetForegroundColour(wxColour(255,0,0));
   mpWXTitle->SetSize(400,-1);
   // Menu
      mpMenuBar->AddMenu("File",ID_REFOBJ_MENU_OBJ);
         //:TODO: reactivate & test those menus
         //mpMenuBar->AddMenuItem(ID_REFOBJ_MENU_OBJ,ID_REFOBJ_MENU_OBJ_SAVE,"Save");
         //mpMenuBar->AddMenuItem(ID_REFOBJ_MENU_OBJ,ID_REFOBJ_MENU_OBJ_LOAD,"Load");
         mpMenuBar->AddMenuItem(ID_REFOBJ_MENU_OBJ,ID_DIFFSINGLECRYST_MENU_SAVEHKLIOBSICALC,
                                "Save HKL Iobs Icalc (text)");
         mpMenuBar->AddMenuItem(ID_REFOBJ_MENU_OBJ,ID_DIFFSINGLECRYST_MENU_SAVEHKLFCALC,
                                "Save HKL Fcalc (text)");
         mpMenuBar->AddMenuItem(ID_REFOBJ_MENU_OBJ,ID_DIFFSINGLECRYST_MENU_SIMULATE,
                                "Simulation mode (generate HKL list)");
         //mpMenuBar->AppendSeparator();
         mpMenuBar->AddMenuItem(ID_REFOBJ_MENU_OBJ,ID_DIFFSINGLECRYST_MENU_IMPORT_HKLIOBS,
                                 "Import HKL Iobs");
         mpMenuBar->AddMenuItem(ID_REFOBJ_MENU_OBJ,ID_DIFFSINGLECRYST_MENU_IMPORT_HKLIOBSSIGMA,
                                 "Import HKL Iobs Sigma");
         //mpMenuBar->AddMenuItem(ID_REFOBJ_MENU_OBJ,ID_DIFFSINGLECRYST_MENU_IMPORT_JANAM91,
         //                        "Import Jana M91");
         mpMenuBar->AddMenuItem(ID_REFOBJ_MENU_OBJ,ID_DIFFSINGLECRYST_MENU_IMPORT_HKLIOBSGROUP,
                                 "Import Reflections with group intensity");
        
      mpMenuBar->AddMenu("Radiation",ID_DIFFSINGLECRYST_MENU_WAVELENGTH);
         mpMenuBar->AddMenuItem(ID_DIFFSINGLECRYST_MENU_WAVELENGTH,
                                ID_DIFFSINGLECRYST_MENU_WAVELENGTH_NEUTRON,
                                "Neutron");
         mpMenuBar->AddMenuItem(ID_DIFFSINGLECRYST_MENU_WAVELENGTH,
                                ID_DIFFSINGLECRYST_MENU_WAVELENGTH_XRAY,
                                "X-Ray");
         mpMenuBar->AddMenuItem(ID_DIFFSINGLECRYST_MENU_WAVELENGTH,
                                ID_DIFFSINGLECRYST_MENU_WAVELENGTH_ELECTRON,
                                "Electron");
         mpMenuBar->AddMenuItem(ID_DIFFSINGLECRYST_MENU_WAVELENGTH,
                                ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET,
                                "Monochromatic Wavelength");
         mpMenuBar->AddMenuItem(ID_DIFFSINGLECRYST_MENU_WAVELENGTH,
                                ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_AG,
                                "X-Ray Tube Ag Ka12");
         mpMenuBar->AddMenuItem(ID_DIFFSINGLECRYST_MENU_WAVELENGTH,
                                ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_AGA1,
                                "X-Ray Tube Ag Ka1");
         mpMenuBar->AddMenuItem(ID_DIFFSINGLECRYST_MENU_WAVELENGTH,
                                ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_MO,
                                "X-Ray Tube Mo Ka12");
         mpMenuBar->AddMenuItem(ID_DIFFSINGLECRYST_MENU_WAVELENGTH,
                                ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_MOA1,
                                "X-Ray Tube Mo Ka1");
         mpMenuBar->AddMenuItem(ID_DIFFSINGLECRYST_MENU_WAVELENGTH,
                                ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CU,
                                "X-Ray Tube Cu Ka12");
         mpMenuBar->AddMenuItem(ID_DIFFSINGLECRYST_MENU_WAVELENGTH,
                                ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CUA1,
                                "X-Ray Tube Cu Ka1");
         mpMenuBar->AddMenuItem(ID_DIFFSINGLECRYST_MENU_WAVELENGTH,
                                ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_FE,
                                "X-Ray Tube Fe Ka12");
         mpMenuBar->AddMenuItem(ID_DIFFSINGLECRYST_MENU_WAVELENGTH,
                                ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_FEA1,
                                "X-Ray Tube Fe Ka1");
         mpMenuBar->AddMenuItem(ID_DIFFSINGLECRYST_MENU_WAVELENGTH,
                                ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CO,
                                "X-Ray Tube Co Ka12");
         mpMenuBar->AddMenuItem(ID_DIFFSINGLECRYST_MENU_WAVELENGTH,
                                ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_COA1,
                                "X-Ray Tube Co Ka1");
         mpMenuBar->AddMenuItem(ID_DIFFSINGLECRYST_MENU_WAVELENGTH,
                                ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CR,
                                "X-Ray Tube Cr Ka12");
         mpMenuBar->AddMenuItem(ID_DIFFSINGLECRYST_MENU_WAVELENGTH,
                                ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CRA1,
                                "X-Ray Tube Cr Ka1");
      //mpMenuBar->AddMenu("Compute",ID_CRYSTAL_MENU_DISPLAY);
      //   mpMenuBar->AddMenuItem(ID_CRYSTAL_MENU_DISPLAY,ID_DIFFSINGLECRYST_MENU_FITSCALE_R,
      //                          "Fit Scale for R");
      //   mpMenuBar->AddMenuItem(ID_CRYSTAL_MENU_DISPLAY,ID_DIFFSINGLECRYST_MENU_FITSCALE_RW,
      //                          "Fit Scale for Rw");
      mpSizer->SetItemMinSize(mpMenuBar,
                              mpMenuBar->GetSize().GetWidth(),
                              mpMenuBar->GetSize().GetHeight());
   //Radiation
      mpSizer->Add(mpData->mRadiation.WXCreate(this),0);
      mList.Add(mpData->mRadiation.WXGet());
   // Crystal
      mpFieldCrystal=new WXFieldChoice(this,ID_DIFFSINGLECRYST_CRYSTAL,"Crystal:",300);
      mpSizer->Add(mpFieldCrystal,0,wxALIGN_LEFT);
      mList.Add(mpFieldCrystal);
   // Max Sin(theta/Lambda)
      WXFieldPar<REAL> *maxSiThOvLa=
         new WXFieldPar<REAL>(this,"Max Sin(theta)/lambda:",-1,&(mpData->mMaxSinThetaOvLambda));
      mpSizer->Add(maxSiThOvLa,0,wxALIGN_LEFT);
      mList.Add(maxSiThOvLa);
   // Statistics
      wxBoxSizer* pStats=new wxBoxSizer(wxHORIZONTAL);
      
      WXFieldPar<REAL> *pWXFieldChi2=new WXFieldPar<REAL>(this,"Chi^2",-1,&mChi2,100);
      pStats->Add(pWXFieldChi2    ,0,wxALIGN_CENTER);
      mList.Add(pWXFieldChi2);
      
      WXFieldPar<REAL> *pWXFieldGof=new WXFieldPar<REAL>(this,"GoF",-1,&mGoF,70);
      pStats->Add(pWXFieldGof    ,0,wxALIGN_CENTER);
      mList.Add(pWXFieldGof);
      
      WXFieldPar<REAL> *pWXFieldRwp=new WXFieldPar<REAL>(this,"Rwp",-1,&mRwp,70);
      pStats->Add(pWXFieldRwp    ,0,wxALIGN_CENTER);
      mList.Add(pWXFieldRwp);
      
      WXFieldPar<REAL> *pWXFieldRp=new WXFieldPar<REAL>(this,"Rp",-1,&mRp,70);
      pStats->Add(pWXFieldRp    ,0,wxALIGN_CENTER);
      mList.Add(pWXFieldRp);
      
      mpSizer->Add(pStats);
         
   this->BottomLayout(0);
   this->CrystUpdate(true);
   VFN_DEBUG_MESSAGE("WXDiffractionSingleCrystal::WXDiffractionSingleCrystal():End",6)
}

void WXDiffractionSingleCrystal::CrystUpdate(const bool uui,const bool lock)
{
   VFN_DEBUG_ENTRY("WXDiffractionSingleCrystal::CrystUpdate()",6)
   if(lock) mMutex.Lock();
   WXCrystValidateAllUserInput();

   mChi2=mpData->GetChi2();
   if(0==mpData->GetIobs().numElements()) mGoF=0;
   else mGoF=mpData->GetChi2()/mpData->GetIobs().numElements();
   mRwp=mpData->GetRw();
   mRp=mpData->GetR();
   
   if(lock) mMutex.Unlock();
   this->WXRefinableObj::CrystUpdate(uui,lock);
   VFN_DEBUG_EXIT("WXDiffractionSingleCrystal::CrystUpdate()",6)
} 

void WXDiffractionSingleCrystal::OnMenuSimulate(wxCommandEvent & WXUNUSED(event))
{
   WXCrystValidateAllUserInput();
   double theta;
   {
      wxTextEntryDialog dialog(this,"Theta Max",
                              "Enter maximum Theta (degrees)","50",wxOK | wxCANCEL);
      if(wxID_OK!=dialog.ShowModal())
      {
         VFN_DEBUG_EXIT("WXDiffractionSingleCrystal::OnMenuSimulate():Cancelled",6)
         return;
      }
      dialog.GetValue().ToDouble(&theta);
   }
   wxString choices[2];
   choices[0]="all reflections (keep Friedel&Bijvoet mates)";
   choices[1]="only unique reflections";
   wxSingleChoiceDialog dialog(this,"Choose method",
                               "Choose method for Reflection generation",
                               2,choices,0,wxOK | wxCANCEL);
   if(wxID_OK!=dialog.ShowModal()) return;
   const int choice=dialog.GetSelection();
   if(0==choice) mpData->GenHKLFullSpace(theta*DEG2RAD,false);
   else mpData->GenHKLFullSpace(theta*DEG2RAD,true);
}
void WXDiffractionSingleCrystal::OnMenuImport(wxCommandEvent & event)
{
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_IMPORT_HKLIOBS)
   {
      wxFileDialog open(this,"Choose file to import from",
                                     "","","*.*",wxOPEN | wxFILE_MUST_EXIST);
      if(open.ShowModal() != wxID_OK) return;
      long nb=0;
      {
         wxTextEntryDialog dialog(this,"Number of reflections",
                                 "Enter The number of reflections to import","50",
                                 wxOK | wxCANCEL);
         if(wxID_OK!=dialog.ShowModal())
         {
            VFN_DEBUG_EXIT("WXDiffractionSingleCrystal))OnMenuImport())Cancelled",6)
            return;
         }
         dialog.GetValue().ToLong(&nb);
      }
      mpData->ImportHklIobs(open.GetPath().c_str(),nb);
      mpData->UpdateDisplay();
      return;
   }
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_IMPORT_HKLIOBSSIGMA)
   {
      wxFileDialog open(this,"Choose file to import from",
                                     "","","*.*",wxOPEN | wxFILE_MUST_EXIST);
      if(open.ShowModal() != wxID_OK) return;
      long nb=0;
      {
         wxTextEntryDialog dialog(this,"Number of reflections",
                                 "Enter The number of reflections to import","50",
                                 wxOK | wxCANCEL);
         if(wxID_OK!=dialog.ShowModal())
         {
            VFN_DEBUG_EXIT("WXDiffractionSingleCrystal))OnMenuImport())Cancelled",6)
            return;
         }
         dialog.GetValue().ToLong(&nb);
      }
      mpData->ImportHklIobsSigma(open.GetPath().c_str(),nb);
      mpData->UpdateDisplay();
      return;
   }
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_IMPORT_HKLIOBSGROUP)
   {
      wxFileDialog open(this,"Choose data file",
                                     "","","*.*",wxOPEN | wxFILE_MUST_EXIST);
      if(open.ShowModal() != wxID_OK) return;
      mpData->ImportHklIobsGroup(open.GetPath().c_str());
      mpData->UpdateDisplay();
      return;
   }
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_IMPORT_JANAM91)
   {
      wxFileDialog open(this,"Choose data file",
                                     "","","*.*",wxOPEN | wxFILE_MUST_EXIST);
      if(open.ShowModal() != wxID_OK) return;
      mpData->ImportHklIobsSigmaJanaM91(open.GetPath().c_str());
      mpData->UpdateDisplay();
      return;
   }
}
void WXDiffractionSingleCrystal::OnMenuSaveHKLIobsIcalc(wxCommandEvent & WXUNUSED(event))
{
   VFN_DEBUG_MESSAGE("WXDiffractionSingleCrystal::OnMenuSaveHKLIobsIcalc()",6)
   WXCrystValidateAllUserInput();
   wxFileDialog save(this,"Choose a file","","","*.txt",wxSAVE | wxOVERWRITE_PROMPT);
   if(save.ShowModal() != wxID_OK) return;
   mpData->SaveHKLIobsIcalc(save.GetPath().c_str());
}
void WXDiffractionSingleCrystal::OnMenuSaveHKLFcalc(wxCommandEvent & WXUNUSED(event))
{
   VFN_DEBUG_MESSAGE("WXDiffractionSingleCrystal::OnMenuSaveHKLFcalc()",6)
   WXCrystValidateAllUserInput();
   wxFileDialog save(this,"Choose a file","","","*.txt",wxSAVE | wxOVERWRITE_PROMPT);
   if(save.ShowModal() != wxID_OK) return;
   ofstream os(save.GetPath().c_str());
   mpData->PrintFhklCalcDetail(os);
   os.close();
   mpData->GetCrystal().GetScatteringComponentList().Print();
}
void WXDiffractionSingleCrystal::OnMenuSetWavelength(wxCommandEvent &event)
{
   WXCrystValidateAllUserInput();
   //:TODO: Use wxRadiation instead
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_WAVELENGTH_XRAY)
      mpData->SetRadiationType(RAD_XRAY);
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_WAVELENGTH_NEUTRON)
      mpData->SetRadiationType(RAD_NEUTRON);
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_WAVELENGTH_ELECTRON)
      mpData->SetRadiationType(RAD_ELECTRON);
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET)
   {
      double lambda;
      wxTextEntryDialog dialog(this,"new Wavelength)",
                              "Enter new Wavelength (Angstroems)","1",wxOK | wxCANCEL);
      if(wxID_OK!=dialog.ShowModal())
      {
         VFN_DEBUG_EXIT("WXDiffractionSingleCrystal))OnMenuSetWavelength())Monochromatic)Cancelled",6)
         return;
      }
      dialog.GetValue().ToDouble(&lambda);
      mpData->SetWavelength(lambda);
   }
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_AG)
      mpData->SetWavelength("Ag");
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_MO)
      mpData->SetWavelength("Mo");
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CU)
      mpData->SetWavelength("Cu");
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_FE)
      mpData->SetWavelength("Fe");
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CO)
      mpData->SetWavelength("Co");
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CR)
      mpData->SetWavelength("Cr");
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_AGA1)
      mpData->SetWavelength("AgA1");
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_MOA1)
      mpData->SetWavelength("MoA1");
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CUA1)
      mpData->SetWavelength("CuA1");
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_FEA1)
      mpData->SetWavelength("FeA1");
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_COA1)
      mpData->SetWavelength("CoA1");
   if(event.GetId()== ID_DIFFSINGLECRYST_MENU_WAVELENGTH_SET_CRA1)
      mpData->SetWavelength("CrA1");
   this->CrystUpdate(true,true);
}
void WXDiffractionSingleCrystal::OnChangeCrystal(wxCommandEvent & WXUNUSED(event))
{
   VFN_DEBUG_MESSAGE("WXDiffractionSingleCrystal::OnChangeCrystal()",6)
   WXCrystValidateAllUserInput();
   int choice;
   Crystal *cryst=dynamic_cast<Crystal*>
      (WXDialogChooseFromRegistry(gCrystalRegistry,(wxWindow*)this,
         "Choose a Crystal Structure:",choice));
   if(0==cryst) return;
   mpData->SetCrystal(*cryst);
   this->CrystUpdate(true,true);
}
void WXDiffractionSingleCrystal::UpdateUI(const bool lock)
{
   if(lock) mMutex.Lock();
   if(&(mpData->GetCrystal())!=0) mpFieldCrystal->SetValue(mpData->GetCrystal().GetName());
   this->WXRefinableObj::UpdateUI(false);
   if(lock) mMutex.Unlock();
}

}//namespace

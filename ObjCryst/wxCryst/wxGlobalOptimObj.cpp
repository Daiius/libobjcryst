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

#include "wx/wx.h"
#include "wxCryst/wxGlobalOptimObj.h"

// Next two just to fix some parameters during global optimization
#include "ObjCryst/Crystal.h"
#include "ObjCryst/ScatteringData.h"

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
//    WXOptimizationObj
//
////////////////////////////////////////////////////////////////////////
WXOptimizationObj::WXOptimizationObj(wxWindow* parent, OptimizationObj *obj):
WXCrystObj(parent),mpGlobalOptimRunThread(0)
{
   VFN_DEBUG_ENTRY("WXOptimizationObj::WXOptimizationObj(wxWindow*,GlobalOptimObj*,)",7)
   mpWXTitle->SetForegroundColour(wxColour(255,0,0));
   mpWXTitle->SetLabel("Global Optimization:");
   // Menu
      mpMenuBar=new WXCrystMenuBar(this,this);
      mpSizer->Add(mpMenuBar);
      mList.Add(mpMenuBar);
      
      //mpMenuBar->AddMenu("Object",ID_REFOBJ_MENU_OBJ);
         //mpMenuBar->AddMenuItem(ID_REFOBJ_MENU_OBJ,ID_REFOBJ_MENU_OBJ_SAVE,"Save");
         //mpMenuBar->AddMenuItem(ID_REFOBJ_MENU_OBJ,ID_REFOBJ_MENU_OBJ_LOAD,"Load");
      mpMenuBar->AddMenu("Optimized Objects",ID_GLOBALOPT_MENU_OBJECTS);
         mpMenuBar->AddMenuItem(ID_GLOBALOPT_MENU_OBJECTS,ID_GLOBALOPT_MENU_OBJECTS_ADDOBJ,
                                "Add object to optimize");
      mpMenuBar->AddMenu("Run/Stop",ID_GLOBALOPT_MENU_OPT);
         mpMenuBar->AddMenuItem(ID_GLOBALOPT_MENU_OPT,
                                ID_GLOBALOPT_MENU_OPT_RUN,"Run Optimization");
         mpMenuBar->AddMenuItem(ID_GLOBALOPT_MENU_OPT,
                                ID_GLOBALOPT_MENU_OPT_STOP,"Stop Optimization");
      
    //Refined Objects
   for(int i=0;i<obj->mRefinedObjList.GetNb();i++)
   {
      WXFieldName *refobj=new WXFieldName(this,"Optimized object:",this,-1,300,false);
      refobj->SetValue(obj->mRefinedObjList.GetObj(i).GetClassName()
                       +":"+obj->mRefinedObjList.GetObj(i).GetName());
      mpSizer->Add(refobj);
      mList.Add(refobj);
   }
   
   // This will be done later
   // this->CrystUpdate();
   // this->Layout();
   VFN_DEBUG_EXIT("WXOptimizationObj::WXOptimizationObj(wxWindow*,GlobalOptimObj*,)",7)
}

void WXOptimizationObj::CrystUpdate()
{
   this->WXCrystObj::CrystUpdate();
   if(true==wxThread::IsMain()) this->UpdateUI();
   else
   {
      wxUpdateUIEvent event(ID_CRYST_UPDATEUI);
      wxPostEvent(this,event);
   }
}

bool WXOptimizationObj::OnChangeName(const int id)
{
   VFN_DEBUG_MESSAGE("WXOptimizationObj::OnChangeName()",6)
   switch(id)
   {
      case ID_WXOBJ_NAME:
      {
      VFN_DEBUG_MESSAGE("WXOptimizationObj::OnChangeName():Changing GlobalOptimObj Name",6)
         this->GetOptimizationObj().SetName(mpWXTitle->GetValue());
         return true;
      }
   }
   return false;
}
void WXOptimizationObj::OnSave(){}

void WXOptimizationObj::OnLoad(){}

void WXOptimizationObj::OnAddRefinedObject()
{
   WXCrystValidateAllUserInput();
   int choice;
   RefinableObj *obj=
      WXDialogChooseFromRegistry(gTopRefinableObjRegistry,this,
                                 "Choose object to optimize:",choice);
   if(0==obj) return;
   this->GetOptimizationObj().AddRefinableObj(*obj);
}

void WXOptimizationObj::AddRefinedObject(RefinableObj &obj)
{
   WXCrystValidateAllUserInput();
   WXFieldName *refobj=new WXFieldName(this,"Optimized object:",this,-1,300,false);
   refobj->SetValue(obj.GetClassName()+":"+obj.GetName());
   mpSizer->Add(refobj);
   mList.Add(refobj);
   this->Layout();
}

void WXOptimizationObj::OnRemoveRefinedObject()
{
}

void WXOptimizationObj::OnStopOptimization()
{
   this->GetOptimizationObj().StopAfterCycle();
}
void WXOptimizationObj::OnUpdateUI(wxUpdateUIEvent& event)
{
   VFN_DEBUG_ENTRY("WXOptimizationObj::OnUpdateUI()",5)
   this->UpdateUI();
   VFN_DEBUG_EXIT("WXOptimizationObj::OnUpdateUI()",5)
}

void WXOptimizationObj::UpdateUI()
{
   VFN_DEBUG_ENTRY("WXOptimizationObj::UpdateUI()",5)
   mpWXTitle->SetValue(this->GetOptimizationObj().GetName());
   mpWXTitle->UpdateUI();
   this->WXCrystObj::UpdateUI();
   VFN_DEBUG_EXIT("WXOptimizationObj::UpdateUI()",5)
}

////////////////////////////////////////////////////////////////////////
//
//    WXGlobalOptimRunThread
//
////////////////////////////////////////////////////////////////////////
WXGlobalOptimRunThread::WXGlobalOptimRunThread(OptimizationObj &globalOptObj,
                                               long &nbTrial,const REAL finalCost):
wxThread(wxTHREAD_DETACHED),mpGlobalOptObj(&globalOptObj),mpNbTrial(&nbTrial),
mFinalCost(finalCost)
{
}

void *WXGlobalOptimRunThread::Entry()
{
   cout <<endl<<"Entering refinement thread "<<endl<<endl;
   mpGlobalOptObj->Optimize(*mpNbTrial,false,mFinalCost);
   return NULL;
}
void WXGlobalOptimRunThread::OnExit()
{
   cout <<endl<<"Exiting refinement thread "<<endl<<endl;
}
////////////////////////////////////////////////////////////////////////
//
//    WXMonteCarloObj
//
////////////////////////////////////////////////////////////////////////
BEGIN_EVENT_TABLE(WXMonteCarloObj, wxWindow)
   EVT_BUTTON(ID_WXOBJ_COLLAPSE,                       WXCrystObj::OnToggleCollapse)
   //EVT_MENU(ID_REFOBJ_MENU_OBJ_SAVE,                   WXOptimizationObj::OnSave)
   //EVT_MENU(ID_REFOBJ_MENU_OBJ_LOAD,                   WXOptimizationObj::OnLoad)
   EVT_MENU(ID_GLOBALOPT_MENU_OBJECTS_ADDOBJ,        WXOptimizationObj::OnAddRefinedObject)
   EVT_MENU(ID_GLOBALOPT_MENU_OPT_RUN,           WXOptimizationObj::OnRunOptimization)
   EVT_MENU(ID_GLOBALOPT_MENU_OPT_STOP,          WXOptimizationObj::OnStopOptimization)
   EVT_UPDATE_UI(ID_CRYST_UPDATEUI,                    WXOptimizationObj::OnUpdateUI)
END_EVENT_TABLE()

WXMonteCarloObj::WXMonteCarloObj(wxWindow *parent, MonteCarloObj* obj):
WXOptimizationObj(parent,obj),mpMonteCarloObj(obj),mNbTrial(100000000)
{
   VFN_DEBUG_ENTRY("WXMonteCarloObj::WXMonteCarloObj()",7)
   //options
      WXFieldOption *opt;
      
      opt=new WXFieldOption(this,-1,&(mpMonteCarloObj->mGlobalOptimType));
      mpSizer->Add(opt,0,wxALIGN_LEFT);
      mList.Add(opt);
      
      opt=new WXFieldOption(this,-1,&(mpMonteCarloObj->mAnnealingScheduleTemp));
      mpSizer->Add(opt,0,wxALIGN_LEFT);
      mList.Add(opt);

      wxBoxSizer *sizerTemp=new wxBoxSizer(wxHORIZONTAL);
      WXFieldPar<REAL> *tempMax=
         new WXFieldPar<REAL>(this,"Temperature Max:",-1,&(mpMonteCarloObj->mTemperatureMax));
      WXFieldPar<REAL> *tempMin=
         new WXFieldPar<REAL>(this,"Temperature Min:",-1,&(mpMonteCarloObj->mTemperatureMin));
      sizerTemp->Add(tempMax);
      sizerTemp->Add(tempMin);
      mpSizer->Add(sizerTemp);
      mList.Add(tempMax);
      mList.Add(tempMin);
      
      opt=new WXFieldOption(this,-1,&(mpMonteCarloObj->mAnnealingScheduleMutation));
      mpSizer->Add(opt,0,wxALIGN_LEFT);
      mList.Add(opt);
      
      wxBoxSizer *sizerAmp=new wxBoxSizer(wxHORIZONTAL);
      WXFieldPar<REAL> *ampMax=
         new WXFieldPar<REAL>(this,"Amplitude Max:",-1,&(mpMonteCarloObj->mMutationAmplitudeMax));
      WXFieldPar<REAL> *ampMin=
         new WXFieldPar<REAL>(this,"Amplitude Min:",-1,&(mpMonteCarloObj->mMutationAmplitudeMin));
      sizerAmp->Add(ampMax);
      sizerAmp->Add(ampMin);
      mpSizer->Add(sizerAmp);
      mList.Add(ampMax);
      mList.Add(ampMin);
      
      #if 0
      // Only to evaluate how the algorithm works
      opt=new WXFieldOption(this,-1,&(mpMonteCarloObj->mSaveDetailledHistory));
      mpSizer->Add(opt,0,wxALIGN_LEFT);
      mList.Add(opt);
      #endif
      opt=new WXFieldOption(this,-1,&(mpMonteCarloObj->mXMLAutoSave));
      mpSizer->Add(opt,0,wxALIGN_LEFT);
      mList.Add(opt);
   // Number of trials to go
      mpWXFieldNbTrial=new WXFieldPar<long>(this,"Number of trials to go:",-1,&mNbTrial,70);
      mpSizer->Add(mpWXFieldNbTrial);
      mList.Add(mpWXFieldNbTrial);
   this->CrystUpdate();
   this->Layout();
   VFN_DEBUG_EXIT("WXMonteCarloObj::WXMonteCarloObj()",7)
}

void WXMonteCarloObj::OnRunOptimization()
{
   VFN_DEBUG_ENTRY("WXGeneticAlgorithm::OnRunOptimization()",6)
   WXCrystValidateAllUserInput();
   if(true==this->GetOptimizationObj().IsOptimizing())
   {
      wxMessageDialog dumbUser(this,"The optimization is already running !","Huh ?",
                               wxOK|wxICON_EXCLAMATION);
      dumbUser.ShowModal();
      VFN_DEBUG_EXIT("WXGeneticAlgorithm::OnRunOptimization()",6)
      return;
   }
   
   //Fix parameters than really should not be global-optimized
      mpMonteCarloObj->SetParIsFixed(gpRefParTypeUnitCell,true);
      mpMonteCarloObj->SetParIsFixed(gpRefParTypeScattDataScale,true);
      mpMonteCarloObj->SetParIsFixed(gpRefParTypeScattDataProfile,true);
      mpMonteCarloObj->SetParIsFixed(gpRefParTypeScattDataCorrIntAbsorp,true);
      mpMonteCarloObj->SetParIsFixed(gpRefParTypeScattDataCorrIntPolar,true);
      mpMonteCarloObj->SetParIsFixed(gpRefParTypeScattDataCorrIntExtinc,true);
      mpMonteCarloObj->SetParIsFixed(gpRefParTypeScattDataCorrPos,true);
      mpMonteCarloObj->SetParIsFixed(gpRefParTypeScattDataBackground,true);
      mpMonteCarloObj->SetParIsFixed(gpRefParTypeRadiation,true);
      mpMonteCarloObj->UpdateDisplay();
   
   double finalCost=0;
   if(mNbTrial<0)
   {
      mNbTrial = - mNbTrial;
      wxTextEntryDialog costDialog(this,"Enter desired cost for the optimization to stop",
                              "Goal Cost",".20",wxOK | wxCANCEL);
      if(wxID_OK==costDialog.ShowModal()) costDialog.GetValue().ToDouble(&finalCost);
   }
   mpGlobalOptimRunThread = new WXGlobalOptimRunThread(this->GetOptimizationObj(),mNbTrial,finalCost);
   if(mpGlobalOptimRunThread->Create() != wxTHREAD_NO_ERROR) 
      wxLogError("Can't create optimization thread");
   else mpGlobalOptimRunThread->Run();
   
   VFN_DEBUG_EXIT("WXMonteCarloObj::OnRunOptimization()",6)
}
void WXMonteCarloObj::UpdateDisplayNbTrial()
{
   VFN_DEBUG_MESSAGE("WXMonteCarloObj::UpdateDisplayNbTrial()",5)
   mpWXFieldNbTrial->CrystUpdate();
   wxUpdateUIEvent event(ID_CRYST_UPDATEUI);
   wxPostEvent(this,event);
}
OptimizationObj & WXMonteCarloObj::GetOptimizationObj()
{
   VFN_DEBUG_MESSAGE("WXMonteCarloObj::GetOptimizationObj()",2)
   return *mpMonteCarloObj;
}
const OptimizationObj & WXMonteCarloObj::GetOptimizationObj()const
{
   VFN_DEBUG_MESSAGE("WXMonteCarloObj::GetOptimizationObj() const",2)
   return *mpMonteCarloObj;
}


}// namespace 


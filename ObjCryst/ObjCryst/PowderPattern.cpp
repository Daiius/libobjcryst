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
/*
*  source file for LibCryst++ PowderPattern class
*
*/

#include <cmath>

#include <typeinfo>
#include <stdio.h> //for sprintf()
#include "ObjCryst/PowderPattern.h"
#include "ObjCryst/PowderPatternBackgroundBayesianMinimiser.h"
#include "RefinableObj/Simplex.h"
#include "Quirks/VFNDebug.h"
#include "Quirks/VFNStreamFormat.h"
#ifdef __WX__CRYST__
   #include "wxCryst/wxPowderPattern.h"
#endif

#include <fstream>
#include <iomanip>
#include <sstream>

#ifdef _MSC_VER // MS VC++ predefined macros....
#undef min
#undef max
#endif

//#define USE_BACKGROUND_MAXLIKE_ERROR

namespace ObjCryst
{

////////////////////////////////////////////////////////////////////////
//
//    PowderPatternComponent    
//
////////////////////////////////////////////////////////////////////////
ObjRegistry<PowderPatternComponent> 
   gPowderPatternComponentRegistry("List of all PowderPattern Components");

PowderPatternComponent::PowderPatternComponent():
mIsScalable(false),mpParentPowderPattern(0)
{
   gPowderPatternComponentRegistry.Register(*this);
   mClockMaster.AddChild(mClockBraggLimits);
}

PowderPatternComponent::PowderPatternComponent(const PowderPatternComponent &old):
mIsScalable(old.mIsScalable),
mpParentPowderPattern(old.mpParentPowderPattern)
{
   mClockMaster.AddChild(mClockBraggLimits);
   if(mpParentPowderPattern!=0)
   {
      mClockMaster.AddChild(mpParentPowderPattern->GetClockPowderPatternPar());
      mClockMaster.AddChild(mpParentPowderPattern->GetClockPowderPattern2ThetaCorr());
      mClockMaster.AddChild(mpParentPowderPattern->GetClockPowderPatternRadiation());
   }
}

PowderPatternComponent::~PowderPatternComponent()
{
   gPowderPatternComponentRegistry.DeRegister(*this);
}

const string& PowderPatternComponent::GetClassName() const
{
   const static string className="PowderPatternComponent";
   return className;
}

const PowderPattern& PowderPatternComponent::GetParentPowderPattern()const
{
   return *mpParentPowderPattern;
}

bool PowderPatternComponent::IsScalable()const {return mIsScalable;}

const RefinableObjClock& PowderPatternComponent::GetClockPowderPatternCalc()const
{
   return mClockPowderPatternCalc;
}
const RefinableObjClock& PowderPatternComponent::GetClockBraggLimits()const
{
   return mClockBraggLimits;
}

const list<pair<const REAL, const string> >& PowderPatternComponent::GetPatternLabelList()const
{
   return mvLabel;
}

////////////////////////////////////////////////////////////////////////
//
//        PowderPatternBackground
//
////////////////////////////////////////////////////////////////////////
PowderPatternBackground::PowderPatternBackground():
mBackgroundNbPoint(0),
mMaxSinThetaOvLambda(10),mModelVariance(0)
{
   mClockMaster.AddChild(mClockBackgroundPoint);
   this->InitOptions();
}

PowderPatternBackground::PowderPatternBackground(const  PowderPatternBackground &old):
mBackgroundNbPoint(old.mBackgroundNbPoint),
mBackgroundInterpPoint2Theta(old.mBackgroundInterpPoint2Theta),
mBackgroundInterpPointIntensity(old.mBackgroundInterpPointIntensity),
mMaxSinThetaOvLambda(10),mModelVariance(0)
{
   mClockMaster.AddChild(mClockBackgroundPoint);
   this->InitOptions();
   mInterpolationModel.SetChoice(old.mInterpolationModel.GetChoice());
}

PowderPatternBackground::~PowderPatternBackground(){}
const string& PowderPatternBackground::GetClassName() const
{
   const static string className="PowderPatternBackground";
   return className;
}

void PowderPatternBackground::SetParentPowderPattern(const PowderPattern &s)
{
   if(mpParentPowderPattern!=0) 
      mClockMaster.RemoveChild(mpParentPowderPattern->GetIntegratedProfileLimitsClock());
   mpParentPowderPattern = &s;
   mClockMaster.AddChild(mpParentPowderPattern->GetIntegratedProfileLimitsClock());
   mClockMaster.AddChild(mpParentPowderPattern->GetClockPowderPatternPar());
   mClockMaster.AddChild(mpParentPowderPattern->GetClockPowderPattern2ThetaCorr());
   mClockMaster.AddChild(mpParentPowderPattern->GetClockPowderPatternRadiation());
}
const CrystVector_REAL& PowderPatternBackground::GetPowderPatternCalc()const
{
   this->CalcPowderPattern();
   return mPowderPatternCalc;
}

pair<const CrystVector_REAL*,const RefinableObjClock*>
   PowderPatternBackground::GetPowderPatternIntegratedCalc()const
{
   VFN_DEBUG_MESSAGE("PowderPatternBackground::GetPowderPatternIntegratedCalc()",3)
   this->CalcPowderPatternIntegrated();
   return make_pair(&mPowderPatternIntegratedCalc,&mClockPowderPatternIntegratedCalc);
}

void PowderPatternBackground::ImportUserBackground(const string &filename)
{
   VFN_DEBUG_MESSAGE("PowderPatternBackground::ImportUserBackground():"<<filename,5)
   ifstream fin (filename.c_str());
   if(!fin)
   {
      throw ObjCrystException("PowderPatternBackground::ImportUserBackground() : \
Error opening file for input:"+filename);
   }
   long max=100;
   CrystVector_REAL bckgd2Theta(max),bckgd(max);
   
   long nbPoints=0;
   do
   {
      fin >> bckgd2Theta(nbPoints);
      fin >> bckgd(nbPoints);
      if(!fin) break;
      VFN_DEBUG_MESSAGE("Background=" << bckgd(nbPoints)\
         <<" at 2theta="<<bckgd2Theta(nbPoints),3)
      nbPoints++;
      if(nbPoints==max)
      {
         max += 100;
         bckgd2Theta.resizeAndPreserve(max);
         bckgd.resizeAndPreserve(max);
      }
   } while(fin.eof() == false);
   fin.close();
   bckgd2Theta.resizeAndPreserve(nbPoints);
   bckgd.resizeAndPreserve(nbPoints);
   //cout << bckgd << endl;
   mBackgroundNbPoint=nbPoints;
   mBackgroundInterpPoint2Theta=bckgd2Theta;
   mBackgroundInterpPoint2Theta*= DEG2RAD;
   mBackgroundInterpPointIntensity=bckgd;
   this->InitRefParList();
   mClockBackgroundPoint.Click();
   {
      char buf [200];
      sprintf(buf,"Imported %d background points",(int)nbPoints);
      (*fpObjCrystInformUser)((string)buf);
   }
   this->UpdateDisplay();
   VFN_DEBUG_MESSAGE("PowderPatternBackground::ImportUserBackground():finished",5)
}
void PowderPatternBackground::SetInterpPoints(const CrystVector_REAL tth,
                                              const CrystVector_REAL backgd)
{
   VFN_DEBUG_ENTRY("PowderPatternBackground::SetInterpPoints():",5)
   if(  (tth.numElements()!=backgd.numElements())
      ||(tth.numElements()<2))
   {
      throw ObjCrystException("PowderPatternBackground::SetInterpPoints() : \
number of points differ or less than 2 points !");
   }
   mBackgroundNbPoint=tth.numElements();
   mBackgroundInterpPoint2Theta=tth;
   mBackgroundInterpPointIntensity=backgd;
   this->InitRefParList();
   mClockBackgroundPoint.Click();
   this->UpdateDisplay();
   VFN_DEBUG_EXIT("PowderPatternBackground::SetInterpPoints()",5)
}

void PowderPatternBackground::GetGeneGroup(const RefinableObj &obj,
                                CrystVector_uint & groupIndex,
                                unsigned int &first) const
{
   // One group for all background points
   unsigned int index=0;
   VFN_DEBUG_MESSAGE("PowderPatternBackground::GetGeneGroup()",4)
   for(long i=0;i<obj.GetNbPar();i++)
      for(long j=0;j<this->GetNbPar();j++)
         if(&(obj.GetPar(i)) == &(this->GetPar(j)))
         {
            if(index==0) index=first++;
            groupIndex(i)=index;
         }
}

const CrystVector_REAL& PowderPatternBackground::GetPowderPatternCalcVariance()const
{
   this->CalcPowderPattern();
   return mPowderPatternCalcVariance;
}

pair<const CrystVector_REAL*,const RefinableObjClock*>
   PowderPatternBackground::GetPowderPatternIntegratedCalcVariance()const
{
   this->CalcPowderPatternIntegrated();
   return make_pair(&mPowderPatternIntegratedCalcVariance,
                    &mClockPowderPatternIntegratedVarianceCalc);
}

bool PowderPatternBackground::HasPowderPatternCalcVariance()const
{
   #ifdef USE_BACKGROUND_MAXLIKE_ERROR
   return true;
   #else
   return false;
   #endif
}

void PowderPatternBackground::TagNewBestConfig()const
{
}

void PowderPatternBackground::OptimizeBayesianBackground()
{
   VFN_DEBUG_ENTRY("PowderPatternBackground::OptimizeBayesianBackground()",10);
   PowderPatternBackgroundBayesianMinimiser min(*this);
   SimplexObj simplex("Simplex Test");
   simplex.AddRefinableObj(min);
   long nbcycle;
   REAL lastllk=simplex.GetLogLikelihood();
   long ct=0;
   cout<<ct<<"Chi^2(BayesianBackground)="<<lastllk<<endl;
   this->GetParentPowderPattern().UpdateDisplay();
   this->SetGlobalOptimStep(gpRefParTypeScattDataBackground,
                            mBackgroundInterpPointIntensity.max()/100.0);
   for(;;)
   {
      {
         char buf [200];
         sprintf(buf,"Optimizing Background, Cycle %d, Chi^2(Background)=%f",
                 (int)ct,(float)lastllk);
         (*fpObjCrystInformUser)((string)buf);
      }
      nbcycle=50*mBackgroundNbPoint;
      simplex.Optimize(nbcycle,true);
      this->GetParentPowderPattern().UpdateDisplay();
      const REAL tmp=simplex.GetLogLikelihood();
      cout<<ct<<"Chi^2(BayesianBackground)="<<tmp<<endl;
      if((lastllk-tmp)<1e-4*lastllk) {lastllk=tmp ;break;}
      lastllk=tmp;
      if(++ct>20) break;
   }
   this->SetGlobalOptimStep(gpRefParTypeScattDataBackground,10.0);
   {
      char buf [200];
      sprintf(buf,"Done Optimizing Bayesian Background, Chi^2(Background)=%f",(float)lastllk);
      (*fpObjCrystInformUser)((string)buf);
   }
   VFN_DEBUG_EXIT("PowderPatternBackground::OptimizeBayesianBackground()",10);
}

void PowderPatternBackground::CalcPowderPattern() const
{
   if(mClockPowderPatternCalc>mClockMaster) return;
   
   //:TODO: This needs serious optimization !
   if(   (mClockPowderPatternCalc>mClockBackgroundPoint)
       &&(mClockPowderPatternCalc>mpParentPowderPattern->GetClockPowderPatternPar())
       &&(mClockPowderPatternCalc>mInterpolationModel.GetClock())) return;
   TAU_PROFILE("PowderPatternBackground::CalcPowderPattern()","void ()",TAU_DEFAULT);
   VFN_DEBUG_MESSAGE("PowderPatternBackground::CalcPowderPattern()",3);
   
   switch(mInterpolationModel.GetChoice())
   {
      case POWDER_BACKGROUND_LINEAR:
      {
         VFN_DEBUG_MESSAGE("PowderPatternBackground::CalcPowderPattern()..Linear",2)
         REAL t1,t2,b1,b2,t;
         const long nbPoint=mpParentPowderPattern->GetNbPoint();
         mPowderPatternCalc.resize(nbPoint);
         if(mBackgroundNbPoint==0)
         {
            mPowderPatternCalc=0;
            break;
         }
         VFN_DEBUG_MESSAGE("PowderPatternBackground::CalcPowderPattern()"<<nbPoint,2)
         //mPowderPatternCalc=0.;
         REAL *b=mPowderPatternCalc.data();
         t1=mBackgroundInterpPoint2Theta(0);
         t2=mBackgroundInterpPoint2Theta(1);
         b1=mBackgroundInterpPointIntensity(0);
         b2=mBackgroundInterpPointIntensity(1);
         t=mpParentPowderPattern->Get2ThetaMin();
         long point=1;
         for(long i=0;i<nbPoint;i++)
         {
            if(t >= t2)
            {
               if(point < mBackgroundNbPoint-1)
               {
                  b1=b2;
                  t1=t2;
                  b2=mBackgroundInterpPointIntensity(point+1);
                  t2=mBackgroundInterpPoint2Theta(point+1);
                  point++ ;
               }
            }
            *b = (b1*(t2-t)+b2*(t-t1))/(t2-t1) ;
            b++;
            t+=mpParentPowderPattern->Get2ThetaStep();
         }
         break;
      }
      case POWDER_BACKGROUND_CUBIC_SPLINE:
      {
         const unsigned long nb=mpParentPowderPattern->GetNbPoint();
         mPowderPatternCalc.resize(nb);
         if(mBackgroundNbPoint==0)
         {
            mPowderPatternCalc=0;
            break;
         }
         const REAL tth0=mpParentPowderPattern->Get2ThetaMin();
         const REAL tths=mpParentPowderPattern->Get2ThetaStep();
         CubicSpline spline(mBackgroundInterpPoint2Theta,mBackgroundInterpPointIntensity);
         for(unsigned long i=0;i<nb;++i) mPowderPatternCalc(i)=spline(tth0+i*tths);
         break;
      }
   }
   VFN_DEBUG_MESSAGE("PowderPatternBackground::CalcPowderPattern()",3);
   #ifdef USE_BACKGROUND_MAXLIKE_ERROR
   {
      const long nbPoint=mpParentPowderPattern->GetNbPoint();
      mPowderPatternCalcVariance.resize(nbPoint);
      const REAL step=mModelVariance*mModelVariance/(REAL)nbPoint;
      REAL var=0;
      REAL *p=mPowderPatternCalcVariance.data();
      for(long i=0;i<nbPoint;i++) {*p++ = var;var +=step;}
   }
   mClockPowderPatternVarianceCalc.Click();
   #endif
   mClockPowderPatternCalc.Click();
   VFN_DEBUG_MESSAGE("PowderPatternBackground::CalcPowderPattern():End",3);
}

void PowderPatternBackground::CalcPowderPatternIntegrated() const
{
   if(mClockPowderPatternCalc>mClockMaster) return;

   this->CalcPowderPattern();// :TODO: Optimize
   if(  (mClockPowderPatternIntegratedCalc>mClockPowderPatternCalc)
      &&(mClockPowderPatternIntegratedCalc>mpParentPowderPattern->GetIntegratedProfileLimitsClock()))
         return;

   VFN_DEBUG_ENTRY("PowderPatternBackground::CalcPowderPatternIntegrated()",3)
   TAU_PROFILE("PowderPatternBackground::CalcPowderPatternIntegrated()","void ()",TAU_DEFAULT);
   const CrystVector_long *pMin=&(mpParentPowderPattern->GetIntegratedProfileMin());
   const CrystVector_long *pMax=&(mpParentPowderPattern->GetIntegratedProfileMax());

   const long numInterval=pMin->numElements();
   mPowderPatternIntegratedCalc.resize(numInterval);
   REAL *p2=mPowderPatternIntegratedCalc.data();
   for(int j=0;j<numInterval;j++)
   {
      const long max=(*pMax)(j);
      const REAL *p1=mPowderPatternCalc.data()+(*pMin)(j);
      *p2=0;           
      for(int k=(*pMin)(j);k<=max;k++) *p2 += *p1++;
      p2++;
   }
   #ifdef USE_BACKGROUND_MAXLIKE_ERROR
   mPowderPatternIntegratedCalcVariance.resize(numInterval);
   p2=mPowderPatternIntegratedCalcVariance.data();
   for(int j=0;j<numInterval;j++)
   {
      const long max=(*pMax)(j);
      const REAL *p1=mPowderPatternCalcVariance.data()+(*pMin)(j);
      *p2=0;           
      for(int k=(*pMin)(j);k<=max;k++) *p2 += *p1++;
      p2++;
   }
   mClockPowderPatternIntegratedVarianceCalc.Click();
   #endif
   mClockPowderPatternIntegratedCalc.Click();
   VFN_DEBUG_EXIT("PowderPatternBackground::CalcPowderPatternIntegrated()",3)
}

void PowderPatternBackground::Prepare()
{
}
void PowderPatternBackground::GetBraggLimits(CrystVector_long *&min,CrystVector_long *&max)const
{
   min=0;
   max=0;
}

void PowderPatternBackground::SetMaxSinThetaOvLambda(const REAL max)
{
   mMaxSinThetaOvLambda=max;
   mClockMaster.Click();
}

void PowderPatternBackground::InitRefParList()
{
   this->ResetParList();
   REAL *p=mBackgroundInterpPointIntensity.data();
   char buf [10];
   string str="Background_Point_";
   //for(int i=0;i<3;i++)
   for(int i=0;i<mBackgroundNbPoint;i++)
   {
      sprintf(buf,"%d",i);
      RefinablePar tmp(str+(string)buf,p++,
                        0.,1000.,gpRefParTypeScattDataBackground,REFPAR_DERIV_STEP_RELATIVE,
                        false,true,true,false,1.);
      tmp.SetGlobalOptimStep(10.);
      tmp.AssignClock(mClockBackgroundPoint);
      tmp.SetDerivStep(1e-3);
      this->AddPar(tmp);
   }
   #ifdef USE_BACKGROUND_MAXLIKE_ERROR
   {
      RefinablePar tmp("ML Model Error",&mModelVariance,
                        0.,100000.,gpRefParTypeObjCryst,REFPAR_DERIV_STEP_RELATIVE,
                        true,true,true,false,1.);
      tmp.AssignClock(mClockBackgroundPoint);
      tmp.SetDerivStep(1e-3);
      //tmp.SetGlobalOptimStep(10.);
      tmp.SetGlobalOptimStep(sqrt(mBackgroundInterpPointIntensity.sum()
                             /mBackgroundInterpPointIntensity.numElements()));
      this->AddPar(tmp);
   }
   #endif
}

void PowderPatternBackground::InitOptions()
{
   VFN_DEBUG_MESSAGE("PowderPatternBackground::InitOptions()",5)
   static string InterpolationModelName;
   static string InterpolationModelChoices[2];
   
   static bool needInitNames=true;
   if(true==needInitNames)
   {
      InterpolationModelName="Interpolation Model";
      InterpolationModelChoices[0]="Linear";
      InterpolationModelChoices[1]="Spline";
      //InterpolationModelChoices[2]="Chebyshev";
      
      needInitNames=false;//Only once for the class
   }
   mInterpolationModel.Init(2,&InterpolationModelName,InterpolationModelChoices);
   this->AddOption(&mInterpolationModel);
   mClockMaster.AddChild(mInterpolationModel.GetClock());
   mInterpolationModel.SetChoice(1);
}
#ifdef __WX__CRYST__
WXCrystObjBasic* PowderPatternBackground::WXCreate(wxWindow* parent)
{
   //:TODO: Check mpWXCrystObj==0
   mpWXCrystObj=new WXPowderPatternBackground(parent,this);
   return mpWXCrystObj;
}
#endif
////////////////////////////////////////////////////////////////////////
//
//        PowderPatternDiffraction
//
////////////////////////////////////////////////////////////////////////
PowderPatternDiffraction::PowderPatternDiffraction():
mFullProfileWidthFactor(5.),
mCagliotiU(0),mCagliotiV(0),mCagliotiW(3e-5),
mPseudoVoigtEta0(0.5),mPseudoVoigtEta1(0.),mUseAsymmetricProfile(false),
mCorrLorentz(*this),mCorrPolar(*this),mCorrSlitAperture(*this),
mCorrTextureMarchDollase(*this)
{
   VFN_DEBUG_MESSAGE("PowderPatternDiffraction::PowderPatternDiffraction()",5)
   mIsScalable=true;
   this->InitOptions();
   mReflectionProfileType.SetChoice(PROFILE_PSEUDO_VOIGT);
   this->SetIsIgnoringImagScattFact(true);
   this->AddSubRefObj(mCorrTextureMarchDollase);
   mClockMaster.AddChild(mClockProfilePar);
   mClockMaster.AddChild(mClockLorentzPolarSlitCorrPar);
}

PowderPatternDiffraction::PowderPatternDiffraction(const PowderPatternDiffraction &old):
mReflectionProfileType(old.mReflectionProfileType),
mFullProfileWidthFactor(old.mFullProfileWidthFactor),
mCagliotiU(old.mCagliotiU),mCagliotiV(old.mCagliotiV),mCagliotiW(old.mCagliotiW),
mPseudoVoigtEta0(old.mPseudoVoigtEta0),mPseudoVoigtEta1(old.mPseudoVoigtEta1),
mUseAsymmetricProfile(old.mUseAsymmetricProfile),
mCorrLorentz(*this),mCorrPolar(*this),mCorrSlitAperture(*this),
mCorrTextureMarchDollase(*this)
{
   this->AddSubRefObj(mCorrTextureMarchDollase);
   mClockMaster.AddChild(mClockProfilePar);
   mClockMaster.AddChild(mClockLorentzPolarSlitCorrPar);
}

PowderPatternDiffraction::~PowderPatternDiffraction()
{}
const string& PowderPatternDiffraction::GetClassName() const
{
   const static string className="PowderPatternDiffraction";
   return className;
}

PowderPatternDiffraction* PowderPatternDiffraction::CreateCopy()const
{
   return new PowderPatternDiffraction(*this);
}

void PowderPatternDiffraction::SetParentPowderPattern(const PowderPattern &s)
{
   if(mpParentPowderPattern!=0) 
      mClockMaster.RemoveChild(mpParentPowderPattern->GetIntegratedProfileLimitsClock());
   mpParentPowderPattern = &s;
   mClockMaster.AddChild(mpParentPowderPattern->GetIntegratedProfileLimitsClock());
   mClockMaster.AddChild(mpParentPowderPattern->GetClockPowderPatternPar());
   mClockMaster.AddChild(mpParentPowderPattern->GetClockPowderPattern2ThetaCorr());
   mClockMaster.AddChild(mpParentPowderPattern->GetClockPowderPatternRadiation());
}

const CrystVector_REAL& PowderPatternDiffraction::GetPowderPatternCalc()const
{
   this->CalcPowderPattern();
   return mPowderPatternCalc;
}

pair<const CrystVector_REAL*,const RefinableObjClock*>
   PowderPatternDiffraction::GetPowderPatternIntegratedCalc()const
{
   this->CalcPowderPatternIntegrated();
   return make_pair(&mPowderPatternIntegratedCalc,&mClockPowderPatternIntegratedCalc);
}

void PowderPatternDiffraction::SetReflectionProfilePar(const ReflectionProfileType prof,
                          const REAL fwhmCagliotiW,
                          const REAL fwhmCagliotiU,
                          const REAL fwhmCagliotiV,
                          const REAL eta0,
                          const REAL eta1)
{
   VFN_DEBUG_MESSAGE("PowderPatternDiffraction::SetReflectionProfilePar()",5)
   mReflectionProfileType.SetChoice(prof);
   mCagliotiU=fwhmCagliotiU;
   mCagliotiV=fwhmCagliotiV;
   mCagliotiW=fwhmCagliotiW;
   mPseudoVoigtEta0=eta0;
   mPseudoVoigtEta1=eta1;
   mClockProfilePar.Click();
}

void PowderPatternDiffraction::GenHKLFullSpace()
{
   VFN_DEBUG_MESSAGE("PowderPatternDiffraction::GenHKLFullSpace():",3)
   this->ScatteringData::GenHKLFullSpace(mpParentPowderPattern->Get2ThetaMax()/2,true);
}
void PowderPatternDiffraction::BeginOptimization(const bool allowApproximations,
                                                 const bool enableRestraints)
{
   if(mUseFastLessPreciseFunc!=allowApproximations)
   {
      mClockProfileCalc.Reset();
   }
   this->ScatteringData::BeginOptimization(allowApproximations,enableRestraints);
}
void PowderPatternDiffraction::EndOptimization()
{
   if(mUseFastLessPreciseFunc==true)
   {
      mClockProfileCalc.Reset();
   }
   this->ScatteringData::EndOptimization();
}
void PowderPatternDiffraction::GetGeneGroup(const RefinableObj &obj,
                                CrystVector_uint & groupIndex,
                                unsigned int &first) const
{
   // One group for all profile parameters
   unsigned int index=0;
   VFN_DEBUG_MESSAGE("PowderPatternDiffraction::GetGeneGroup()",4)
   for(long i=0;i<obj.GetNbPar();i++)
      for(long j=0;j<this->GetNbPar();j++)
         if(&(obj.GetPar(i)) == &(this->GetPar(j)))
         {
            //if(this->GetPar(j).GetType()->IsDescendantFromOrSameAs())
            //{
               if(index==0) index=first++;
               groupIndex(i)=index;
            //}
            //else //no parameters other than unit cell
         }
}

const CrystVector_REAL& PowderPatternDiffraction::GetPowderPatternCalcVariance()const
{
   
   return mPowderPatternCalcVariance;
}

pair<const CrystVector_REAL*,const RefinableObjClock*>
   PowderPatternDiffraction::GetPowderPatternIntegratedCalcVariance()const
{
   this->CalcPowderPatternIntegrated();
   return make_pair(&mPowderPatternIntegratedCalcVariance,
                    &mClockPowderPatternIntegratedVarianceCalc);
}

bool PowderPatternDiffraction::HasPowderPatternCalcVariance()const
{
   return true;
}

const Radiation& PowderPatternDiffraction::GetRadiation()const
{ return mpParentPowderPattern->GetRadiation();}

void PowderPatternDiffraction::CalcPowderPattern() const
{
   this->GetNbReflBelowMaxSinThetaOvLambda();
   if(mClockPowderPatternCalc>mClockMaster) return;
   TAU_PROFILE("PowderPatternDiffraction::CalcPowderPattern()-Apply profiles","void (bool)",TAU_DEFAULT);
   
   VFN_DEBUG_MESSAGE("PowderPatternDiffraction::CalcPowderPattern():",3)

   // :TODO: Can't do this as this is non-const
   //if(this->GetCrystal().GetSpaceGroup().GetClockSpaceGroup()>mClockHKL)
   //   this->GenHKLFullSpace();
   //
   // The workaround is to call Prepare() (non-const) before every calculation
   // when a modifictaion may have occured.

   this->CalcIhkl();
   this->CalcPowderReflProfile();
   
   if(  (mClockPowderPatternCalc>mClockIhklCalc)
      &&(mClockPowderPatternCalc>mClockProfileCalc)) return;
   
   if(true) //:TODO: false == mUseFastLessPreciseFunc
   {
      REAL theta;
      long thetaPt,first,last,shift;
      const long nbPoints=2*mSavedPowderReflProfileNbPoint+1;
      long step;
      const long nbRefl=this->GetNbRefl();
      VFN_DEBUG_MESSAGE("PowderPatternDiffraction::CalcPowderPattern\
Applying profiles for "<<nbRefl<<" reflections",3)

      const long  specNbPoints=mpParentPowderPattern->GetNbPoint();
      mPowderPatternCalc.resize(specNbPoints);
      mPowderPatternCalc=0;
      const bool useML= (mIhklCalcVariance.numElements() != 0);
      if(useML)
      {
         mPowderPatternCalcVariance.resize(specNbPoints);
         mPowderPatternCalcVariance=0;
      }
      else mPowderPatternCalcVariance.resize(0);
      VFN_DEBUG_MESSAGE("PowderPatternDiffraction::CalcPowderPattern() Has variance:"<<useML,2)

      for(long i=0;i<mNbReflUsed;i += step)
      {
         REAL intensity=0.;
         REAL var=0.;
         //check if the next reflection is at the same theta. If this is true,
         //Then assume that the profile is exactly the same.
         for(step=0; ;)
         {
            intensity += mIhklCalc(i + step);
            if(useML) var += mIhklCalcVariance(i + step);
            step++;
            if( (i+step) >= nbRefl) break;
            if(mTheta(i+step) > (mTheta(i)+1e-5) ) break;
         }
         theta =  mpParentPowderPattern->Get2ThetaCorr(2*mTheta(i));
         thetaPt= mpParentPowderPattern->Get2ThetaCorrPixel(2*mTheta(i));

         first=thetaPt-mSavedPowderReflProfileNbPoint;

         if( first >= specNbPoints) continue;
         if( first < 0)
         {
            shift = -first;
            first =0;
         } else shift =0;
         last=thetaPt+mSavedPowderReflProfileNbPoint;
         if( last >= specNbPoints) last=specNbPoints-1;
         
         VFN_DEBUG_MESSAGE("Apply profile(Monochromatic)Refl("<<i<<")"\
            <<mIntH(i)<<" "<<mIntK(i)<<" "<<mIntL(i)<<" "\
            <<"  I="<<intensity<<"  2Theta="<<2*mTheta(i)*RAD2DEG\
            <<",pixel #"<<first<<"->"<<last,2)
         
         {
            const REAL *p2 = mSavedPowderReflProfile.data() + i*nbPoints +shift;
            REAL *p3 = mPowderPatternCalc.data()+first;
            for(long j=first;j<=last;j++) *p3++ += *p2++ * intensity;
         }
         if(useML)
         {
            const REAL *p2 = mSavedPowderReflProfile.data() + i*nbPoints +shift;
            REAL *p3 = mPowderPatternCalcVariance.data()+first;
            for(long j=first;j<=last;j++) *p3++ += *p2++ * var;
         }
      }
   }
   else
   {
      //:TODO:
      throw ObjCrystException("PowderPatternDiffraction::CalcPowderPattern() : \
         FAST option not yet implemented !");
   }
   mClockPowderPatternCalc.Click();
   VFN_DEBUG_MESSAGE("PowderPatternDiffraction::CalcPowderPattern: End.",3)
}

void PowderPatternDiffraction::CalcPowderPatternIntegrated() const
{
   this->GetNbReflBelowMaxSinThetaOvLambda();
   if(mClockPowderPatternIntegratedCalc>mClockMaster) return;
   TAU_PROFILE("PowderPatternDiffraction::CalcPowderPatternIntegrated()","void (bool)",TAU_DEFAULT);

   this->CalcIhkl();
   this->PrepareIntegratedProfile();
   
   if(  (mClockPowderPatternIntegratedCalc>mClockIhklCalc)
      &&(mClockPowderPatternIntegratedCalc>mClockIntegratedProfileFactor)
      &&(mClockPowderPatternIntegratedCalc>mpParentPowderPattern->GetIntegratedProfileLimitsClock()))
            return;
   VFN_DEBUG_ENTRY("PowderPatternDiffraction::CalcPowderPatternIntegrated()",3)
   const long nbRefl=this->GetNbRefl();

   const long  nb=mpParentPowderPattern->GetIntegratedProfileMin().numElements();
   mPowderPatternIntegratedCalc.resize(nb);
   mPowderPatternIntegratedCalc=0;
   const bool useML= (mIhklCalcVariance.numElements() != 0);
   if(useML)
   {
      mPowderPatternIntegratedCalcVariance.resize(nb);
      mPowderPatternIntegratedCalcVariance=0;
   }
   else mPowderPatternIntegratedCalcVariance.resize(0);
   const REAL *pth=mTheta.data();
   const REAL *pI=mIhklCalc.data();
   const REAL *pIvar=mIhklCalcVariance.data();
   vector< pair<unsigned long, CrystVector_REAL> >::const_iterator pos;
   pos=mIntegratedProfileFactor.begin();
   for(long i=0;i<mNbReflUsed;)
   {
      //VFN_DEBUG_MESSAGE("PowderPatternDiffraction::CalcPowderPatternIntegrated():"<<i,10)
      REAL intensity=0.;
      REAL var=0.;
      const REAL thmax=*pth+1e-5;
      //check if the next reflection is at the same theta. If this is true,
      //Then assume that the profile is exactly the same.
      for(;;)
      {
         intensity += *pI++;
         if(useML) var += *pIvar++;
         ++pos;
         if( ++i >= nbRefl) break;
         if( *(++pth) > thmax ) break;
      }
      --pos;
      //VFN_DEBUG_MESSAGE("PowderPatternDiffraction::CalcPowderPatternIntegrated():"<<i,10)
      REAL *pData=mPowderPatternIntegratedCalc.data()+pos->first;
      const REAL *pFact=pos->second.data();
      const unsigned long nb=pos->second.numElements();
      //cout <<i<<" - "<< intensity<<"*:";
      for(unsigned long j=0;j<nb;j++)
      {
         //cout <<pos->first+j<<"("<<*pFact<<","<<*pData<<") ";
         *pData++ += intensity * *pFact++ ;
      }
      //cout<<endl;
      
      if(useML)
      {
         const REAL *pFact=pos->second.data();
         REAL *pVar=mPowderPatternIntegratedCalcVariance.data()+pos->first;
         for(unsigned long j=0;j<nb;j++) *pVar++ += var * *pFact++ ;
      }
      ++pos;
   }
   #ifdef __DEBUG__
   if(gVFNDebugMessageLevel<3)
   {
      this->CalcPowderPattern();
      CrystVector_REAL integr(nb),min(nb),max(nb),diff(nb),index(nb);
      integr=0;
      for(long i=0;i<nb;i++)
      {
         index(i)=i;
         min(i)=mpParentPowderPattern->GetIntegratedProfileMin()(i);
         max(i)=mpParentPowderPattern->GetIntegratedProfileMax()(i);
         integr(i)=0;
         for(long j=mpParentPowderPattern->GetIntegratedProfileMin()(i);
                 j<=mpParentPowderPattern->GetIntegratedProfileMax()(i);j++)
         {
            integr(i) += mPowderPatternCalc(j);
         }
         diff(i)=1.-mPowderPatternIntegratedCalc(i)/integr(i);
      }
      cout << "Integrated intensities, Component"<<endl
           << FormatVertVectorHKLFloats<REAL>
                (index,min,max,integr,mPowderPatternIntegratedCalc,diff,20,6)<<endl;
   }
   #endif
   mClockPowderPatternIntegratedCalc.Click();
   VFN_DEBUG_EXIT("PowderPatternDiffraction::CalcPowderPatternIntegrated",3)
}

void PowderPatternDiffraction::CalcPowderReflProfile()const
{
   this->CalcSinThetaLambda();
   //mClockProfileCalc.Print();
   //this->GetRadiation().GetClockWavelength().Print();
   //this->GetRadiation().GetClockRadiation().Print();
   if(  (mClockProfileCalc>mClockProfilePar)
      &&(mClockProfileCalc>mReflectionProfileType.GetClock())
      &&(mClockProfileCalc>mClockTheta)
      &&(mClockProfileCalc>this->GetRadiation().GetClockWavelength())
      &&(mClockProfileCalc>mpParentPowderPattern->GetClockPowderPattern2ThetaCorr())
      &&(mClockProfileCalc>mClockHKL)) return;
   
   TAU_PROFILE("PowderPatternDiffraction::CalcPowderReflProfile()","void (bool)",TAU_DEFAULT);
   VFN_DEBUG_ENTRY("PowderPatternDiffraction::CalcPowderReflProfile()",5)

   const REAL specMin     =mpParentPowderPattern->Get2ThetaMin();
   //const REAL specMax     =mpParentPowderPattern->Get2ThetaMax();
   const REAL specStep    =mpParentPowderPattern->Get2ThetaStep();
   const long  specNbPoints=mpParentPowderPattern->GetNbPoint();
   
   long thetaPt;
   REAL fwhm,tmp;
   REAL *p1,*p2;
   CrystVector_REAL ttheta,tmp2theta,reflProfile,tmpV;
   //Width of calc profiles
   VFN_DEBUG_MESSAGE("PowderPatternDiffraction::CalcPowderReflProfile():\
Computing Widths",5)
   {
      tmp = specMin+specStep*specNbPoints;
      tmp/= 2;
      fwhm=      mCagliotiW + mCagliotiV*tmp + mCagliotiU*tmp*tmp;
      if(fwhm<1e-10) fwhm=1e-10;
      fwhm=sqrt(fwhm);
      if(true==mUseFastLessPreciseFunc)
         mSavedPowderReflProfileNbPoint =(long)(mFullProfileWidthFactor
                                             *fwhm/specStep/2);
      else
         mSavedPowderReflProfileNbPoint =(long)(mFullProfileWidthFactor
                                       *fwhm/specStep);
      switch(this->GetRadiation().GetWavelengthType())
      {
         case WAVELENGTH_MONOCHROMATIC:break;
         case WAVELENGTH_ALPHA12:
         {
            mSavedPowderReflProfileNbPoint
               +=(long)( tan(mpParentPowderPattern->Get2ThetaMax())
                        *this->GetRadiation().GetXRayTubeDeltaLambda()
                        /this->GetRadiation().GetWavelength()(0));
            break;
         }
         default: throw ObjCrystException("PowderPatternDiffraction::PrepareIntegratedProfile():\
Radiation must be either monochromatic or from an X-Ray Tube !!");
      }
      VFN_DEBUG_MESSAGE("PowderPatternDiffraction::CalcPowderReflProfile():"<<\
      "Profiles half-width="<<mFullProfileWidthFactor*fwhm*RAD2DEG<<" ("<<\
      mSavedPowderReflProfileNbPoint<<" points)",5)
   }
   //Prepare arrays
      ttheta.resize(2*mSavedPowderReflProfileNbPoint+1);
      reflProfile.resize(2*mSavedPowderReflProfileNbPoint+1);
      p2=ttheta.data();
      for(REAL i=-mSavedPowderReflProfileNbPoint;i<=mSavedPowderReflProfileNbPoint;i++)
         *p2++ = i * specStep;
      mSavedPowderReflProfile.resize(this->GetNbRefl(),2*mSavedPowderReflProfileNbPoint+1);
   //Calc all profiles
   mvLabel.clear();
   stringstream label;

   unsigned int nbLine=1;
   CrystVector_REAL spectrumDeltaLambdaOvLambda;
   CrystVector_REAL spectrumFactor;//relative weigths of different lines of X-Ray tube
   switch(this->GetRadiation().GetWavelengthType())
   {
      case WAVELENGTH_MONOCHROMATIC:
      {
         spectrumDeltaLambdaOvLambda.resize(1);spectrumDeltaLambdaOvLambda=0.0;
         spectrumFactor.resize(1);spectrumFactor=1.0;
         break;
      }
      case WAVELENGTH_ALPHA12:
      {
         nbLine=2;
         spectrumDeltaLambdaOvLambda.resize(2);
         spectrumDeltaLambdaOvLambda(0)
            =-this->GetRadiation().GetXRayTubeDeltaLambda()
             *this->GetRadiation().GetXRayTubeAlpha2Alpha1Ratio()
             /(1+this->GetRadiation().GetXRayTubeAlpha2Alpha1Ratio())
             /this->GetRadiation().GetWavelength()(0);
         spectrumDeltaLambdaOvLambda(1)
            = this->GetRadiation().GetXRayTubeDeltaLambda()
             /(1+this->GetRadiation().GetXRayTubeAlpha2Alpha1Ratio())
             /this->GetRadiation().GetWavelength()(0);
         
         spectrumFactor.resize(2);
         spectrumFactor(0)=1./(1.+this->GetRadiation().GetXRayTubeAlpha2Alpha1Ratio());
         spectrumFactor(1)=this->GetRadiation().GetXRayTubeAlpha2Alpha1Ratio()
                           /(1.+this->GetRadiation().GetXRayTubeAlpha2Alpha1Ratio());
         break;
      }
      default: throw ObjCrystException("PowderPatternDiffraction::PrepareIntegratedProfile():\
Radiation must be either monochromatic or from an X-Ray Tube !!");
   }
   
   
   VFN_DEBUG_MESSAGE("PowderPatternDiffraction::CalcPowderReflProfile():\
Computing all Profiles",5)
   for(unsigned int line=0;line<nbLine;line++)
   {
      p1=mSavedPowderReflProfile.data();
      for(long i=0;i<this->GetNbRefl();i++)
      {
         VFN_DEBUG_MESSAGE("PowderPatternDiffraction::CalcPowderReflProfile():\
Computing all Profiles: Reflection #"<<i,5)
         tmp=mTheta(i);
         fwhm=mCagliotiW + mCagliotiV*tmp + mCagliotiU*tmp*tmp;
         if(fwhm<1e-10) fwhm=1e-10;
         fwhm =sqrt(fwhm);
         tmp=mpParentPowderPattern->Get2ThetaCorr(2*tmp);

         REAL powderAsym=1.;
         //if(true == mUseAsymmetricProfile) 
         //   powderAsym=mPowderAsymA0+mPowderAsymA1/sin(tmp)+mPowderAsymA2/sin(tmp)/sin(tmp);

         //:KLUDGE: Need to use the same 'thetaPt' when calculating the complete powder spectrum
         thetaPt =(long) ((tmp-specMin)/specStep);
         tmp2theta = ttheta;
         if(nbLine>1)
         {// we have several lines, not centered on the profile range
            tmp=mTheta(i);
            tmp = mpParentPowderPattern->Get2ThetaCorr(
                        2*(tmp+tan(tmp)*spectrumDeltaLambdaOvLambda(line)));
         }
         tmp2theta += (specMin + thetaPt * specStep)-tmp;
         if(line==0)
         {// For an X-Ray tube, label on first (strongest) of reflections lines (Kalpha1)
            label.str("");
            label<<mIntH(i)<<" "<<mIntK(i)<<" "<<mIntL(i);
            mvLabel.push_back(make_pair(tmp,label.str()));
         }

         switch(mReflectionProfileType.GetChoice())
         {
            case PROFILE_GAUSSIAN:
            {
               if(true == mUseAsymmetricProfile)
                  reflProfile=PowderProfileGauss(tmp2theta,fwhm,powderAsym);
               else reflProfile=PowderProfileGauss(tmp2theta,fwhm);
               break;
            }
            case PROFILE_LORENTZIAN:
            {
               if(true == mUseAsymmetricProfile)
                  reflProfile=PowderProfileLorentz(tmp2theta,fwhm,powderAsym);
               else reflProfile=PowderProfileLorentz(tmp2theta,fwhm);
               break;
            }
            case PROFILE_PSEUDO_VOIGT:
            {
               if(true == mUseAsymmetricProfile)
                  reflProfile=PowderProfileGauss(tmp2theta,fwhm,powderAsym);
               else reflProfile=PowderProfileGauss(tmp2theta,fwhm);
               reflProfile *= 1-(mPseudoVoigtEta0+2*mTheta(i)*mPseudoVoigtEta1);
               if(true == mUseAsymmetricProfile)
                  tmpV=PowderProfileLorentz(tmp2theta,fwhm,powderAsym);
               else tmpV=PowderProfileLorentz(tmp2theta,fwhm);
               tmpV *= mPseudoVoigtEta0+2*mTheta(i)*mPseudoVoigtEta1;
               reflProfile += tmpV;
               break;
            }
            case PROFILE_PSEUDO_VOIGT_FINGER_COX_JEPHCOAT:
            {
               throw ObjCrystException(
                  "PROFILE_PSEUDO_VOIGT_FINGER_COX_JEPHCOAT Not implemented");
               /*
               tmp2theta*=180./M_PI;//Keep degrees:external library used
               fwhm *=180./M_PI;
               REAL eta=mPowderPseudoVoigtEta0+2*mTheta(i)*mPowderPseudoVoigtEta1;
               REAL dPRdT, dPRdG, dPRdE , dPRdS, dPRdD;
               bool useAsym=true;
               if( (mPowderAsymSourceWidth < 1e-8) && (mPowderAsymSourceWidth < 1e-8) )
                  useAsym=false;

               for(long j=0;j<2*mSavedPowderReflProfileNbPoint+1;j++)
                  reflProfile(j)=Profval( eta,fwhm,mPowderAsymSourceWidth,
                                          mPowderAsymDetectorWidth,tmp2theta(j) ,
                                          0.,&dPRdT,&dPRdG,&dPRdE,&dPRdS,&dPRdD,
                                          useAsym);
               break;
               */
            }
            case PROFILE_PEARSON_VII: throw ObjCrystException("PEARSON_VII Not implemented yet");

         }
         if(nbLine>1) reflProfile *=spectrumFactor(line);
         p2=reflProfile.data();
         if(line==0)for(int j=0;j<2*mSavedPowderReflProfileNbPoint+1;j++) *p1++  = *p2++;
         else       for(int j=0;j<2*mSavedPowderReflProfileNbPoint+1;j++) *p1++ += *p2++;
      }
   }
   mClockProfileCalc.Click();
   VFN_DEBUG_EXIT("PowderPatternDiffraction::CalcPowderReflProfile()",5)
}

void PowderPatternDiffraction::CalcIntensityCorr()const
{
   bool needRecalc=false;
   
   this->CalcSinThetaLambda();
   if(mClockIntensityCorr<mClockTheta) needRecalc=true;
   
   const CrystVector_REAL *mpCorr[4];
   
   mpCorr[0]=&(mCorrLorentz.GetCorr());
   if(mClockIntensityCorr<mCorrLorentz.GetClockCorr()) needRecalc=true;
   
   if(this->GetRadiation().GetRadiationType()==RAD_XRAY)
   {
      mpCorr[1]=&(mCorrPolar.GetCorr());
      if(mClockIntensityCorr<mCorrPolar.GetClockCorr()) needRecalc=true;
   }
   
   mpCorr[2]=&(mCorrSlitAperture.GetCorr());
   if(mClockIntensityCorr<mCorrSlitAperture.GetClockCorr()) needRecalc=true;
   
   if(mCorrTextureMarchDollase.GetNbPhase()>0)
   {
      mpCorr[3]=&(mCorrTextureMarchDollase.GetCorr());
      if(mClockIntensityCorr<mCorrTextureMarchDollase.GetClockCorr()) needRecalc=true;
   }
   
   if(needRecalc==false) return;
   
   TAU_PROFILE("PowderPatternDiffraction::CalcIntensityCorr()","void ()",TAU_DEFAULT);
   VFN_DEBUG_MESSAGE("PowderPatternDiffraction::CalcIntensityCorr()",2)
   mIntensityCorr = *(mpCorr[0]);
   if(this->GetRadiation().GetRadiationType()==RAD_XRAY) mIntensityCorr *= *(mpCorr[1]);
   mIntensityCorr *= *(mpCorr[2]);
   if(mCorrTextureMarchDollase.GetNbPhase()>0) mIntensityCorr *= *mpCorr[3];
   mClockIntensityCorr.Click();
   VFN_DEBUG_MESSAGE("PowderPatternDiffraction::CalcIntensityCorr():finished",2)
}

void PowderPatternDiffraction::CalcIhkl() const
{
   this->CalcStructFactor();
   this->CalcIntensityCorr();
   if(  (mClockIhklCalc>mClockIntensityCorr)
      &&(mClockIhklCalc>mClockStructFactor)
      &&(mClockIhklCalc>mClockNbReflUsed)) return;
      
   VFN_DEBUG_MESSAGE("PowderPatternDiffraction::CalcIhkl()",3)
   TAU_PROFILE("PowderPatternDiffraction::CalcIhkl()","void ()",TAU_DEFAULT);
   const REAL *pr,*pi,*pcorr;
   const int *mult;
   REAL *p;
   
   pr=mFhklCalcReal.data();
   pi=mFhklCalcImag.data();
   pcorr=mIntensityCorr.data();
   
   mult=mMultiplicity.data();
   mIhklCalc.resize(mNbRefl);
   p=mIhklCalc.data();
   if(mFhklCalcVariance.numElements()>0)
   {
      const REAL *pv=mFhklCalcVariance.data();
      for(long i=0;i<mNbReflUsed;i++)
      {
         *p++ = *mult++ * (*pr * *pr + *pi * *pi + 2 * *pv++) * *pcorr++;
         pr++;
         pi++;
      }
   }
   else
   {
      for(long i=0;i<mNbReflUsed;i++)
      {
         *p++ = *mult++ * (*pr * *pr + *pi * *pi) * *pcorr++;
         pr++;
         pi++;
      }
   }
   if(mFhklCalcVariance.numElements()==0)
   {
      VFN_DEBUG_MESSAGE("PowderPatternDiffraction::CalcIhkl(): No Calc Variance",2)
      mIhklCalcVariance.resize(0);
      VFN_DEBUG_MESSAGE(endl<<
                     FormatVertVectorHKLFloats<REAL>(mH,mK,mL,mSinThetaLambda,
                                                      mFhklCalcReal,
                                                      mFhklCalcImag,
                                                      mIhklCalc,
                                                      mIntensityCorr
                                                      ),2)
   }
   else
   {
      VFN_DEBUG_MESSAGE("PowderPatternDiffraction::CalcIhkl(): Calc Variance",2)
      mIhklCalcVariance.resize(mNbRefl);
      REAL *pVar2=mIhklCalcVariance.data();

      const REAL *pInt=mIhklCalc.data();
      const REAL *pVar=mFhklCalcVariance.data();
      pcorr=mIntensityCorr.data();
      mult=mMultiplicity.data();
      
      pVar2=mIhklCalcVariance.data();
      
      for(long j=0;j<mNbReflUsed;j++)
      {
         *pVar2++ = (4* *mult) * *pcorr * *pVar *(*pInt++ - (*mult * *pcorr) * *pVar);
         pVar++;mult++;pcorr++;
      }
      VFN_DEBUG_MESSAGE(endl<<
                     FormatVertVectorHKLFloats<REAL>(mH,mK,mL,mSinThetaLambda,
                                                   mFhklCalcReal,       
                                                   mIhklCalc,           
                                                   mExpectedIntensityFactor,
                                                   mIntensityCorr,
                                                   mMultiplicity,
                                                   mvLuzzatiFactor[&(mpCrystal->GetScatteringPowerRegistry().GetObj(0))],  
                                                   mFhklCalcVariance,   
                                                   mIhklCalcVariance),2);
      VFN_DEBUG_MESSAGE(mNbRefl<<" "<<mNbReflUsed,2)
   }

   //cout <<FormatVertVector<REAL>(mTheta,mIhklCalc,mMultiplicity,
   //                               mFhklCalcReal,mFhklCalcImag,mIntensityCorr);
   mClockIhklCalc.Click();
   VFN_DEBUG_MESSAGE("PowderPatternDiffraction::CalcIhkl():End",3)
}

void PowderPatternDiffraction::Prepare()
{
   if(  (this->GetCrystal().GetSpaceGroup().GetClockSpaceGroup()>mClockHKL)
      ||(this->GetCrystal().GetClockLatticePar()>mClockHKL)
      ||(this->GetRadiation().GetClockWavelength()>mClockHKL)
      ||(mpParentPowderPattern->GetClockPowderPatternPar()>mClockHKL))
         this->GenHKLFullSpace();
   //if(0==this->GetNbRefl()) this->GenHKLFullSpace();
}
void PowderPatternDiffraction::InitOptions()
{
   VFN_DEBUG_MESSAGE("PowderPatternDiffraction::InitOptions()",5)
   static string ReflectionProfileTypeName;
   static string ReflectionProfileTypeChoices[3];
   
   static bool needInitNames=true;
   if(true==needInitNames)
   {
      ReflectionProfileTypeName="Profile Type";
      ReflectionProfileTypeChoices[0]="Gaussian";
      ReflectionProfileTypeChoices[1]="Lorentzian";
      ReflectionProfileTypeChoices[2]="Pseudo-Voigt";
      
      needInitNames=false;//Only once for the class
   }
   mReflectionProfileType.Init(3,&ReflectionProfileTypeName,ReflectionProfileTypeChoices);
   this->AddOption(&mReflectionProfileType);
   
   //Init parameters. This should not be done here !!!
   {
      RefinablePar tmp("U",&mCagliotiU,-1/RAD2DEG/RAD2DEG,1./RAD2DEG/RAD2DEG,
                        gpRefParTypeScattDataProfileWidth,
                        REFPAR_DERIV_STEP_ABSOLUTE,true,true,true,false,RAD2DEG*RAD2DEG);
      tmp.AssignClock(mClockProfilePar);
      tmp.SetDerivStep(1e-7);
      this->AddPar(tmp);
   }
   {
      RefinablePar tmp("V",&mCagliotiV,-1/RAD2DEG/RAD2DEG,1./RAD2DEG/RAD2DEG,
                        gpRefParTypeScattDataProfileWidth,
                        REFPAR_DERIV_STEP_ABSOLUTE,true,true,true,false,RAD2DEG*RAD2DEG);
      tmp.AssignClock(mClockProfilePar);
      tmp.SetDerivStep(1e-7);
      this->AddPar(tmp);
   }
   {
      RefinablePar tmp("W",&mCagliotiW,0,1./RAD2DEG/RAD2DEG,
                        gpRefParTypeScattDataProfileWidth,
                        REFPAR_DERIV_STEP_ABSOLUTE,true,true,true,false,RAD2DEG*RAD2DEG);
      tmp.AssignClock(mClockProfilePar);
      tmp.SetDerivStep(1e-7);
      this->AddPar(tmp);
   }
   {
      RefinablePar tmp("Eta0",&mPseudoVoigtEta0,0,1.,gpRefParTypeScattDataProfileType,
                        REFPAR_DERIV_STEP_ABSOLUTE,true,true,true,false);
      tmp.AssignClock(mClockProfilePar);
      tmp.SetDerivStep(1e-4);
      this->AddPar(tmp);
   }
   {
      RefinablePar tmp("Eta1",&mPseudoVoigtEta1,-1,1.,gpRefParTypeScattDataProfileType,
                        REFPAR_DERIV_STEP_ABSOLUTE,true,true,true,false);
      tmp.AssignClock(mClockProfilePar);
      tmp.SetDerivStep(1e-4);
      this->AddPar(tmp);
   }
}
void PowderPatternDiffraction::GetBraggLimits(CrystVector_long *&min,CrystVector_long *&max)const
{
   VFN_DEBUG_ENTRY("PowderPatternDiffraction::GetBraggLimits(*min,*max)",3)
   this->CalcPowderReflProfile();
   if(mClockProfileCalc>mClockBraggLimits)
   {
      REAL fwhmRatio;//integrate from -fwhmRatio*fwhm to -fwhmRatio*fwhm
      switch(mReflectionProfileType.GetChoice())
      {
         case PROFILE_GAUSSIAN:fwhmRatio=1;
         case PROFILE_LORENTZIAN:fwhmRatio=2.;
         case PROFILE_PSEUDO_VOIGT:fwhmRatio=1.+mPseudoVoigtEta0;
      }
      cout <<" Integrated R/Rw-factors up to +/- "<<fwhmRatio<<"*FWHM"<<endl;
      TAU_PROFILE("PowderPatternDiffraction::GetBraggLimits()","void ()",TAU_DEFAULT);
      VFN_DEBUG_MESSAGE("PowderPatternDiffraction::GetBraggLimits(*min,*max):Recalc",3)
      mIntegratedReflMin.resize(this->GetNbRefl());
      mIntegratedReflMax.resize(this->GetNbRefl());
      REAL fwhm,tmp;
      for(long i=0;i<this->GetNbRefl();i++)
      {
         tmp=mTheta(i);
         fwhm=sqrt(mCagliotiW + mCagliotiV*tmp + mCagliotiU*tmp*tmp);
         mIntegratedReflMin(i)=mpParentPowderPattern->Get2ThetaCorrPixel(2*mTheta(i)-2*fwhm);
         mIntegratedReflMax(i)=mpParentPowderPattern->Get2ThetaCorrPixel(2*mTheta(i)+2*fwhm);
      }
      mClockBraggLimits.Click();
   }
   //cout << FormatVertVector<long>(mIntegratedReflMin,mIntegratedReflMax)<<endl;
   min=&mIntegratedReflMin;
   max=&mIntegratedReflMax;
   VFN_DEBUG_EXIT("PowderPatternDiffraction::GetBraggLimits(*min,*max)",3)
}

void PowderPatternDiffraction::SetMaxSinThetaOvLambda(const REAL max)
{this->ScatteringData::SetMaxSinThetaOvLambda(max);}

void PowderPatternDiffraction::PrepareIntegratedProfile()const
{
   this->CalcPowderReflProfile();
   
   if(  (mClockIntegratedProfileFactor>mClockProfileCalc)
      &&(mClockIntegratedProfileFactor>mpParentPowderPattern->GetIntegratedProfileLimitsClock())
      &&(mClockIntegratedProfileFactor>mClockNbReflUsed))
   return;
   VFN_DEBUG_ENTRY("PowderPatternDiffraction::PrepareIntegratedProfile()",5)
   TAU_PROFILE("PowderPatternDiffraction::PrepareIntegratedProfile()","void ()",TAU_DEFAULT);
   const CrystVector_long *pMin=&(mpParentPowderPattern->GetIntegratedProfileMin());
   const CrystVector_long *pMax=&(mpParentPowderPattern->GetIntegratedProfileMax());

   const long numInterval=pMin->numElements();
   const long nbPoints=2*mSavedPowderReflProfileNbPoint+1;
   
   vector< map<long, REAL> > vIntegratedProfileFactor;
   vIntegratedProfileFactor.resize(mNbReflUsed);
   vector< map<long, REAL> >::iterator pos1;
   pos1=vIntegratedProfileFactor.begin();
   
   mIntegratedProfileFactor.resize(mNbReflUsed);
   vector< pair<unsigned long, CrystVector_REAL> >::iterator pos2;
   pos2=mIntegratedProfileFactor.begin();
   for(long i=0;i<mNbReflUsed;i++)
   {
      pos1->clear();
      long firstInterval=numInterval;
      for(long j=0;j<numInterval;j++)
      {
         const long thetaPt= mpParentPowderPattern->Get2ThetaCorrPixel(2*mTheta(i));
         const long first0 = thetaPt - mSavedPowderReflProfileNbPoint;
         const long last0  = thetaPt + mSavedPowderReflProfileNbPoint;
               long first= first0>(*pMin)(j) ? first0:(*pMin)(j);
         const long last = last0 <(*pMax)(j) ? last0 :(*pMax)(j);
         if(first<=last)
         {
            if(firstInterval>j) firstInterval=j;
            if(pos1->find(j) == pos1->end()) (*pos1)[j]=0.;
            REAL *fact = &((*pos1)[j]);//this creates the 'j' entry if necessary
            const long shift=first-first0;
            const REAL *p2 = mSavedPowderReflProfile.data() + i*nbPoints +shift;
            for(int k=first;k<=last;k++) *fact += *p2++;
            //cout << i<<","<<j<<","<<first<<","<<last<<":"<<*fact<<endl;
         }
      }
      pos2->first=firstInterval;
      pos2->second.resize(pos1->size());
      REAL *pFact=pos2->second.data();
      for(map<long, REAL>::const_iterator pos=pos1->begin();pos!=pos1->end();++pos)
         *pFact++ = pos->second;
      pos1++;
      pos2++;
   }
   mClockIntegratedProfileFactor.Click();
   #ifdef __DEBUG__
   if(gVFNDebugMessageLevel<3)
   {
      unsigned long i=0;
      for(vector< pair<unsigned long, CrystVector_REAL> >::const_iterator
            pos=mIntegratedProfileFactor.begin();
            pos!=mIntegratedProfileFactor.end();++pos)
      {
         cout <<"Integrated profile factors for reflection #"<<i++<<"  ";
         for(int j=0;j<pos->second.numElements();++j)
            cout << j+pos->first<<"("<<pos->second(j)<<")  ";
         cout<<endl;
      }
   }
   #endif

   VFN_DEBUG_EXIT("PowderPatternDiffraction::PrepareIntegratedProfile()",5)
}

#ifdef __WX__CRYST__
WXCrystObjBasic* PowderPatternDiffraction::WXCreate(wxWindow* parent)
{
   //:TODO: Check mpWXCrystObj==0
   mpWXCrystObj=new WXPowderPatternDiffraction(parent,this);
   return mpWXCrystObj;
}
#endif

////////////////////////////////////////////////////////////////////////
//
//        PowderPattern
//
////////////////////////////////////////////////////////////////////////
ObjRegistry<PowderPattern> 
   gPowderPatternRegistry("List of all PowderPattern objects");

PowderPattern::PowderPattern():
m2ThetaMin(0),m2ThetaStep(0),mNbPoint(0),
m2ThetaZero(0.),m2ThetaDisplacement(0.),m2ThetaTransparency(0.),
mScaleFactor(20),mUseFastLessPreciseFunc(false),
mStatisticsExcludeBackground(false),mMaxSinThetaOvLambda(10),mNbPointUsed(0)
{
   mScaleFactor=1;
   mSubObjRegistry.SetName("SubObjRegistry for a PowderPattern object");
   mPowderPatternComponentRegistry.SetName("Powder Pattern Components");
   this->AddSubRefObj(mRadiation);
   this->Init();
   gPowderPatternRegistry.Register(*this);
   gTopRefinableObjRegistry.Register(*this);
   mClockMaster.AddChild(mClockPowderPatternPar);
   mClockMaster.AddChild(mClockNbPointUsed);
}

PowderPattern::PowderPattern(const PowderPattern &old):
m2ThetaMin(old.m2ThetaMin),m2ThetaStep(old.m2ThetaStep),mNbPoint(old.mNbPoint),
mRadiation(old.mRadiation),
m2ThetaZero(old.m2ThetaZero),m2ThetaDisplacement(old.m2ThetaDisplacement),
m2ThetaTransparency(old.m2ThetaTransparency),
mPowderPatternComponentRegistry(old.mPowderPatternComponentRegistry),
mScaleFactor(old.mScaleFactor),
mUseFastLessPreciseFunc(old.mUseFastLessPreciseFunc),
mStatisticsExcludeBackground(old.mStatisticsExcludeBackground),
mMaxSinThetaOvLambda(old.mMaxSinThetaOvLambda),mNbPointUsed(old.mNbPointUsed)
{
   this->Init();
   mSubObjRegistry.SetName("SubObjRegistry for a PowderPattern :"+mName);
   gPowderPatternRegistry.Register(*this);
   gTopRefinableObjRegistry.Register(*this);
   this->AddSubRefObj(mRadiation);
   mClockMaster.AddChild(mClockPowderPatternPar);
   mClockMaster.AddChild(mClockNbPointUsed);
}

PowderPattern::~PowderPattern()
{
   for(int i=0;i<mPowderPatternComponentRegistry.GetNb();i++)
   {
      mPowderPatternComponentRegistry.GetObj(i).DeRegisterClient(*this);
      this->RemoveSubRefObj(mPowderPatternComponentRegistry.GetObj(i));
      delete &(mPowderPatternComponentRegistry.GetObj(i));
   }
   gPowderPatternRegistry.DeRegister(*this);
   gTopRefinableObjRegistry.DeRegister(*this);
}
const string& PowderPattern::GetClassName() const
{
   const static string className="PowderPattern";
   return className;
}

void PowderPattern::AddPowderPatternComponent(PowderPatternComponent &comp)
{
   VFN_DEBUG_ENTRY("PowderPattern::AddPowderPatternComponent():"<<comp.GetName(),5)
   comp.SetParentPowderPattern(*this);
   this->AddSubRefObj(comp);
   comp.RegisterClient(*this);
   mClockPowderPatternCalc.Reset();
   mClockIntegratedFactorsPrep.Reset();
   mPowderPatternComponentRegistry.Register(comp);
   //:TODO: check if there are enough scale factors
   //mScaleFactor.resizeAndPreserve(mPowderPatternComponentRegistry.GetNb());
   mScaleFactor(mPowderPatternComponentRegistry.GetNb()-1)=1.;
   mClockScaleFactor.Click();
   if(comp.IsScalable())
   {//Init refinable parameter
      RefinablePar tmp("Scale_"+comp.GetName(),mScaleFactor.data()+mPowderPatternComponentRegistry.GetNb()-1,
                        1e-10,1e10,gpRefParTypeScattDataScale,REFPAR_DERIV_STEP_RELATIVE,
                        false,true,true,false,1.);
      tmp.SetGlobalOptimStep(0.);
      tmp.AssignClock(mClockScaleFactor);
      tmp.SetDerivStep(1e-4);
      this->AddPar(tmp);
   }
   this->UpdateDisplay();
   VFN_DEBUG_EXIT("PowderPattern::AddPowderPatternComponent():"<<comp.GetName(),5)
}

unsigned int PowderPattern::GetNbPowderPatternComponent()const
{
   return mPowderPatternComponentRegistry.GetNb();
}

const PowderPatternComponent& PowderPattern::GetPowderPatternComponent
                                                   (const string &name)const
{
   return mPowderPatternComponentRegistry.GetObj(name);
}

const PowderPatternComponent& PowderPattern::GetPowderPatternComponent
                                                   (const int i) const
{
   return mPowderPatternComponentRegistry.GetObj(i);
}

PowderPatternComponent& PowderPattern::GetPowderPatternComponent
                                                   (const string &name)
{
   return mPowderPatternComponentRegistry.GetObj(name);
}

PowderPatternComponent& PowderPattern::GetPowderPatternComponent
                                                   (const int i) 
{
   return mPowderPatternComponentRegistry.GetObj(i);
}

void PowderPattern::SetPowderPatternPar(const REAL tthetaMin,
                                          const REAL tthetaStep,
                                          unsigned long nbPoint)
{
   m2ThetaMin=tthetaMin;
   m2ThetaStep=tthetaStep;
   mNbPoint=nbPoint;
   mClockPowderPatternPar.Click();
}

unsigned long PowderPattern::GetNbPoint()const {return mNbPoint;}

void PowderPattern::SetRadiation(const Radiation &radiation)
{
   mRadiation=radiation;
   mClockPowderPatternRadiation.Click();
}
const Radiation& PowderPattern::GetRadiation()const {return mRadiation;}

void PowderPattern::SetRadiationType(const RadiationType rad)
{
   mRadiation.SetRadiationType(rad);
}

RadiationType PowderPattern::GetRadiationType()const {return mRadiation.GetRadiationType();}
void PowderPattern::SetWavelength(const REAL lambda)
{
   VFN_DEBUG_MESSAGE("PowderPattern::SetWavelength(lambda)",3)
   mRadiation.SetWavelength(lambda);
}

void PowderPattern::SetWavelength(const string &XRayTubeElementName,const REAL alpha12ratio)
{
   VFN_DEBUG_MESSAGE("PowderPattern::SetWavelength(wavelength)",3)
   mRadiation.SetWavelength(XRayTubeElementName,alpha12ratio);
}

REAL PowderPattern::GetWavelength()const{return mRadiation.GetWavelength()(0);}

const CrystVector_REAL& PowderPattern::GetPowderPatternCalc()const
{
   this->CalcPowderPattern();
   return mPowderPatternCalc;
}

const CrystVector_REAL& PowderPattern::GetPowderPatternObs()const
{
   return mPowderPatternObs;
}

const CrystVector_REAL& PowderPattern::GetPowderPatternObsSigma()const
{
   return mPowderPatternObsSigma;
}

const CrystVector_REAL& PowderPattern::GetPowderPatternVariance()const
{
   return mPowderPatternVariance;
}

const CrystVector_REAL& PowderPattern::GetPowderPatternWeight()const
{
   return mPowderPatternWeight;
}

//CrystVector_REAL PowderPattern::Get2Theta()const

REAL PowderPattern::Get2ThetaMin()const {return m2ThetaMin;}

REAL PowderPattern::Get2ThetaStep()const {return m2ThetaStep;}

REAL PowderPattern::Get2ThetaMax()const 
{
   VFN_DEBUG_MESSAGE("PowderPattern::Get2ThetaMax()",2)
   return m2ThetaMin+m2ThetaStep*mNbPoint;
}

const RefinableObjClock& PowderPattern::GetClockPowderPatternCalc()const
{  return mClockPowderPatternCalc;}

const RefinableObjClock& PowderPattern::GetClockPowderPatternPar()const
{  return mClockPowderPatternPar;}

const RefinableObjClock& PowderPattern::GetClockPowderPatternRadiation()const
{  return mClockPowderPatternRadiation;}

const RefinableObjClock& PowderPattern::GetClockPowderPattern2ThetaCorr()const
{  return mClockPowderPattern2ThetaCorr;}

void PowderPattern::Set2ThetaZero(const REAL newZero)
{
   m2ThetaZero=newZero;
   mClockPowderPatternPar.Click();
}

void PowderPattern::Set2ThetaDisplacement(const REAL displacement)
{
   m2ThetaDisplacement=displacement;
   mClockPowderPatternPar.Click();
}

void PowderPattern::Set2ThetaTransparency(const REAL transparency)
{
   m2ThetaTransparency=transparency;
   mClockPowderPatternPar.Click();
}

REAL PowderPattern::Get2ThetaCorr(const REAL ttheta)const
{
   REAL t=ttheta;
   t +=  m2ThetaZero +m2ThetaDisplacement/cos(t/2) +m2ThetaTransparency*sin(t);
   return t;
}

long PowderPattern::Get2ThetaCorrPixel(const REAL ttheta)const
{
   REAL t=ttheta;
   t +=  m2ThetaZero +m2ThetaDisplacement/cos(t/2) +m2ThetaTransparency*sin(t);
   return (long) ((t-m2ThetaMin)/m2ThetaStep);
}
void PowderPattern::ImportPowderPatternFullprof(const string &filename)
{
   //15.000   0.030  70.000 LANI4FE#1 REC 800� 4JRS                                 
   //2447.   2418.   2384.   2457.   2398.   2374.   2378.   2383.
   //...
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPatternFullprof() : \
from file : "+filename,5)
   ifstream fin(filename.c_str());
   if(!fin)
   {
      throw ObjCrystException("PowderPattern::ImportPowderPatternFullprof() : \
Error opening file for input:"+filename);
   }
   fin >> m2ThetaMin;
   fin >> m2ThetaStep;
   REAL tmp;
   fin >> tmp;
   mNbPoint=(long)((tmp-m2ThetaMin)/m2ThetaStep+1.001);
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPatternFullprof() :"\
      << " 2Theta min=" << m2ThetaMin << " 2Theta max=" << tmp \
      << " NbPoints=" << mNbPoint,5)
   m2ThetaMin  *= DEG2RAD;
   m2ThetaStep *= DEG2RAD;
   mPowderPatternObs.resize (mNbPoint);
   mPowderPatternObsSigma.resize (mNbPoint);
   mPowderPatternWeight.resize(mNbPoint);
   
   char tmpComment[200];
   fin.getline(tmpComment,100);
   //if(""==mName) mName.append(tmpComment);
   
   for(unsigned long i=0;i<mNbPoint;i++) fin >> mPowderPatternObs(i);
   fin.close();
   this->SetSigmaToSqrtIobs();
   this->SetWeightToInvSigmaSq();
   mClockPowderPatternPar.Click();
   this->UpdateDisplay();
   {
      char buf [200];
      sprintf(buf,"Imported powder pattern: %d points, 2theta=%7.3f -> %7.3f, step=%6.3f",
              (int)mNbPoint,m2ThetaMin*RAD2DEG,(m2ThetaMin+m2ThetaStep*(mNbPoint-1))*RAD2DEG,
              m2ThetaStep*RAD2DEG);
      (*fpObjCrystInformUser)((string)buf);
   }
   VFN_DEBUG_MESSAGE("PowderPattern::ImportFullProfPattern():finished:"<<mNbPoint<<" points",5)
}

void PowderPattern::ImportPowderPatternPSI_DMC(const string &filename)
{
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPatternPSI_DMC() : \
from file : "+filename,5)
   ifstream fin(filename.c_str());
   if(!fin)
   {
      throw ObjCrystException("PowderPattern::ImportPowderPatternPSI_DMC() : \
Error opening file for input:"+filename);
   }
   //Skip the first two lines
      char tmpComment[200];
      fin.getline(tmpComment,190);
      fin.getline(tmpComment,190);
   
   fin >> m2ThetaMin;
   fin >> m2ThetaStep;
   REAL tmp;
   fin >> tmp;
   mNbPoint=(long)((tmp-m2ThetaMin+.001)/m2ThetaStep)+1;
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPatternPSI_DMC() :"\
      << " 2Theta min=" << m2ThetaMin << " 2Theta max=" << tmp \
      << " NbPoints=" << mNbPoint,5)
   m2ThetaMin  *= DEG2RAD;
   m2ThetaStep *= DEG2RAD;
   mPowderPatternObs.resize (mNbPoint);
   mPowderPatternObsSigma.resize (mNbPoint);
   mPowderPatternWeight.resize(mNbPoint);
   
   fin.getline(tmpComment,100);
   //if(""==mName) mName.append(tmpComment);
   
   for(unsigned long i=0;i<mNbPoint;i++) fin >> mPowderPatternObs(i);
   for(unsigned long i=0;i<mNbPoint;i++) fin >> mPowderPatternObsSigma(i);
   fin.close();
   this->SetWeightToInvSigmaSq();
   mClockPowderPatternPar.Click();
   this->UpdateDisplay();
   {
      char buf [200];
      sprintf(buf,"Imported powder pattern: %d points, 2theta=%7.3f -> %7.3f, step=%6.3f",
              (int)mNbPoint,m2ThetaMin*RAD2DEG,(m2ThetaMin+m2ThetaStep*(mNbPoint-1))*RAD2DEG,
              m2ThetaStep*RAD2DEG);
      (*fpObjCrystInformUser)((string)buf);
   }
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPatternPSI_DMC():finished",5)
}

void PowderPattern::ImportPowderPatternILL_D1A5(const string &filename)
{
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPatternILL_D1AD2B() : \
from file : "+filename,5)
   ifstream fin(filename.c_str());
   if(!fin)
   {
      throw ObjCrystException("PowderPattern::ImportPowderPatternILL_D1AD2B() : \
Error opening file for input:"+filename);
   }
   //Skip the first three lines
      char tmpComment[200];
      fin.getline(tmpComment,190);
      fin.getline(tmpComment,190);
      fin.getline(tmpComment,190);
   
   fin >> mNbPoint;
   fin.getline(tmpComment,190);
   fin >> m2ThetaMin;
   fin >> m2ThetaStep;
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPatternILL_D1AD2B() :"\
      << " 2Theta min=" << m2ThetaMin << " 2Theta max=" << m2ThetaMin+mNbPoint*m2ThetaStep \
      << " NbPoints=" << mNbPoint,5)
   m2ThetaMin  *= DEG2RAD;
   m2ThetaStep *= DEG2RAD;
   mPowderPatternObs.resize (mNbPoint);
   mPowderPatternObsSigma.resize (mNbPoint);
   mPowderPatternWeight.resize(mNbPoint);
   
   //if(""==mName) mName.append(tmpComment);
   
   for(unsigned long i=0;i<mNbPoint;i++) fin >> mPowderPatternObs(i);
   for(unsigned long i=0;i<mNbPoint;i++) fin >> mPowderPatternObsSigma(i);
   fin.close();
   this->SetWeightToInvSigmaSq();
   mClockPowderPatternPar.Click();
   this->UpdateDisplay();
   {
      char buf [200];
      sprintf(buf,"Imported powder pattern: %d points, 2theta=%7.3f -> %7.3f, step=%6.3f",
              (int)mNbPoint,m2ThetaMin*RAD2DEG,(m2ThetaMin+m2ThetaStep*(mNbPoint-1))*RAD2DEG,
              m2ThetaStep*RAD2DEG);
      (*fpObjCrystInformUser)((string)buf);
   }
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPatternILL_D1AD2B():finished",5)
}

void PowderPattern::ImportPowderPatternXdd(const string &filename)
{
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPatternXdd():from file :" \
                           +filename,5)
   ifstream fin (filename.c_str());
   if(!fin)
   {
      throw ObjCrystException("PowderPattern::ImportPowderPatternXdd() : \
Error opening file for input:"+filename);
   }
   char tmpComment[200];
   fin.getline(tmpComment,100);
   //if(""==mName) mName.append(tmpComment);
   
   fin >> m2ThetaMin;
   fin >> m2ThetaStep;
   REAL tmp;
   fin >> tmp;
   mNbPoint=(long)((tmp-m2ThetaMin)/m2ThetaStep+1);
   m2ThetaMin  *= DEG2RAD;
   m2ThetaStep *= DEG2RAD;
   mPowderPatternObs.resize (mNbPoint);
   mPowderPatternObsSigma.resize(mNbPoint);
   mPowderPatternCalc.resize(mNbPoint);
   mPowderPatternWeight.resize(mNbPoint);
   mPowderPatternWeight=1.;
   
   fin >> tmp; //Count time
   fin >> tmp; //unused
   fin >> tmp; //unused (wavelength?)
   
   for(unsigned long i=0;i<mNbPoint;i++) fin >> mPowderPatternObs(i);
   fin.close();
   this->SetSigmaToSqrtIobs();
   this->SetWeightToInvSigmaSq();
   this->UpdateDisplay();
   {
      char buf [200];
      sprintf(buf,"Imported powder pattern: %d points, 2theta=%7.3f -> %7.3f, step=%6.3f",
              (int)mNbPoint,m2ThetaMin*RAD2DEG,(m2ThetaMin+m2ThetaStep*(mNbPoint-1))*RAD2DEG,
              m2ThetaStep*RAD2DEG);
      (*fpObjCrystInformUser)((string)buf);
   }
   VFN_DEBUG_MESSAGE("DiffractionDataPowder::ImportXddPattern() :finished",5)
}

void PowderPattern::ImportPowderPatternSietronicsCPI(const string &filename)
{
   VFN_DEBUG_ENTRY("PowderPattern::ImportPowderPatternSietronicsCPI():from file :" \
                           +filename,5)
   ifstream fin (filename.c_str());
   if(!fin)
   {
      throw ObjCrystException("PowderPattern::ImportPowderPatternSietronicsCPI() : \
Error opening file for input:"+filename);
   }
   char tmpComment[300];
   fin.getline(tmpComment,100);
   VFN_DEBUG_MESSAGE(" ->Discarded comment :"<<tmpComment,1)
   
   fin >> m2ThetaMin;
   REAL tmp;
   fin >> tmp;//2Theta Max
   fin >> m2ThetaStep;
   mNbPoint=(long)((tmp-m2ThetaMin)/m2ThetaStep+1);
   m2ThetaMin  *= DEG2RAD;
   m2ThetaStep *= DEG2RAD;
   mPowderPatternObs.resize (mNbPoint);
   mPowderPatternObsSigma.resize(mNbPoint);
   mPowderPatternCalc.resize(mNbPoint);
   mPowderPatternWeight.resize(mNbPoint);
   mPowderPatternWeight=1.;
   
   //Following lines are ignored (no fixed format ?)
   string str;
   do
   {
      fin>>str;
      VFN_DEBUG_MESSAGE(" ->Read :"<<str,1)
   } while ("SCANDATA"!=str);
   
   for(unsigned long i=0;i<mNbPoint;i++) fin >> mPowderPatternObs(i);
   fin.close();
   this->SetSigmaToSqrtIobs();
   this->SetWeightToInvSigmaSq();
   mClockPowderPatternPar.Click();
   this->UpdateDisplay();
   {
      char buf [200];
      sprintf(buf,"Imported powder pattern: %d points, 2theta=%7.3f -> %7.3f, step=%6.3f",
              (int)mNbPoint,m2ThetaMin*RAD2DEG,(m2ThetaMin+m2ThetaStep*(mNbPoint-1))*RAD2DEG,
              m2ThetaStep*RAD2DEG);
      (*fpObjCrystInformUser)((string)buf);
   }
   VFN_DEBUG_EXIT("DiffractionDataPowder::ImportPowderPatternSietronicsCPI()",5)
}

void PowderPattern::ImportPowderPattern2ThetaObsSigma(const string &filename,const int nbSkip)
{
   VFN_DEBUG_MESSAGE("DiffractionDataPowder::ImportPowderPattern2ThetaObsSigma():from:" \
                           +filename,5)
   ifstream fin (filename.c_str());
   if(!fin)
   {
      throw ObjCrystException("PowderPattern::ImportPowderPattern2ThetaObsSigma():\
Error opening file for input:"+filename);
   }
   {//Get rid of first lines
      char tmpComment[200];
      for(int i=0;i<nbSkip;i++) fin.getline(tmpComment,150);
   }
   mPowderPatternObs.resize (500);
   mPowderPatternObsSigma.resize(500);
   CrystVector_REAL tmp2Theta(500);
   mNbPoint=0;
   
   do
   {
      fin >> tmp2Theta              (mNbPoint);
      fin >> mPowderPatternObs     (mNbPoint);
      fin >> mPowderPatternObsSigma(mNbPoint);
      //cout << tmp2Theta              (mNbPoint)<<" "
      //     << mPowderPatternObs     (mNbPoint)<<" "
      //    << mPowderPatternObsSigma(mNbPoint)<<endl;
      if(!fin) break;
      mNbPoint++;
      if( (mNbPoint%500)==0)
      {
         tmp2Theta.resizeAndPreserve(mNbPoint+500);
         mPowderPatternObs.resizeAndPreserve(mNbPoint+500);
         mPowderPatternObsSigma.resizeAndPreserve(mNbPoint+500);
      }
   } while(fin.eof() == false);
   fin.close();
   
   tmp2Theta.resizeAndPreserve              (mNbPoint);
   mPowderPatternObs.resizeAndPreserve     (mNbPoint);
   mPowderPatternObsSigma.resizeAndPreserve(mNbPoint);
   mPowderPatternWeight.resize(mNbPoint);
   
   m2ThetaMin=tmp2Theta(0);
   m2ThetaStep=(tmp2Theta(mNbPoint-1)-tmp2Theta(0))
                                 /(mNbPoint-1);
   m2ThetaMin  *= DEG2RAD;
   m2ThetaStep *= DEG2RAD;
   
   this->SetWeightToInvSigmaSq();
   mClockPowderPatternPar.Click();
   this->UpdateDisplay();
   {
      char buf [200];
      sprintf(buf,"Imported powder pattern: %d points, 2theta=%7.3f -> %7.3f, step=%6.3f",
              (int)mNbPoint,m2ThetaMin*RAD2DEG,(m2ThetaMin+m2ThetaStep*(mNbPoint-1))*RAD2DEG,
              m2ThetaStep*RAD2DEG);
      (*fpObjCrystInformUser)((string)buf);
   }
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPattern2ThetaObsSigma()\
:finished: "<<mNbPoint<<" points",5)
}

void PowderPattern::ImportPowderPattern2ThetaObs(const string &filename,const int nbSkip)
{
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPattern2ThetaObs():from:" \
                           +filename,5)
   ifstream fin (filename.c_str());
   if(!fin)
   {
      throw ObjCrystException("PowderPattern::ImportPowderPattern2ThetaObs():\
Error opening file for input:"+filename);
   }
   {//Get rid of first lines
      char tmpComment[200];
      for(int i=0;i<nbSkip;i++) fin.getline(tmpComment,150);
   }
   mPowderPatternObs.resize (500);
   mPowderPatternObsSigma.resize(500);
   CrystVector_REAL tmp2Theta(500);
   mNbPoint=0;
   
   do
   {
      fin >> tmp2Theta              (mNbPoint);
      fin >> mPowderPatternObs     (mNbPoint);
      mPowderPatternObsSigma(mNbPoint)
         =sqrt(mPowderPatternObs(mNbPoint));
      if(!fin) break;
      mNbPoint++;
      if( (mNbPoint%500)==0)
      {
         tmp2Theta.resizeAndPreserve(mNbPoint+500);
         mPowderPatternObs.resizeAndPreserve(mNbPoint+500);
         mPowderPatternObsSigma.resizeAndPreserve(mNbPoint+500);
      }
   } while(fin.eof() == false);
   fin.close();
   
   tmp2Theta.resizeAndPreserve              (mNbPoint);
   mPowderPatternObs.resizeAndPreserve     (mNbPoint);
   mPowderPatternObsSigma.resizeAndPreserve(mNbPoint);
   mPowderPatternWeight.resize(mNbPoint);

   m2ThetaMin=tmp2Theta(0);
   m2ThetaStep=(tmp2Theta(mNbPoint-1)-tmp2Theta(0))/(mNbPoint-1);
   m2ThetaMin  *= DEG2RAD;
   m2ThetaStep *= DEG2RAD;

   this->SetSigmaToSqrtIobs();
   this->SetWeightToInvSigmaSq();
   mClockPowderPatternPar.Click();
   this->UpdateDisplay();
   {
      char buf [200];
      sprintf(buf,"Imported powder pattern: %d points, 2theta=%7.3f -> %7.3f, step=%6.3f",
              (int)mNbPoint,m2ThetaMin*RAD2DEG,(m2ThetaMin+m2ThetaStep*(mNbPoint-1))*RAD2DEG,
              m2ThetaStep*RAD2DEG);
      (*fpObjCrystInformUser)((string)buf);
   }
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPattern2ThetaObs():finished",5)
}

void PowderPattern::ImportPowderPatternMultiDetectorLLBG42(const string &filename)
{

   //Sample 4: NaY + CF2=CCL2 T=20K, Lambda: 2.343 A.                                
   //     100       0   0.100      70       0       0
   //   3.000
   // 500000.  12000.    0.00    0.00
   //70  570369  569668  562868  532769  527469  495669  481969  452767  429468  4132
   //68  393269  372067  353769  337068  328268  310270  299469  296470  294668  2780
   //...
   //14  255814  282714  274014  281314  300314  302714  301114  298014  313214  3097
   //14  286914  295714  305214  300714  288311  295511  288511  3024 8  2937 7  2883
   // 7  2905 7  2895 7  2767 7  2777 7  2758 7  2495 7  2507 7  2496 7  2382 7  2329
   // 7  2542 7  2415 7  2049 7  2389 7  2270 7  2157 6  2227 6  2084 3  1875 2  2094
   // 1  1867
   //   -1000
   //  -10000
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPatternMultiDetectorLLBG42() : \
from file : "+filename,5)
   ifstream fin(filename.c_str());
   if(!fin)
   {
      throw ObjCrystException("PowderPattern::ImportPowderPatternMultiDetectorLLBG42() : \
Error opening file for input:"+filename);
   }
   
   string str;
   getline(fin,str);
   float junk;
   fin >> junk;
   fin >> junk;
   fin >> m2ThetaStep;
   fin >> junk;
   fin >> junk;
   fin >> junk;
   fin >> m2ThetaMin;
   fin >> junk;
   fin >> junk;
   fin >> junk;
   fin >> junk;
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPatternMultiDetectorLLBG42() :"\
      << " 2Theta min=" << m2ThetaMin << " 2Theta step=" <<  m2ThetaStep,5)
   m2ThetaMin  *= DEG2RAD;
   m2ThetaStep *= DEG2RAD;
   mPowderPatternObs.resize (1000);
   mPowderPatternObsSigma.resize (1000);
   
   getline(fin,str);//finish reading line
   
   float tmp;
   string sub;
   float ct,iobs;
   mNbPoint=0;
   for(;;)
   {            
      getline(fin,str);
      sscanf(str.c_str(),"%f",&tmp);
      if(tmp<0) break;
      const unsigned int nb=str.length()/8;
      for(unsigned int i=0;i<nb;i++)
      {
         if(mNbPoint==(unsigned int)mPowderPatternObs.numElements())
         {
            mPowderPatternObs.resizeAndPreserve(mNbPoint+100);
            mPowderPatternObsSigma.resizeAndPreserve(mNbPoint+100);
         }
         sub=str.substr(i*8,8);
         sscanf(sub.c_str(),"%2f%6f",&ct,&iobs);
         mPowderPatternObs(mNbPoint)=iobs;
         mPowderPatternObsSigma(mNbPoint++)=sqrt(iobs/ct);
      }
   }
   //exit(1);
   mPowderPatternObs.resizeAndPreserve (mNbPoint);
   mPowderPatternObsSigma.resizeAndPreserve (mNbPoint);
   mPowderPatternWeight.resizeAndPreserve(mNbPoint);
   
   fin.close();
   this->SetWeightToInvSigmaSq();
   mClockPowderPatternPar.Click();
   this->UpdateDisplay();
   {
      char buf [200];
      sprintf(buf,"Imported powder pattern: %d points, 2theta=%7.3f -> %7.3f, step=%6.3f",
              (int)mNbPoint,m2ThetaMin*RAD2DEG,(m2ThetaMin+m2ThetaStep*(mNbPoint-1))*RAD2DEG,
              m2ThetaStep*RAD2DEG);
      (*fpObjCrystInformUser)((string)buf);
   }
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPatternMultiDetectorLLBG42():finished:"<<mNbPoint<<" points",5)
}

void PowderPattern::ImportPowderPatternFullprof4(const string &filename)
{
   //1.550   0.005  66.000
   // 213.135 193.243 208.811 185.873 231.607 200.995 196.792 187.516 215.977 199.634
   //  17.402  16.570  12.180  11.491  18.141  11.950  11.824  11.542  17.518  11.909
   // 211.890 185.740 204.610 200.645 199.489 169.549 203.189 178.298 186.241 198.522
   //  12.269  11.487  17.051  11.939  11.905  10.975  16.992  11.255  11.503  11.876
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPatternFullprof4() : \
from file : "+filename,5)
   ifstream fin(filename.c_str());
   if(!fin)
   {
      throw ObjCrystException("PowderPattern::ImportPowderPatternFullprof4() : \
Error opening file for input:"+filename);
   }
   fin >> m2ThetaMin;
   fin >> m2ThetaStep;
   REAL tmp;
   fin >> tmp;
   mNbPoint=(long)((tmp-m2ThetaMin)/m2ThetaStep+1.0001);
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPatternFullprof4() :"\
      << " 2Theta min=" << m2ThetaMin << " 2Theta max=" << tmp \
      << " NbPoints=" << mNbPoint,5)
   m2ThetaMin  *= DEG2RAD;
   m2ThetaStep *= DEG2RAD;
   mPowderPatternObs.resize (mNbPoint);
   mPowderPatternObsSigma.resize (mNbPoint);
   mPowderPatternWeight.resize(mNbPoint);
   
   string str;
   getline(fin,str);//read end of first line
   
   unsigned ct=0;
   unsigned ctSig=0;
   float line[10];
   for(;ct<mNbPoint;)
   {            
      getline(fin,str);
      for(unsigned int i=0;i<str.size();i++) if(' '==str[i]) str[i]='0';
      sscanf(str.c_str(),"%8f%8f%8f%8f%8f%8f%8f%8f%8f%8f",
                         line+0,line+1,line+2,line+3,line+4,line+5,line+6,line+7,line+8,line+9);
      for(unsigned int j=0;j<10;j++)
         if(ct<mNbPoint) mPowderPatternObs(ct++)=line[j];
      getline(fin,str);
      for(unsigned int i=0;i<str.size();i++) if(' '==str[i]) str[i]='0';
      sscanf(str.c_str(),"%8f%8f%8f%8f%8f%8f%8f%8f%8f%8f",
                         line+0,line+1,line+2,line+3,line+4,line+5,line+6,line+7,line+8,line+9);
      for(unsigned int j=0;j<10;j++)
         if(ctSig<mNbPoint) mPowderPatternObsSigma(ctSig++)=line[j];
   }
   fin.close();
   this->SetWeightToInvSigmaSq();
   mClockPowderPatternPar.Click();
   this->UpdateDisplay();
   {
      char buf [200];
      sprintf(buf,"Imported powder pattern: %d points, 2theta=%7.3f -> %7.3f, step=%6.3f",
              (int)mNbPoint,m2ThetaMin*RAD2DEG,(m2ThetaMin+m2ThetaStep*(mNbPoint-1))*RAD2DEG,
              m2ThetaStep*RAD2DEG);
      (*fpObjCrystInformUser)((string)buf);
   }
   VFN_DEBUG_MESSAGE("PowderPattern::ImportFullProfPattern4():finished:"<<mNbPoint<<" points",5)
}

void PowderPattern::SetPowderPatternObs(const CrystVector_REAL& obs)
{
   VFN_DEBUG_MESSAGE("PowderPattern::ImportPowderPatternObs()",5)
   if((unsigned long)obs.numElements() != mNbPoint)
   {
      cout << obs.numElements()<<" "<<mNbPoint<<" "<<this<<endl;
      throw(ObjCrystException("PowderPattern::SetPowderPatternObs(vect): The \
supplied vector of observed intensities does not have the expected number of points!"));
   }
   
   mPowderPatternObs=obs;
   mPowderPatternObsSigma.resize(mPowderPatternObs.numElements());
   mPowderPatternWeight.resize(mNbPoint);
   
   this->SetSigmaToSqrtIobs();
   this->SetWeightToInvSigmaSq();
   mClockIntegratedFactorsPrep.Reset();
   {
      char buf [200];
      sprintf(buf,"Changed powder pattern: %d points, 2theta=%7.3f -> %7.3f, step=%6.3f",
              (int)mNbPoint,m2ThetaMin*RAD2DEG,(m2ThetaMin+m2ThetaStep*(mNbPoint-1))*RAD2DEG,
              m2ThetaStep*RAD2DEG);
      (*fpObjCrystInformUser)((string)buf);
   }
}
void PowderPattern::SavePowderPattern(const string &filename) const
{
   VFN_DEBUG_MESSAGE("PowderPattern::SavePowderPattern",5)
   this->CalcPowderPattern();
   ofstream out(filename.c_str());
   CrystVector_REAL ttheta(mNbPoint);
   //REAL *p=ttheta.data();
   for(unsigned long i=0;i<mNbPoint;i++) 
      ttheta(i) = m2ThetaMin+ i * m2ThetaStep;
   ttheta *= RAD2DEG;
   
   CrystVector_REAL diff;
   diff=mPowderPatternObs;
   diff-=mPowderPatternCalc;
   out << "#    2Theta     Iobs       ICalc   Iobs-Icalc    Weight  Comp0" << endl;
   out << FormatVertVector<REAL>(ttheta,
                    mPowderPatternObs,
                    mPowderPatternCalc,
                    diff,mPowderPatternWeight,
                    mPowderPatternComponentRegistry.GetObj(0).mPowderPatternCalc,12,4);
   out.close();
   VFN_DEBUG_MESSAGE("DiffractionDataPowder::SavePowderPattern:End",3)
}

void PowderPattern::PrintObsCalcData(ostream&os)const
{
   VFN_DEBUG_MESSAGE("DiffractionDataPowder::PrintObsCalcData()",5);
   CrystVector_REAL ttheta(mNbPoint);
   //REAL *p=ttheta.data();
   for(unsigned long i=0;i<mNbPoint;i++) 
      ttheta(i) = m2ThetaMin+ i * m2ThetaStep;
   ttheta *= RAD2DEG;
   os << "PowderPattern : " << mName <<endl;
   os << "      2Theta      Obs          Sigma        Calc        Weight" <<endl;
   os << FormatVertVector<REAL>(ttheta,mPowderPatternObs,mPowderPatternObsSigma,
               mPowderPatternCalc,mPowderPatternWeight,12,4);
   //            mPowderPatternComponentRegistry.GetObj(0).mPowderPatternCalc,12,4);
}

REAL PowderPattern::GetR()const
{
   if(  (0==this->GetPowderPatternObs().numElements())
      ||(0==GetNbPowderPatternComponent()))
   {
      return 0;
   } 
   this->CalcPowderPattern();
   TAU_PROFILE("PowderPattern::GetR()","void ()",TAU_DEFAULT);
   
   REAL tmp1=0.;
   REAL tmp2=0.;
   
   unsigned long maxPoints=mNbPointUsed;
   if(  (true==mStatisticsExcludeBackground)
      &&(mPowderPatternBackgroundCalc.numElements()>0))
   {
      const REAL *p1, *p2, *p3;
      p1=mPowderPatternCalc.data();
      p2=mPowderPatternObs.data();
      p3=mPowderPatternBackgroundCalc.data();
      const long nbExclude=mExcludedRegionMin2Theta.numElements();
      if(0==nbExclude)
      {
         VFN_DEBUG_MESSAGE("PowderPattern::GetR():Exclude Backgd",4);
         for(unsigned long i=0;i<maxPoints;i++)
         {
            tmp1 += ((*p1)-(*p2)) * ((*p1)-(*p2));
            tmp2 += ((*p2)-(*p3)) * ((*p2)-(*p3));
            p1++;p2++;p3++;
         }
      }
      else
      {
         VFN_DEBUG_MESSAGE("PowderPattern::GetR():Exclude Backgd,Exclude regions",4);
         unsigned long min,max;
         unsigned long i=0;
         for(int j=0;j<nbExclude;j++)
         {
            min=(long)floor((mExcludedRegionMin2Theta(j)-m2ThetaMin)
                                 /m2ThetaStep);
            max=(long)ceil((mExcludedRegionMax2Theta(j)-m2ThetaMin)
                                 /m2ThetaStep);
            if(min>maxPoints) break;
            if(max>maxPoints)max=maxPoints;
            for(;i<min;i++)//! min is the *beginning* of the excluded region !
            {
               tmp1 += ((*p1)-(*p2)) * ((*p1)-(*p2));
               tmp2 += ((*p2)-(*p3)) * ((*p2)-(*p3));
               p1++;p2++;p3++;
            }
            p1 += max-i;
            p2 += max-i;
            p3 += max-i;
            i  += max-i;
         }
         for(;i<maxPoints;i++)
         {
            tmp1 += ((*p1)-(*p2)) * ((*p1)-(*p2));
            tmp2 += ((*p2)-(*p3)) * ((*p2)-(*p3));
            p1++;p2++;p3++;
         }

      }
   } // Exclude Background ?
   else
   {
      const REAL *p1, *p2;
      p1=mPowderPatternCalc.data();
      p2=mPowderPatternObs.data();
      const long nbExclude=mExcludedRegionMin2Theta.numElements();
      if(0==nbExclude)
      {
         VFN_DEBUG_MESSAGE("PowderPattern::GetR()",4);
         for(unsigned long i=0;i<maxPoints;i++)
         {
            tmp1 += ((*p1)-(*p2))*((*p1)-(*p2));
            tmp2 += (*p2) * (*p2);
            //cout <<i<<":"<< tmp1 << " "<<tmp2 << " " << *p1 <<" "<<*p2<<endl;
            p1++;p2++;
         }
      }
      else
      {
         VFN_DEBUG_MESSAGE("PowderPattern::GetR(),Exclude regions",4);
         unsigned long min,max;
         unsigned long i=0;
         for(int j=0;j<nbExclude;j++)
         {
            min=(long)floor((mExcludedRegionMin2Theta(j)-m2ThetaMin)
                                 /m2ThetaStep);
            max=(long)ceil((mExcludedRegionMax2Theta(j)-m2ThetaMin)
                                 /m2ThetaStep);
            if(min>maxPoints) break;
            if(max>maxPoints)max=maxPoints;
            for(;i<min;i++)//! min is the *beginning* of the excluded region !
            {
               tmp1 += ((*p1)-(*p2))*((*p1)-(*p2));
               tmp2 += (*p2) * (*p2);
               p1++;p2++;
            }
            p1 += max-i;
            p2 += max-i;
            i  += max-i;
         }
         for(;i<maxPoints;i++)
         {
            tmp1 += ((*p1)-(*p2))*((*p1)-(*p2));
            tmp2 += (*p2) * (*p2);
            p1++;p2++;
         }
      }
   }
   
   VFN_DEBUG_MESSAGE("PowderPattern::GetR()="<<sqrt(tmp1/tmp2),4);
   //cout << FormatVertVector<REAL>(mPowderPatternCalc,mPowderPatternObs);
   //this->SavePowderPattern("refinedPattern.out");
   //abort();
   return sqrt(tmp1/tmp2);
}

REAL PowderPattern::GetIntegratedR()const
{
   if(  (0==this->GetPowderPatternObs().numElements())
      ||(0==GetNbPowderPatternComponent()))
   {
      return 0;
   } 
   this->CalcPowderPattern();
   this->PrepareIntegratedRfactor();
   VFN_DEBUG_ENTRY("PowderPattern::GetIntegratedR()",4);
   TAU_PROFILE("PowderPattern::GetIntegratedR()","void ()",TAU_DEFAULT);
   
   REAL tmp1=0.;
   REAL tmp2=0.;
   const long numInterval=mIntegratedPatternMin.numElements();
   if(  (true==mStatisticsExcludeBackground)
      &&(mPowderPatternBackgroundCalc.numElements()>0))
   {
      const REAL *p1, *p2, *p3;
      CrystVector_REAL integratedCalc(numInterval);
      integratedCalc=0;
      CrystVector_REAL backgdCalc(numInterval);
      backgdCalc=0;
      REAL *pp1=integratedCalc.data();
      REAL *pp2=backgdCalc.data();
      for(int i=0;i<numInterval;i++)
      {
         const long max=mIntegratedPatternMax(i);
         p1=mPowderPatternCalc.data()+mIntegratedPatternMin(i);
         for(int j=mIntegratedPatternMin(i);j<=max;j++) *pp1 += *p1++;
         pp1++;
         p1=mPowderPatternBackgroundCalc.data()+mIntegratedPatternMin(i);
         for(int j=mIntegratedPatternMin(i);j<=max;j++) *pp2 += *p1++;
         pp2++;
      }
      
      p1=integratedCalc.data();
      p2=mIntegratedObs.data();
      p3=backgdCalc.data();
      VFN_DEBUG_MESSAGE("PowderPattern::GetIntegratedR():Exclude Backgd",2);
      for(long i=0;i<numInterval;i++)
      {
         tmp1 += ((*p1)-(*p2)) * ((*p1)-(*p2));
         tmp2 += ((*p2)-(*p3)) * ((*p2)-(*p3));
         p1++;p2++;p3++;
      }
   } // Exclude Background ?
   else
   {
      const REAL *p1, *p2;
      CrystVector_REAL integratedCalc(numInterval);
      integratedCalc=0;
      REAL *pp1=integratedCalc.data();
      for(int i=0;i<numInterval;i++)
      {
         const long max=mIntegratedPatternMax(i);
         p1=mPowderPatternCalc.data()+mIntegratedPatternMin(i);
         for(int j=mIntegratedPatternMin(i);j<=max;j++) *pp1 += *p1++;
         pp1++;
      }
      p1=integratedCalc.data();
      p2=mIntegratedObs.data();
      VFN_DEBUG_MESSAGE("PowderPattern::GetIntegratedR()",2);
      for(long i=0;i<numInterval;i++)
      {
         tmp1 += ((*p1)-(*p2))*((*p1)-(*p2));
         tmp2 += (*p2) * (*p2);
         //cout <<i<<":"<< tmp1 << " "<<tmp2 << " " << *p1 <<" "<<*p2<<endl;
         p1++;p2++;
      }
   }
   
   VFN_DEBUG_EXIT("PowderPattern::GetIntegratedR()="<<sqrt(tmp1/tmp2),4);
   return sqrt(tmp1/tmp2);
}

REAL PowderPattern::GetRw()const
{
   if(  (0==this->GetPowderPatternObs().numElements())
      ||(0==GetNbPowderPatternComponent()))
   {
      return 0;
   } 
   this->CalcPowderPattern();
   TAU_PROFILE("PowderPattern::GetRw()","void ()",TAU_DEFAULT);
   VFN_DEBUG_MESSAGE("PowderPattern::GetRw()",3);
   
   
   //cout <<FormatVertVector<REAL>(mPowderPatternObs,
   //                               mPowderPatternCalc,
   //                               mPowderPatternWeight);
   REAL tmp1=0.;
   REAL tmp2=0.;
   
   unsigned long maxPoints=mNbPointUsed;

   if(  (true==mStatisticsExcludeBackground)
      &&(mPowderPatternBackgroundCalc.numElements()>0))
   {
      VFN_DEBUG_MESSAGE("PowderPattern::GetRw():Exclude Backgd",3);
      const REAL *p1, *p2, *p3, *p4;
      p1=mPowderPatternCalc.data();
      p2=mPowderPatternObs.data();
      p3=mPowderPatternBackgroundCalc.data();
      p4=mPowderPatternWeight.data();
      const long nbExclude=mExcludedRegionMin2Theta.numElements();
      if(0==nbExclude)
      {
         for(unsigned long i=0;i<maxPoints;i++)
         {
            tmp1 += *p4   * ((*p1)-(*p2)) * ((*p1)-(*p2));
            tmp2 += *p4++ * ((*p2)-(*p3)) * ((*p2)-(*p3));
            p1++;p2++;p3++;
         }
      }
      else
      {
         unsigned long min,max;
         unsigned long i=0;
         for(int j=0;j<nbExclude;j++)
         {
            min=(long)floor((mExcludedRegionMin2Theta(j)-m2ThetaMin)
                                 /m2ThetaStep);
            max=(long)ceil((mExcludedRegionMax2Theta(j)-m2ThetaMin)
                                 /m2ThetaStep);
            if(min>maxPoints) break;
            if(max>maxPoints)max=maxPoints;
            for(;i<min;i++)//! min is the *beginning* of the excluded region !
            {
               tmp1 += *p4   * ((*p1)-(*p2)) * ((*p1)-(*p2));
               tmp2 += *p4++ * ((*p2)-(*p3)) * ((*p2)-(*p3));
               p1++;p2++;p3++;
            }
            p1 += max-i;
            p2 += max-i;
            p3 += max-i;
            p4 += max-i;
            i  += max-i;
         }
         for(;i<maxPoints;i++)
         {
            tmp1 += *p4   * ((*p1)-(*p2)) * ((*p1)-(*p2));
            tmp2 += *p4++ * ((*p2)-(*p3)) * ((*p2)-(*p3));
            p1++;p2++;p3++;
         }

      }
   }
   else
   {
      VFN_DEBUG_MESSAGE("PowderPattern::GetRw()",3);
      const REAL *p1, *p2, *p4;
      p1=mPowderPatternCalc.data();
      p2=mPowderPatternObs.data();
      p4=mPowderPatternWeight.data();
      const long nbExclude=mExcludedRegionMin2Theta.numElements();
      if(0==nbExclude)
      {
         for(unsigned long i=0;i<maxPoints;i++)
         {
            tmp1 += *p4   * ((*p1)-(*p2))*((*p1)-(*p2));
            tmp2 += *p4++ * (*p2) * (*p2);
            p1++;p2++;
         }
      }
      else
      {
         unsigned long min,max;
         unsigned long i=0;
         for(int j=0;j<nbExclude;j++)
         {
            min=(long)floor((mExcludedRegionMin2Theta(j)-m2ThetaMin)
                                 /m2ThetaStep);
            max=(long)ceil((mExcludedRegionMax2Theta(j)-m2ThetaMin)
                                 /m2ThetaStep);
            if(min>maxPoints) break;
            if(max>maxPoints)max=maxPoints;
            for(;i<min;i++)//! min is the *beginning* of the excluded region !
            {
               tmp1 += *p4   * ((*p1)-(*p2))*((*p1)-(*p2));
               tmp2 += *p4++ * (*p2) * (*p2);
               p1++;p2++;
            }
            p1 += max-i;
            p2 += max-i;
            p4 += max-i;
            i  += max-i;
         }
         for(;i<maxPoints;i++)
         {
            tmp1 += *p4   * ((*p1)-(*p2))*((*p1)-(*p2));
            tmp2 += *p4++ * (*p2) * (*p2);
            p1++;p2++;
         }
      }
   }
   VFN_DEBUG_MESSAGE("PowderPattern::GetRw()="<<sqrt(tmp1/tmp2),3);
   return sqrt(tmp1/tmp2);
}
REAL PowderPattern::GetIntegratedRw()const
{
   if(  (0==this->GetPowderPatternObs().numElements())
      ||(0==GetNbPowderPatternComponent()))
   {
      return 0;
   } 
   this->CalcPowderPattern();
   this->PrepareIntegratedRfactor();
   TAU_PROFILE("PowderPattern::GetIntegratedRw()","void ()",TAU_DEFAULT);
   
   REAL tmp1=0.;
   REAL tmp2=0.;
   const long numInterval=mIntegratedPatternMin.numElements();
   if(  (true==mStatisticsExcludeBackground)
      &&(mPowderPatternBackgroundCalc.numElements()>0))
   {
      const REAL *p1, *p2, *p3, *p4;
      CrystVector_REAL integratedCalc(numInterval);
      integratedCalc=0;
      CrystVector_REAL backgdCalc(numInterval);
      backgdCalc=0;
      REAL *pp1=integratedCalc.data();
      REAL *pp2=backgdCalc.data();
      for(int i=0;i<numInterval;i++)
      {
         const long max=mIntegratedPatternMax(i);
         p1=mPowderPatternCalc.data()+mIntegratedPatternMin(i);
         for(int j=mIntegratedPatternMin(i);j<=max;j++) *pp1 += *p1++;
         pp1++;
         p1=mPowderPatternBackgroundCalc.data()+mIntegratedPatternMin(i);
         for(int j=mIntegratedPatternMin(i);j<=max;j++) *pp2 += *p1++;
         pp2++;
      }
      
      p1=integratedCalc.data();
      p2=mIntegratedObs.data();
      p3=backgdCalc.data();
      if(mIntegratedWeight.numElements()==0) p4=mIntegratedWeightObs.data();
      else p4=mIntegratedWeight.data();
      VFN_DEBUG_MESSAGE("PowderPattern::GetIntegratedRw():Exclude Backgd",4);
      for(long i=0;i<numInterval;i++)
      {
         tmp1 += *p4   * ((*p1)-(*p2)) * ((*p1)-(*p2));
         tmp2 += *p4++ * ((*p2)-(*p3)) * ((*p2)-(*p3));
         //cout <<i<<": " <<mIntegratedPatternMin(i)<<"->"<<mIntegratedPatternMax(i)
         //     <<" "<< tmp1 << " "<<tmp2 << " " << *p1 <<" "<<*p2<<" "<<*p3<<" "<<*(p4-1) <<endl;
         p1++;p2++;p3++;
      }
   } // Exclude Background ?
   else
   {
      const REAL *p1, *p2, *p4;
      CrystVector_REAL integratedCalc(numInterval);
      integratedCalc=0;
      REAL *pp1=integratedCalc.data();
      for(int i=0;i<numInterval;i++)
      {
         const long max=mIntegratedPatternMax(i);
         p1=mPowderPatternCalc.data()+mIntegratedPatternMin(i);
         for(int j=mIntegratedPatternMin(i);j<=max;j++) *pp1 += *p1++;
         pp1++;
      }
      p1=integratedCalc.data();
      p2=mIntegratedObs.data();
      if(mIntegratedWeight.numElements()==0) p4=mIntegratedWeightObs.data();
      else p4=mIntegratedWeight.data();
      VFN_DEBUG_MESSAGE("PowderPattern::GetIntegratedRw()",4);
      for(long i=0;i<numInterval;i++)
      {
         tmp1 += *p4   * ((*p1)-(*p2))*((*p1)-(*p2));
         tmp2 += *p4++ * (*p2) * (*p2);
         //cout <<i<<":"<< tmp1 << " "<<tmp2 << " " << *p1 <<" "<<*p2<<endl;
         p1++;p2++;
      }
   }
   
   VFN_DEBUG_MESSAGE("PowderPattern::GetIntegratedRw()="<<sqrt(tmp1/tmp2),4);
   //cout << FormatVertVector<REAL>(mPowderPatternCalc,mPowderPatternObs);
   //this->SavePowderPattern("refinedPattern.out");
   //abort();
   return sqrt(tmp1/tmp2);
}

REAL PowderPattern::GetChi2()const
{
   if(  (0==this->GetPowderPatternObs().numElements())
      ||(0==GetNbPowderPatternComponent()))
   {
      mChi2=0.;
      return mChi2;
   }
   this->CalcNbPointUsed();
   if(mClockChi2>mClockMaster) return mChi2;
   
   if(0 == mOptProfileIntegration.GetChoice())
   {
      this->CalcPowderPatternIntegrated();
      if(  (mClockChi2>mClockPowderPatternPar)
         &&(mClockChi2>mClockScaleFactor)
         &&(mClockChi2>mClockPowderPatternIntegratedCalc)) return mChi2;
   }
   else
   {
      this->CalcPowderPattern();
      if(  (mClockChi2>mClockPowderPatternPar)
         &&(mClockChi2>mClockScaleFactor)
         &&(mClockChi2>mClockPowderPatternCalc)) return mChi2;
   }
   // We want the best scale factor
   if(0 == mOptProfileIntegration.GetChoice()) this->FitScaleFactorForIntegratedRw();
   else this->FitScaleFactorForRw();

   TAU_PROFILE("PowderPattern::GetChi2()","void ()",TAU_DEFAULT);
   
   VFN_DEBUG_ENTRY("PowderPattern::GetChi2()",3);
   
   const unsigned long maxPoints=mNbPointUsed;

   mChi2=0.;
   mChi2LikeNorm=0.;
   if(0 == mOptProfileIntegration.GetChoice())
   {// Integrated profiles
      VFN_DEBUG_MESSAGE("PowderPattern::GetChi2():Integrated profiles",3);
      const REAL *p1, *p2, *p3;
      p1=mPowderPatternIntegratedCalc.data();
      p2=mIntegratedObs.data();
      if(mIntegratedWeight.numElements()==0) p3=mIntegratedWeightObs.data();
      else p3=mIntegratedWeight.data();
      double weightProd=1.;
      VFN_DEBUG_MESSAGE("PowderPattern::GetIntegratedRw()",4);
      for(unsigned long i=0;i<mNbIntegrationUsed;)
      {
         // group weights to avoid computing too many log()
         // group only a limited number to avoid underflow...
         for(unsigned long j=0;j<32;++j)
         {
            mChi2 += *p3 * ((*p1)-(*p2))*((*p1)-(*p2));
            if(*p3>0) weightProd *= *p3;
            p1++;p2++;p3++;
            if(++i == mNbIntegrationUsed) break;
         }
         mChi2LikeNorm -= log(weightProd);
         weightProd=1.;
      }
   }
   else
   {// "Full" profiles
      VFN_DEBUG_MESSAGE("PowderPattern::GetChi2()Integrated profiles",3);
      const REAL *p1, *p2, *p3;
      p1=mPowderPatternCalc.data();
      p2=mPowderPatternObs.data();
      p3=mPowderPatternWeight.data();
      const long nbExclude=mExcludedRegionMin2Theta.numElements();
      if(0==nbExclude)
      {
         for(unsigned long i=0;i<maxPoints;i++)
         {
            mChi2 += *p3 * ((*p1)-(*p2))*((*p1)-(*p2));
            if(*p3<=0) p3++;
            else mChi2LikeNorm -= log(*p3++);
            p1++;p2++;
         }
      }
      else
      {
         unsigned long min,max;
         unsigned long i=0;
         for(int j=0;j<nbExclude;j++)
         {
            min=(long)floor((mExcludedRegionMin2Theta(j)-m2ThetaMin)
                                 /m2ThetaStep);
            max=(long)ceil((mExcludedRegionMax2Theta(j)-m2ThetaMin)
                                 /m2ThetaStep);
            if(min>maxPoints) break;
            if(max>maxPoints)max=maxPoints;
            for(;i<min;i++)//! min is the *beginning* of the excluded region !
            {
               mChi2 += *p3 * ((*p1)-(*p2))*((*p1)-(*p2));
               if(*p3<=0) p3++;
               else mChi2LikeNorm -= log(*p3++);
               p1++;p2++;
            }
            p1 += max-i;
            p2 += max-i;
            p3 += max-i;
            i  += max-i;
         }
         for(;i<maxPoints;i++)
         {
            mChi2 += *p3 * ((*p1)-(*p2))*((*p1)-(*p2));
            if(*p3<=0) p3++;
            else mChi2LikeNorm -= log(*p3++);
            p1++;p2++;
         }
      }
   }
   mChi2LikeNorm/=2;
   VFN_DEBUG_MESSAGE("Chi^2="<<mChi2<<", log(norm)="<<mChi2LikeNorm,3)
   mClockChi2.Click();
   VFN_DEBUG_EXIT("PowderPattern::GetChi2()="<<mChi2,3);
   return mChi2;
}

void PowderPattern::FitScaleFactorForR()const
{
   if(  (0==this->GetPowderPatternObs().numElements())
      ||(0==GetNbPowderPatternComponent()))
   {
      return ;
   } 
   this->CalcPowderPattern();
   TAU_PROFILE("PowderPattern::FitScaleFactorForR()","void ()",TAU_DEFAULT);
   VFN_DEBUG_ENTRY("PowderPattern::FitScaleFactorForR()",3);
   // Which components are scalable ?
      mScalableComponentIndex.resize(mPowderPatternComponentRegistry.GetNb());
      int nbScale=0;
      for(int i=0;i<mPowderPatternComponentRegistry.GetNb();i++)
      {
         if(mPowderPatternComponentRegistry.GetObj(i).IsScalable())
            mScalableComponentIndex(nbScale++)=i;
      }
   VFN_DEBUG_MESSAGE("-> Number of Scale Factors:"<<nbScale<<":Index:"<<endl<<mScalableComponentIndex,3);
   if(0==nbScale)
   {
      VFN_DEBUG_EXIT("PowderPattern::FitScaleFactorForR(): No scalable component!",3);
      return;
   }
   mScalableComponentIndex.resizeAndPreserve(nbScale);
   // prepare matrices
      mFitScaleFactorM.resize(nbScale,nbScale);
      mFitScaleFactorB.resize(nbScale,1);
      mFitScaleFactorX.resize(nbScale,1);
   // Build Matrix & Vector for LSQ
   const long nbExclude=mExcludedRegionMin2Theta.numElements();
   if(0==nbExclude)
   {
      for(int i=0;i<nbScale;i++)
      {
         for(int j=i;j<nbScale;j++)
         {
            // Here use a direct access to the powder spectrum, since
            // we know it has just been recomputed
            const REAL *p1=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(i))
                              .mPowderPatternCalc.data();
            const REAL *p2=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(j))
                              .mPowderPatternCalc.data();
            REAL m=0.;
            for(unsigned long k=0;k<mNbPointUsed;k++) m += *p1++ * *p2++;
            mFitScaleFactorM(i,j)=m;
            mFitScaleFactorM(j,i)=m;
         }
      }
      for(int i=0;i<nbScale;i++)
      {
         const REAL *p1=mPowderPatternObs.data();
         const REAL *p2=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(i))
                           .mPowderPatternCalc.data();
         REAL b=0.;
         if(mPowderPatternBackgroundCalc.numElements()<=1)
            for(unsigned long k=0;k<mNbPointUsed;k++) b += *p1++ * *p2++;
         else
         {
            const REAL *p3=mPowderPatternBackgroundCalc.data();
            for(unsigned long k=0;k<mNbPointUsed;k++) b += (*p1++ - *p3++) * *p2++;
         }
         mFitScaleFactorB(i,0) =b;
      }
   }
   else
   {
      unsigned long min,max;
      for(int i=0;i<nbScale;i++)
      {
         for(int j=i;j<nbScale;j++)
         {
            // Here use a direct access to the powder spectrum, since
            // we know it has just been recomputed
            const REAL *p1=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(i))
                              .mPowderPatternCalc.data();
            const REAL *p2=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(j))
                              .mPowderPatternCalc.data();
            REAL m=0.;
            unsigned long l=0;
            for(int k=0;k<nbExclude;k++)
            {
               min=(unsigned long)floor((mExcludedRegionMin2Theta(k)-m2ThetaMin)
                                    /m2ThetaStep);
               max=(unsigned long)ceil((mExcludedRegionMax2Theta(k)-m2ThetaMin)
                                    /m2ThetaStep);
               if(min>mNbPointUsed) break;
               if(max>mNbPointUsed)max=mNbPointUsed;
               //! min is the *beginning* of the excluded region
               for(;l<min;l++) m += *p1++ * *p2++;
               p1 += max-l;
               p2 += max-l;
               l  = max;
            }
            for(;l<mNbPointUsed;l++) m += *p1++ * *p2++;
            mFitScaleFactorM(i,j)=m;
            mFitScaleFactorM(j,i)=m;
         }
      }
      for(int i=0;i<nbScale;i++)
      {
         const REAL *p1=mPowderPatternObs.data();
         const REAL *p2=mPowderPatternComponentRegistry
                           .GetObj(mScalableComponentIndex(i))
                              .mPowderPatternCalc.data();
         REAL b=0.;
         unsigned long l=0;
         if(mPowderPatternBackgroundCalc.numElements()<=1)
         {
            for(int k=0;k<nbExclude;k++)
            {
               min=(long)floor((mExcludedRegionMin2Theta(k)-m2ThetaMin)
                                    /m2ThetaStep);
               max=(long)ceil((mExcludedRegionMax2Theta(k)-m2ThetaMin)
                                    /m2ThetaStep);
               if(min>mNbPointUsed) break;
               if(max>mNbPointUsed)max=mNbPointUsed;
               //! min is the *beginning* of the excluded region
               for(;l<min;l++) b += *p1++ * *p2++;
               p1 += max-l;
               p2 += max-l;
               l  = max;
            }
            for(;l<mNbPointUsed;l++) b += *p1++ * *p2++;
         }
         else
         {
            const REAL *p3=mPowderPatternBackgroundCalc.data();
            for(int k=0;k<nbExclude;k++)
            {
               min=(long)floor((mExcludedRegionMin2Theta(k)-m2ThetaMin)
                                    /m2ThetaStep);
               max=(long)ceil((mExcludedRegionMax2Theta(k)-m2ThetaMin)
                                    /m2ThetaStep);
               if(min>mNbPointUsed) break;
               if(max>mNbPointUsed)max=mNbPointUsed;
               //! min is the *beginning* of the excluded region
               for(;l<min;l++) b += (*p1++ - *p3++) * *p2++;
               p1 += max-l;
               p2 += max-l;
               l  = max;
            }
            for(;l<mNbPointUsed;l++) b += (*p1++ - *p3++) * *p2++;
         }
         mFitScaleFactorB(i,0) =b;
      }
   }
   if(1==nbScale) mFitScaleFactorX=mFitScaleFactorB(0)/mFitScaleFactorM(0);
   else
      mFitScaleFactorX=product(InvertMatrix(mFitScaleFactorM),mFitScaleFactorB);
   VFN_DEBUG_MESSAGE("B, M, X"<<endl<<mFitScaleFactorB<<endl<<mFitScaleFactorM<<endl<<mFitScaleFactorX,2)
   for(int i=0;i<nbScale;i++)
   {
      const REAL * p1=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(i))
                        .mPowderPatternCalc.data();
      REAL * p0 = mPowderPatternCalc.data();
      const REAL s = mFitScaleFactorX(i)
                       -mScaleFactor(mScalableComponentIndex(i));
      for(unsigned long j=0;j<mNbPointUsed;j++) *p0++ += s * *p1++;
      VFN_DEBUG_MESSAGE("-> Old:"<<mScaleFactor(mScalableComponentIndex(i)) <<" Change:"<<mFitScaleFactorX(i),2);
      mScaleFactor(mScalableComponentIndex(i)) = mFitScaleFactorX(i);
      mClockScaleFactor.Click();
      mClockPowderPatternCalc.Click();//we *did* correct the spectrum
   }
   VFN_DEBUG_EXIT("PowderPattern::FitScaleFactorForR():End",3);
}

void PowderPattern::FitScaleFactorForIntegratedR()const
{
   if(  (0==this->GetPowderPatternObs().numElements())
      ||(0==GetNbPowderPatternComponent()))
   {
      return ;
   } 
   this->CalcPowderPattern();
   this->PrepareIntegratedRfactor();
   VFN_DEBUG_ENTRY("PowderPattern::FitScaleFactorForIntegratedR()",3);
   TAU_PROFILE("PowderPattern::FitScaleFactorForIntegratedR()","void ()",TAU_DEFAULT);
   // Which components are scalable ?
      mScalableComponentIndex.resize(mPowderPatternComponentRegistry.GetNb());
      int nbScale=0;
      for(int i=0;i<mPowderPatternComponentRegistry.GetNb();i++)
      {
         if(mPowderPatternComponentRegistry.GetObj(i).IsScalable())
            mScalableComponentIndex(nbScale++)=i;
      }
   VFN_DEBUG_MESSAGE("-> Number of Scale Factors:"<<nbScale<<":Index:"<<endl<<mScalableComponentIndex,2);
   if(0==nbScale)
   {
      VFN_DEBUG_EXIT("PowderPattern::FitScaleFactorForIntegratedR(): No scalable component!",3);
      return;
   }
   mScalableComponentIndex.resizeAndPreserve(nbScale);
   // prepare matrices
      mFitScaleFactorM.resize(nbScale,nbScale);
      mFitScaleFactorB.resize(nbScale,1);
      mFitScaleFactorX.resize(nbScale,1);
   // Build Matrix & Vector for LSQ
   VFN_DEBUG_MESSAGE("PowderPattern::FitScaleFactorForIntegratedR():1",2);
      const long numInterval=mIntegratedPatternMin.numElements();
      CrystVector_REAL *integratedCalc= new CrystVector_REAL[nbScale];
      for(int i=0;i<nbScale;i++)
      {
         integratedCalc[i].resize(numInterval);
         
         // Here use a direct access to the powder spectrum, since
         // we know it has just been recomputed
         const REAL *p1=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(i))
                          .mPowderPatternCalc.data();
         
         REAL *p2=integratedCalc[i].data();
         for(int j=0;j<numInterval;j++)
         {
            const long max=mIntegratedPatternMax(j);
            p1=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(i))
                          .mPowderPatternCalc.data()+mIntegratedPatternMin(j);
            *p2=0;           
            for(int k=mIntegratedPatternMin(j);k<=max;k++) *p2 += *p1++;
            //cout <<"Calc#"<<i<<":"<< mIntegratedPatternMin(j) << " "
            //     <<mIntegratedPatternMax(j)<<" "
            //     << *p2<<endl;
            p2++;
         }
      }
   VFN_DEBUG_MESSAGE("PowderPattern::FitScaleFactorForIntegratedR():2",2);
      CrystVector_REAL backdIntegrated(numInterval);
      if(mPowderPatternBackgroundCalc.numElements()>1)
      {   
         const REAL *p1;
         REAL *p2=backdIntegrated.data();
         for(int j=0;j<numInterval;j++)
         {
            const long max=mIntegratedPatternMax(j);
            p1=mPowderPatternBackgroundCalc.data()+mIntegratedPatternMin(j);
            *p2=0;
            for(int k=mIntegratedPatternMin(j);k<=max;k++) *p2 += *p1++;
            //cout <<"Backgd:"<< mIntegratedPatternMin(j) << " "
            //     <<mIntegratedPatternMax(j)<<" "
            //     << *p2<<endl;
            p2++;
         }
      }
      
   //if(mPowderPatternBackgroundCalc.numElements()<=1)
   //   cout<< FormatVertVector<REAL>(integratedCalc[0],mIntegratedObs,mIntegratedWeight,backdIntegrated)<<endl;
   //else
   //   cout<< FormatVertVector<REAL>(integratedCalc[0],mIntegratedObs,mIntegratedWeight)<<endl;
   VFN_DEBUG_MESSAGE("PowderPattern::FitScaleFactorForIntegratedR():3",2);
      for(int i=0;i<nbScale;i++)
      {
         for(int j=i;j<nbScale;j++)
         {
            const REAL *p1=integratedCalc[i].data();
            const REAL *p2=integratedCalc[j].data();
            REAL m=0.;
            for(long k=0;k<numInterval;k++)
            {
               m += *p1++ * *p2++;
               //cout <<"M:"<< mIntegratedPatternMin(k) << " "<<mIntegratedPatternMax(k)<<" "<<m<<endl;
            }
            mFitScaleFactorM(i,j)=m;
            mFitScaleFactorM(j,i)=m;
         }
      }
   VFN_DEBUG_MESSAGE("PowderPattern::FitScaleFactorForIntegratedR():4",2);
      for(int i=0;i<nbScale;i++)
      {
         const REAL *p1=mIntegratedObs.data();
         const REAL *p2=integratedCalc[i].data();
         REAL b=0.;
         if(mPowderPatternBackgroundCalc.numElements()<=1)
            for(long k=0;k<numInterval;k++)
            {
               b += *p1++ * *p2++;
               //cout<<"B:"<<mIntegratedPatternMin(k)<<" "<<mIntegratedPatternMax(k)<<" "<<b<<endl;
            }
         else
         {
            const REAL *p3=backdIntegrated.data();
            for(long k=0;k<numInterval;k++) 
            {
               //cout<<"B(minus backgd):"<<mIntegratedPatternMin(k)<<" "
               //    <<mIntegratedPatternMax(k)<<" "
               //    <<*p1<<" "<<*p2<<" "<<*p3<<" "<<b<<endl;
               b += (*p1++ - *p3++) * *p2++;
            }
         }
         mFitScaleFactorB(i,0) =b;
      }
   VFN_DEBUG_MESSAGE("PowderPattern::FitScaleFactorForIntegratedR():5",2);
   
   if(1==nbScale) mFitScaleFactorX=mFitScaleFactorB(0)/mFitScaleFactorM(0);
   else
      mFitScaleFactorX=product(InvertMatrix(mFitScaleFactorM),mFitScaleFactorB);
   VFN_DEBUG_MESSAGE("B, M, X"<<endl<<mFitScaleFactorB<<endl<<mFitScaleFactorM<<endl<<mFitScaleFactorX,3)
   for(int i=0;i<nbScale;i++)
   {
      const REAL * p1=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(i))
                        .mPowderPatternCalc.data();
      REAL * p0 = mPowderPatternCalc.data();
      const REAL s = mFitScaleFactorX(i)
                       -mScaleFactor(mScalableComponentIndex(i));
      for(unsigned long j=0;j<mNbPointUsed;j++) *p0++ += s * *p1++;
      VFN_DEBUG_MESSAGE("-> Old:"<<mScaleFactor(mScalableComponentIndex(i)) <<" New:"<<mFitScaleFactorX(i),3);
      mScaleFactor(mScalableComponentIndex(i)) = mFitScaleFactorX(i);
      mClockScaleFactor.Click();
      mClockPowderPatternCalc.Click();//we *did* correct the spectrum
   }
   delete[] integratedCalc;
   VFN_DEBUG_EXIT("PowderPattern::FitScaleFactorForIntegratedR():End",3);
}

void PowderPattern::FitScaleFactorForRw()const
{
   if(  (0==this->GetPowderPatternObs().numElements())
      ||(0==GetNbPowderPatternComponent()))
   {
      return ;
   } 
   this->CalcPowderPattern();
   TAU_PROFILE("PowderPattern::FitScaleFactorForRw()","void ()",TAU_DEFAULT);
   VFN_DEBUG_ENTRY("PowderPattern::FitScaleFactorForRw()",3);
   this->CalcPowderPattern();
   //:TODO: take into account excluded regions...
   // Which components are scalable ?
      mScalableComponentIndex.resize(mPowderPatternComponentRegistry.GetNb());
      int nbScale=0;
      for(int i=0;i<mPowderPatternComponentRegistry.GetNb();i++)
      {
         if(mPowderPatternComponentRegistry.GetObj(i).IsScalable())
            mScalableComponentIndex(nbScale++)=i;
      }
   VFN_DEBUG_MESSAGE("-> Number of Scale Factors:"<<nbScale<<":Index:"<<endl<<mScalableComponentIndex,2);
   if(0==nbScale)
   {
      VFN_DEBUG_EXIT("PowderPattern::FitScaleFactorForRw(): No scalable component!",3);
      return;
   }
   mScalableComponentIndex.resizeAndPreserve(nbScale);
   // prepare matrices
      mFitScaleFactorM.resize(nbScale,nbScale);
      mFitScaleFactorB.resize(nbScale,1);
      mFitScaleFactorX.resize(nbScale,1);
   // Build Matrix & Vector for LSQ
   const long nbExclude=mExcludedRegionMin2Theta.numElements();
   if(0==nbExclude)
   {
      for(int i=0;i<nbScale;i++)
      {
         for(int j=i;j<nbScale;j++)
         {
            // Here use a direct access to the powder spectrum, since
            // we know it has just been recomputed
            const REAL *p1=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(i))
                              .mPowderPatternCalc.data();
            const REAL *p2=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(j))
                              .mPowderPatternCalc.data();
            const REAL *p3=mPowderPatternWeight.data();
            REAL m=0.;
            for(unsigned long k=0;k<mNbPointUsed;k++) m += *p1++ * *p2++ * *p3++;
            mFitScaleFactorM(i,j)=m;
            mFitScaleFactorM(j,i)=m;
         }
      }
      for(int i=0;i<nbScale;i++)
      {
         const REAL *p1=mPowderPatternObs.data();
         const REAL *p2=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(i))
                           .mPowderPatternCalc.data();
         const REAL *p3=mPowderPatternWeight.data();
         REAL b=0.;
         if(mPowderPatternBackgroundCalc.numElements()<=1)
            for(unsigned long k=0;k<mNbPointUsed;k++) b += *p1++ * *p2++ * *p3++;
         else
         {
            const REAL *p4=mPowderPatternBackgroundCalc.data();
            for(unsigned long k=0;k<mNbPointUsed;k++)
               b += (*p1++ - *p4++) * *p2++ * *p3++;
         }
         mFitScaleFactorB(i,0) =b;
      }
   }
   else
   {
      unsigned long min,max;
      for(int i=0;i<nbScale;i++)
      {
         for(int j=i;j<nbScale;j++)
         {
            // Here use a direct access to the powder spectrum, since
            // we know it has just been recomputed
            const REAL *p1=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(i))
                              .mPowderPatternCalc.data();
            const REAL *p2=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(j))
                              .mPowderPatternCalc.data();
            const REAL *p3=mPowderPatternWeight.data();
            REAL m=0.;
            unsigned long l=0;
            for(int k=0;k<nbExclude;k++)
            {
               min=(unsigned long)floor((mExcludedRegionMin2Theta(k)-m2ThetaMin)
                                    /m2ThetaStep);
               max=(unsigned long)ceil((mExcludedRegionMax2Theta(k)-m2ThetaMin)
                                    /m2ThetaStep);
               if(min>mNbPointUsed) break;
               if(max>mNbPointUsed)max=mNbPointUsed;
               //! min is the *beginning* of the excluded region
               for(;l<min;l++) m += *p1++ * *p2++ * *p3++;
               p1 += max-l;
               p2 += max-l;
               p3 += max-l;
               l  = max;
            }
            for(;l<mNbPointUsed;l++) m += *p1++ * *p2++ * *p3++;
            mFitScaleFactorM(i,j)=m;
            mFitScaleFactorM(j,i)=m;
         }
      }
      for(int i=0;i<nbScale;i++)
      {
         const REAL *p1=mPowderPatternObs.data();
         const REAL *p2=mPowderPatternComponentRegistry
                           .GetObj(mScalableComponentIndex(i))
                              .mPowderPatternCalc.data();
         const REAL *p3=mPowderPatternWeight.data();
         REAL b=0.;
         unsigned long l=0;
         if(mPowderPatternBackgroundCalc.numElements()<=1)
         {
            for(int k=0;k<nbExclude;k++)
            {
               min=(long)floor((mExcludedRegionMin2Theta(k)-m2ThetaMin)
                                    /m2ThetaStep);
               max=(long)ceil((mExcludedRegionMax2Theta(k)-m2ThetaMin)
                                    /m2ThetaStep);
               if(min>mNbPointUsed) break;
               if(max>mNbPointUsed)max=mNbPointUsed;
               //! min is the *beginning* of the excluded region
               for(;l<min;l++) b += *p1++ * *p2++ * *p3++;
               p1 += max-l;
               p2 += max-l;
               p3 += max-l;
               l  = max;
            }
            for(;l<mNbPointUsed;l++) b += *p1++ * *p2++ * *p3++;
         }
         else
         {
            const REAL *p4=mPowderPatternBackgroundCalc.data();
            for(int k=0;k<nbExclude;k++)
            {
               min=(long)floor((mExcludedRegionMin2Theta(k)-m2ThetaMin)
                                    /m2ThetaStep);
               max=(long)ceil((mExcludedRegionMax2Theta(k)-m2ThetaMin)
                                    /m2ThetaStep);
               if(min>mNbPointUsed) break;
               if(max>mNbPointUsed)max=mNbPointUsed;
               //! min is the *beginning* of the excluded region
               for(;l<min;l++) b += (*p1++ - *p4++) * *p2++ * *p3++;
               p1 += max-l;
               p2 += max-l;
               p3 += max-l;
               l  = max;
            }
            for(;l<mNbPointUsed;l++) b += (*p1++ - *p4++) * *p2++ * *p3++;
         }
         mFitScaleFactorB(i,0) =b;
      }
   }
   if(1==nbScale) mFitScaleFactorX=mFitScaleFactorB(0)/mFitScaleFactorM(0);
   else
      mFitScaleFactorX=product(InvertMatrix(mFitScaleFactorM),mFitScaleFactorB);
   VFN_DEBUG_MESSAGE("B, M, X"<<endl<<mFitScaleFactorB<<endl<<mFitScaleFactorM<<endl<<mFitScaleFactorX,2)
   for(int i=0;i<nbScale;i++)
   {
      const REAL * p1=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(i))
                        .mPowderPatternCalc.data();
      REAL * p0 = mPowderPatternCalc.data();
      const REAL s = mFitScaleFactorX(i)
                       -mScaleFactor(mScalableComponentIndex(i));
      for(unsigned long j=0;j<mNbPointUsed;j++) *p0++ += s * *p1++;
      VFN_DEBUG_MESSAGE("-> Old:"<<mScaleFactor(mScalableComponentIndex(i)) <<" Change:"<<mFitScaleFactorX(i),3);
      mScaleFactor(mScalableComponentIndex(i)) = mFitScaleFactorX(i);
      mClockScaleFactor.Click();
      mClockPowderPatternCalc.Click();//we *did* correct the spectrum
   }
   VFN_DEBUG_EXIT("PowderPattern::FitScaleFactorForRw():End",3);
}

void PowderPattern::FitScaleFactorForIntegratedRw()const
{
   if(  (0==this->GetPowderPatternObs().numElements())
      ||(0==GetNbPowderPatternComponent()))
   {
      return ;
   }
   this->CalcPowderPatternIntegrated();
   if(mClockScaleFactor>mClockPowderPatternIntegratedCalc)return;
   VFN_DEBUG_ENTRY("PowderPattern::FitScaleFactorForIntegratedRw()",3);
   TAU_PROFILE("PowderPattern::FitScaleFactorForIntegratedRw()","void ()",TAU_DEFAULT);
   // Which components are scalable ? Which scalable calculated components have a non-null variance ?
      mScalableComponentIndex.resize(mPowderPatternComponentRegistry.GetNb());
      CrystVector_REAL vScalableVarianceIndex(mPowderPatternComponentRegistry.GetNb());
      int nbScale=0;
      int nbVarCalc=0;
      for(int i=0;i<mPowderPatternComponentRegistry.GetNb();i++)
      {
         if(mPowderPatternComponentRegistry.GetObj(i).IsScalable())
            mScalableComponentIndex(nbScale++)=i;
         if(mPowderPatternComponentRegistry.GetObj(i).HasPowderPatternCalcVariance())
         {
            if(0!=mPowderPatternComponentRegistry.GetObj(i)
                    .GetPowderPatternIntegratedCalcVariance().first->numElements())
                        vScalableVarianceIndex(nbVarCalc++)=i;
         }
      }
   VFN_DEBUG_MESSAGE("-> Number of Scale Factors:"<<nbScale<<"("<<nbVarCalc
                     <<"with variance). Index:"<<endl<<mScalableComponentIndex,2);
   if(0==nbScale)
   {
      VFN_DEBUG_EXIT("PowderPattern::FitScaleFactorForIntegratedRw(): No scalable component!",3);
      return;
   }
   bool again;
   unsigned int ctagain=0;
   RECALC_SCALE_FACTOR_VARIANCE_FitScaleFactorForIntegratedRw:
   if(false)//if((nbScale==1)&&(nbVarCalc==1))
   {// Special case when using ML error, the scale appears also in the weight so we have to
    // use a 2nd-degree equation (ax^2+bx+c=0) to get the scale factor.
      double a=0.,b=0.,c=0.;// possible overflows...
      REAL newscale;
      {
         const REAL *p1=mIntegratedObs.data();
         const REAL *p2=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(0))
                              .GetPowderPatternIntegratedCalc().first->data();
         const REAL *p1v=mIntegratedVarianceObs.data();
         const REAL *p2v=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(0))
                              .GetPowderPatternIntegratedCalcVariance().first->data();
         if(mPowderPatternBackgroundIntegratedCalc.numElements()<=1)
         {
            for(unsigned long k=0;k<mNbIntegrationUsed;k++)
            {
               a += *p2v * *p1 * *p2;
               b += *p2 * *p2 * *p1v - *p1 * *p1 * *p2v;
               c -= *p1 * *p2 * *p1v;
               p1++;p2++;p1v++;p2v++;
            }
         }
         else
         {
            const REAL *p3=mPowderPatternBackgroundIntegratedCalc.data();
            for(unsigned long k=0;k<mNbIntegrationUsed;k++) 
            {
               a += *p2v * (*p1 - *p3) * *p2;
               b += *p2 * *p2 * *p1v - (*p1 - *p3) * (*p1 - *p3) * *p2v;
               c -= (*p1 - *p3) * *p2 * *p1v;
               p1++;p2++;p1v++;p2v++;p3++;
            }
         }
         // Only one >0 solution to the 2nd degree equation
         newscale=(REAL)((-b+sqrt(b*b-4*a*c))/(2*a));
      }
      {// Store new scale, and correct old calculated pattern
         const REAL s = newscale-mScaleFactor(mScalableComponentIndex(0));
         const REAL s2 = newscale*newscale
                         -mScaleFactor(mScalableComponentIndex(0))
                         *mScaleFactor(mScalableComponentIndex(0));
         REAL * p0 = mPowderPatternIntegratedCalc.data();
         const REAL * p1=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(0))
                           .GetPowderPatternIntegratedCalc().first->data();
         REAL * p0v = mPowderPatternVarianceIntegrated.data();
         const REAL * p1v=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(0))
                           .GetPowderPatternIntegratedCalcVariance().first->data();
         REAL * p0w = mIntegratedWeight.data();
         for(unsigned long j=0;j<mNbIntegrationUsed;j++)
         {
            *p0++  += s * *p1++;
            *p0v += s2 * *p1v++;
            if(*p0v <=0) {*p0w++ =0;p0v++;}else *p0w++ = 1. / *p0v++;
         }
      }
      mScaleFactor(mScalableComponentIndex(0)) = newscale;
      mClockScaleFactor.Click();
      mClockPowderPatternIntegratedCalc.Click();//we *did* correct the spectrum
   }
   else
   {
      again=false;
      mScalableComponentIndex.resizeAndPreserve(nbScale);
      // prepare matrices
         mFitScaleFactorM.resize(nbScale,nbScale);
         mFitScaleFactorB.resize(nbScale,1);
         mFitScaleFactorX.resize(nbScale,1);
      // Build Matrix & Vector for LSQ
      VFN_DEBUG_MESSAGE("PowderPattern::FitScaleFactorForIntegratedRw():1",2);
         vector<const CrystVector_REAL*> integratedCalc;
         for(int i=0;i<nbScale;i++)
         {
            integratedCalc.push_back(mPowderPatternComponentRegistry.
                                        GetObj(mScalableComponentIndex(i))
                                           .GetPowderPatternIntegratedCalc().first);
         }
      VFN_DEBUG_MESSAGE("PowderPattern::FitScaleFactorForIntegratedRw():3",2);
         for(int i=0;i<nbScale;i++)
         {
            for(int j=i;j<nbScale;j++)
            {
               const REAL *p1=integratedCalc[i]->data();
               const REAL *p2=integratedCalc[j]->data();
               const REAL *p3;
               if(mIntegratedWeight.numElements()==0)
               {
                  p3=mIntegratedWeightObs.data();
                  if(ctagain>0) cout <<"ctagain="<<ctagain<<", using mIntegratedWeightObs"<<endl;
               }
               else
               {
                  p3=mIntegratedWeight.data();
                  if(ctagain>0) cout <<"ctagain="<<ctagain<<", using mIntegratedWeight"<<endl;
               }
               REAL m=0.;
               for(unsigned long k=0;k<mNbIntegrationUsed;k++)
               {
                  m += *p1++ * *p2++ * *p3++;
               }
               mFitScaleFactorM(i,j)=m;
               mFitScaleFactorM(j,i)=m;
            }
         }
      VFN_DEBUG_MESSAGE("PowderPattern::FitScaleFactorForIntegratedRw():4",2);
         for(int i=0;i<nbScale;i++)
         {
            const REAL *p1=mIntegratedObs.data();
            const REAL *p2=integratedCalc[i]->data();
            const REAL *p4;
            if(mIntegratedWeight.numElements()==0) p4=mIntegratedWeightObs.data();
            else p4=mIntegratedWeight.data();
            REAL b=0.;
            if(mPowderPatternBackgroundIntegratedCalc.numElements()<=1)
            {
               for(unsigned long k=0;k<mNbIntegrationUsed;k++)
               {
                  b += *p1++ * *p2++ * *p4++;
                  //cout<<"B:"<<mIntegratedPatternMin(k)<<" "<<mIntegratedPatternMax(k)<<" "<<b<<endl;
               }
            }
            else
            {
               const REAL *p3=mPowderPatternBackgroundIntegratedCalc.data();
               for(unsigned long k=0;k<mNbIntegrationUsed;k++) 
               {
                  //cout<<"B(minus backgd):"<<mIntegratedPatternMin(k)<<" "
                  //    <<mIntegratedPatternMax(k)<<" "
                  //    <<*p1<<" "<<*p2<<" "<<*p3<<" "<<b<<endl;
                  b += (*p1++ - *p3++) * *p2++ * *p4++;
               }
            }
            mFitScaleFactorB(i,0) =b;
         }
      VFN_DEBUG_MESSAGE("PowderPattern::FitScaleFactorForIntegratedRw():5",2);

      if(1==nbScale) mFitScaleFactorX=mFitScaleFactorB(0)/mFitScaleFactorM(0);
      else if(1<nbScale)
         mFitScaleFactorX=product(InvertMatrix(mFitScaleFactorM),mFitScaleFactorB);
      VFN_DEBUG_MESSAGE("B, M, X"<<endl<<mFitScaleFactorB<<endl<<mFitScaleFactorM<<endl<<mFitScaleFactorX,3)
      //Correct the variance, if necessary
      if(nbVarCalc>0)
      {
         //if(ctagain>0) cout <<"ctgain, sumvar="<<log(mPowderPatternVarianceIntegrated.sum());
         for(int i=0;i<nbScale;i++)
         {
            if(mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(i)).HasPowderPatternCalcVariance())
            {
               if(0!=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(i))
                     .GetPowderPatternIntegratedCalcVariance().first->numElements())
               {
                  const REAL * p1=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(i))
                                    .GetPowderPatternIntegratedCalcVariance().first->data();
                  //cout <<",sumvar(i)="<<log(mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(i))
                  //                    .GetPowderPatternIntegratedCalcVariance().first->sum());
                  REAL * p0 = mPowderPatternVarianceIntegrated.data();
                  const REAL s2 = mFitScaleFactorX(i)*mFitScaleFactorX(i)
                                   -mScaleFactor(mScalableComponentIndex(i))
                                    *mScaleFactor(mScalableComponentIndex(i));
                  for(unsigned long j=0;j<mNbIntegrationUsed;j++) *p0++ += s2 * *p1++;
               }
            }
         }
         //if(ctagain>0) cout <<" ->"<<log(mPowderPatternVarianceIntegrated.sum())
         //                   <<" , sumobsvar="<<log(mIntegratedVarianceObs.sum())<<endl;
         REAL *p0 = mIntegratedWeight.data();
         const REAL *p1=mPowderPatternVarianceIntegrated.data();
         for(unsigned long j=0;j<mNbIntegrationUsed;j++)
            if(*p1 <=0) {*p0++ =0;p1++;}
            else *p0++ = 1. / *p1++;

      }
      // Correct the calculated integrated pattern
      for(int i=0;i<nbScale;i++)
      {
         const REAL * p1=mPowderPatternComponentRegistry.GetObj(mScalableComponentIndex(i))
                           .GetPowderPatternIntegratedCalc().first->data();
         REAL * p0 = mPowderPatternIntegratedCalc.data();
         const REAL s = mFitScaleFactorX(i)
                          -mScaleFactor(mScalableComponentIndex(i));
         if(nbVarCalc>0)
         {
            if(abs(s/mFitScaleFactorX(i))>0.001)
            {
               again=true;
                  //cout<<"log(scale) :"<<log(mScaleFactor(mScalableComponentIndex(i))) 
                  //    <<"->"<<log(mFitScaleFactorX(i))<<" Again="<<ctagain++<<endl;
            }
            if((!again)&&(ctagain>0))
            {
               VFN_DEBUG_MESSAGE("log(scale) :"<<log(mScaleFactor(mScalableComponentIndex(i))) 
                   <<"->"<<log(mFitScaleFactorX(i))<<" Again="<<ctagain,5);
            }
         }
         for(unsigned long j=0;j<mNbIntegrationUsed;j++) *p0++ += s * *p1++;
         VFN_DEBUG_MESSAGE("->log(scale) Old :"<<log(mScaleFactor(mScalableComponentIndex(i))) <<" New:"<<log(mFitScaleFactorX(i)),3);
         mScaleFactor(mScalableComponentIndex(i)) = mFitScaleFactorX(i);
      }
      
      mClockScaleFactor.Click();
      mClockPowderPatternIntegratedCalc.Click();//we *did* correct the spectrum
   }
   if(again)
      goto RECALC_SCALE_FACTOR_VARIANCE_FitScaleFactorForIntegratedRw;
   VFN_DEBUG_EXIT("PowderPattern::FitScaleFactorForIntegratedRw():End",3);
}

void PowderPattern::SetSigmaToSqrtIobs()
{
   VFN_DEBUG_MESSAGE("PowderPattern::SetSigmaToSqrtIobs()",5);
   for(long i=0;i<mPowderPatternObs.numElements();i++)
      mPowderPatternObsSigma(i)=sqrt(fabs(mPowderPatternObs(i)));
}
void PowderPattern::SetWeightToInvSigmaSq(const REAL minRelatSigma)
{
   VFN_DEBUG_MESSAGE("PowderPattern::SetWeightToInvSigmaSq()",5);
   //:KLUDGE: If less than 1e-4*max, set to 0.... Do not give weight to unobserved points
   const REAL min=MaxAbs(mPowderPatternObsSigma)*minRelatSigma;
   //mPowderPatternWeight.resize(mPowderPatternObsSigma.numElements());
   REAL tmp;
   for(long i=0;i<mPowderPatternObsSigma.numElements();i++)
   {
      tmp = mPowderPatternObsSigma(i);
      if(tmp<min) mPowderPatternWeight(i)= 1./min/min; 
      else  mPowderPatternWeight(i) =1./tmp/tmp;
   }
}
void PowderPattern::SetWeightToSinTheta(const REAL power)
{
   VFN_DEBUG_MESSAGE("PowderPattern::SetWeightToSinTheta()",5);
   //mPowderPatternWeight.resize(mPowderPatternObs.numElements());
   for(long i=0;i<mPowderPatternWeight.numElements();i++) 
      mPowderPatternWeight(i) =pow(sin((m2ThetaMin+ i * m2ThetaStep)/2),power);
}
void PowderPattern::SetWeightToUnit()
{
   VFN_DEBUG_MESSAGE("PowderPattern::SetWeightToSinTheta()",5);
   //mPowderPatternWeight.resize(mPowderPatternObs.numElements());
   mPowderPatternWeight=1;
}
void PowderPattern::SetWeightPolynomial(const REAL a, const REAL b,
                                        const REAL c,
                                         const REAL minRelatIobs)
{
   VFN_DEBUG_MESSAGE("PowderPattern::SetWeightPolynomial()",5);
   const REAL min=MaxAbs(mPowderPatternObs)*minRelatIobs;
   REAL tmp;
   for(long i=0;i<mPowderPatternWeight.numElements();i++)
   {
      tmp=mPowderPatternObs(i);
      if(tmp<min)
         mPowderPatternWeight(i) =   1./(a+min+b*min*min+c*min*min*min);
      else mPowderPatternWeight(i) = 1./(a+tmp+b*tmp*tmp+c*tmp*tmp*tmp);
   }
}

void PowderPattern::BeginOptimization(const bool allowApproximations,
                                      const bool enableRestraints)
{
   this->Prepare();
   if(0 == mOptProfileIntegration.GetChoice()) this->FitScaleFactorForIntegratedRw();
   else this->FitScaleFactorForRw();
   this->RefinableObj::BeginOptimization(allowApproximations,enableRestraints);
}

void PowderPattern::GlobalOptRandomMove(const REAL mutationAmplitude,
                         const RefParType *type)
{
   if(mRandomMoveIsDone) return;
   this->RefinableObj::GlobalOptRandomMove(mutationAmplitude,type);
}

void PowderPattern::Add2ThetaExcludedRegion(const REAL min2Theta,const REAL max2Theta)
{
   VFN_DEBUG_MESSAGE("PowderPattern::Add2ThetaExcludedRegion()",5)
   const int num=mExcludedRegionMin2Theta.numElements();
   if(num>0)
   {
      mExcludedRegionMin2Theta.resizeAndPreserve(num+1);
      mExcludedRegionMax2Theta.resizeAndPreserve(num+1);
   }
   else
   {
      mExcludedRegionMin2Theta.resize(1);
      mExcludedRegionMax2Theta.resize(1);
   }
   mExcludedRegionMin2Theta(num)=min2Theta;
   mExcludedRegionMax2Theta(num)=max2Theta;
   
   //ensure regions are sorted by ascending 2thetamin
   CrystVector_long subs;
   subs=SortSubs(mExcludedRegionMin2Theta);
   CrystVector_REAL tmp1,tmp2;
   tmp1=mExcludedRegionMin2Theta;
   tmp2=mExcludedRegionMax2Theta;
   for(int i=0;i<mExcludedRegionMin2Theta.numElements();i++)
   {
      mExcludedRegionMin2Theta(i)=tmp1(subs(i));
      mExcludedRegionMax2Theta(i)=tmp2(subs(i));
   }
   VFN_DEBUG_MESSAGE(FormatVertVector<REAL>(mExcludedRegionMin2Theta,mExcludedRegionMax2Theta),5)
   VFN_DEBUG_MESSAGE("PowderPattern::Add2ThetaExcludedRegion():End",5)
}

REAL PowderPattern::GetLogLikelihood()const
{
   REAL tmp=this->GetChi2();
   tmp+=mChi2LikeNorm;
   return tmp;
}

unsigned int PowderPattern::GetNbLSQFunction()const{return 1;}

const CrystVector_REAL& 
   PowderPattern::GetLSQCalc(const unsigned int) const
{return this->GetPowderPatternCalc();}

const CrystVector_REAL& 
   PowderPattern::GetLSQObs(const unsigned int) const
{return this->GetPowderPatternObs();}

const CrystVector_REAL& 
   PowderPattern::GetLSQWeight(const unsigned int) const
{return this->GetPowderPatternWeight();}

void PowderPattern::Prepare()
{
   VFN_DEBUG_MESSAGE("PowderPattern::Prepare()",5);
   for(int i=0;i<mPowderPatternComponentRegistry.GetNb();i++)
   {
      mPowderPatternComponentRegistry.GetObj(i).SetMaxSinThetaOvLambda(mMaxSinThetaOvLambda);
      mPowderPatternComponentRegistry.GetObj(i).Prepare();
   }
}
void PowderPattern::GetGeneGroup(const RefinableObj &obj,
                                CrystVector_uint & groupIndex,
                                unsigned int &first) const
{
   // One group for scales, one for theta error parameters
   unsigned int scaleIndex=0;
   unsigned int thetaIndex=0;
   VFN_DEBUG_MESSAGE("PowderPattern::GetGeneGroup()",4)
   for(long i=0;i<obj.GetNbPar();i++)
      for(long j=0;j<this->GetNbPar();j++)
         if(&(obj.GetPar(i)) == &(this->GetPar(j)))
         {
            if(this->GetPar(j).GetType()->IsDescendantFromOrSameAs(gpRefParTypeScattDataScale))
            {
               if(scaleIndex==0) scaleIndex=first++;
               groupIndex(i)=scaleIndex;
            }
            else //gpRefParTypeScattDataCorrPos
            {
               if(thetaIndex==0) thetaIndex=first++;
               groupIndex(i)=thetaIndex;
            }
         }
}

void PowderPattern::SetMaxSinThetaOvLambda(const REAL max){mMaxSinThetaOvLambda=max;}

REAL PowderPattern::GetMaxSinThetaOvLambda()const{return mMaxSinThetaOvLambda;}

const CrystVector_long& PowderPattern::GetIntegratedProfileMin()const
{
   this->PrepareIntegratedRfactor();
   return mIntegratedPatternMin;
}

const CrystVector_long& PowderPattern::GetIntegratedProfileMax()const
{
   this->PrepareIntegratedRfactor();
   return mIntegratedPatternMax;
}

const RefinableObjClock& PowderPattern::GetIntegratedProfileLimitsClock()const
{
   return mClockIntegratedFactorsPrep;
}

void PowderPattern::CalcPowderPattern() const
{
   this->CalcNbPointUsed();
   if(mClockPowderPatternCalc>mClockMaster) return;
   
   TAU_PROFILE("PowderPattern::CalcPowderPattern()","void ()",TAU_DEFAULT);
   VFN_DEBUG_ENTRY("PowderPattern::CalcPowderPattern()",3);
   if(mPowderPatternComponentRegistry.GetNb()==0)
   {
      mPowderPatternCalc.resize(mNbPoint);
      mPowderPatternCalc=0;

      mPowderPatternVariance.resize(mNbPoint);
      mPowderPatternVariance  = mPowderPatternObsSigma;
      mPowderPatternVariance *= mPowderPatternObsSigma;
      
      const REAL *p0 = mPowderPatternVariance.data();
      REAL *p1=mPowderPatternWeight.data();
      for(unsigned long j=0;j<mNbPoint;j++)
      {
         if(*p0 <=0) {*p1 =0.;}
         else *p1 = 1. / *p0;
         p0++;p1++;
      }
      VFN_DEBUG_EXIT("PowderPattern::CalcPowderPattern():no components!",3);
      return;
   }
   TAU_PROFILE_TIMER(timer1,"PowderPattern::CalcPowderPattern1()Calc components","", TAU_FIELD);
   TAU_PROFILE_TIMER(timer2,"PowderPattern::CalcPowderPattern2(Add spectrums-scaled)"\
                     ,"", TAU_FIELD);
   TAU_PROFILE_TIMER(timer3,"PowderPattern::CalcPowderPattern2(Add spectrums-backgd1)"\
                     ,"", TAU_FIELD);
   TAU_PROFILE_TIMER(timer4,"PowderPattern::CalcPowderPattern2(Add spectrums-backgd2)"\
                     ,"", TAU_FIELD);
   TAU_PROFILE_TIMER(timer5,"PowderPattern::CalcPowderPattern3(Variance)","", TAU_FIELD);
   TAU_PROFILE_START(timer1);
   for(int i=0;i<mPowderPatternComponentRegistry.GetNb();i++)
      mPowderPatternComponentRegistry.GetObj(i).CalcPowderPattern();
   TAU_PROFILE_STOP(timer1);
   VFN_DEBUG_MESSAGE("PowderPattern::CalcPowderPattern():Calculated components..",3);
   bool b=false;
   if(mClockPowderPatternCalc<mClockScaleFactor)
   {
      b=true;
   }
   else
      for(int i=0;i<mPowderPatternComponentRegistry.GetNb();i++) 
         if(mClockPowderPatternCalc<
               mPowderPatternComponentRegistry.GetObj(i).GetClockPowderPatternCalc())
         {
            b=true;
            break;
         }
      
   if(false==b)
   {
      VFN_DEBUG_EXIT("PowderPattern::CalcPowderPattern():no need to recalc",3);
      return;
   }
   mPowderPatternCalc.resize(mNbPoint);
   int nbBackgd=0;//count number of background phases
   for(int i=0;i<mPowderPatternComponentRegistry.GetNb();i++)
   {//THIS SHOULD GO FASTER (PRE-FETCHING ARRAY DATA?)
      VFN_DEBUG_MESSAGE("PowderPattern::CalcPowderPattern():Adding "<< mPowderPatternComponentRegistry.GetObj(i).GetName(),3);
      if(true==mPowderPatternComponentRegistry.GetObj(i).IsScalable())
      {
         TAU_PROFILE_START(timer2);
         if(0==i)
         {
            const REAL * p1=mPowderPatternComponentRegistry.GetObj(i)
                                 .mPowderPatternCalc.data();
            REAL * p0 = mPowderPatternCalc.data();
            const REAL s = mScaleFactor(i);
            for(unsigned long j=0;j<mNbPointUsed;j++) *p0++ = s * *p1++;
            if(!mIsbeingRefined) for(unsigned long j=mNbPointUsed;j<mNbPoint;j++) *p0++ = 0;
         }
         else
         {
            const REAL * p1=mPowderPatternComponentRegistry.GetObj(i)
                                 .mPowderPatternCalc.data();
            REAL * p0 = mPowderPatternCalc.data();
            const REAL s = mScaleFactor(i);
            for(unsigned long j=0;j<mNbPointUsed;j++) *p0++ += s * *p1++;
         }
          TAU_PROFILE_STOP (timer2);
      }
      else
      {// This is a background phase
         TAU_PROFILE_START(timer3);
         if(0==i)
         {
            const REAL * p1=mPowderPatternComponentRegistry.GetObj(i)
                                 .mPowderPatternCalc.data();
            REAL * p0 = mPowderPatternCalc.data();
            for(unsigned long j=0;j<mNbPointUsed;j++) *p0++ = *p1++;
            if(!mIsbeingRefined) for(unsigned long j=mNbPointUsed;j<mNbPoint;j++) *p0++ = 0;
         }
         else
         {
            const REAL * p1=mPowderPatternComponentRegistry.GetObj(i)
                                 .mPowderPatternCalc.data();
            REAL * p0 = mPowderPatternCalc.data();
            for(unsigned long j=0;j<mNbPointUsed;j++) *p0++ += *p1++;

         }
         TAU_PROFILE_STOP(timer3);
         TAU_PROFILE_START(timer4);
         // The following is useless if there is only one background phase...
         if(0==nbBackgd)
         {
            mPowderPatternBackgroundCalc.resize(mNbPoint);
            REAL *p0 = mPowderPatternBackgroundCalc.data();
            const REAL *p1=mPowderPatternComponentRegistry.GetObj(i)
                              .mPowderPatternCalc.data();
            for(unsigned long j=0;j<mNbPointUsed;j++) *p0++ = *p1++;
            if(!mIsbeingRefined) for(unsigned long j=mNbPointUsed;j<mNbPoint;j++) *p0++ = 0;
         }
         else
         {
            REAL *p0 = mPowderPatternBackgroundCalc.data();
            const REAL *p1=mPowderPatternComponentRegistry.GetObj(i)
                              .mPowderPatternCalc.data();
            for(unsigned long j=0;j<mNbPointUsed;j++) *p0++ += *p1++;
         }
         nbBackgd++;
         TAU_PROFILE_STOP(timer4);
      }
   }
   if(0==nbBackgd) mPowderPatternBackgroundCalc.resize(0);//:KLUDGE:
   
   TAU_PROFILE_START(timer5);
   // Calc variance
   {
      VFN_DEBUG_MESSAGE("PowderPattern::CalcPowderPattern():variance",2);
      mPowderPatternVariance=mPowderPatternObsSigma;
      mPowderPatternVariance *= mPowderPatternVariance;
      for(int i=0;i<mPowderPatternComponentRegistry.GetNb();i++)
      {
         if(mPowderPatternComponentRegistry.GetObj(i).HasPowderPatternCalcVariance())
         {
            if(0==mPowderPatternComponentRegistry.GetObj(i).GetPowderPatternCalcVariance()
                    .numElements()) break;
                    
            REAL *p0 = mPowderPatternVariance.data();
            const REAL *p1=mPowderPatternComponentRegistry.GetObj(i)
                              .GetPowderPatternCalcVariance().data();
            
            if(true==mPowderPatternComponentRegistry.GetObj(i).IsScalable())
            {
               const REAL s2 = mScaleFactor(i) * mScaleFactor(i);
               for(unsigned long j=0;j<mNbPointUsed;j++) *p0++ += s2 * *p1++;
            }
            else for(unsigned long j=0;j<mNbPointUsed;j++) *p0++ += *p1++;
         }
      }
      REAL *p0 = mPowderPatternWeight.data();
      const REAL *p1=mPowderPatternVariance.data();
      for(unsigned long j=0;j<mNbPointUsed;j++)
         if(*p1 <=0) {*p0++ =0;p1++;}
         else *p0++ = 1. / *p1++;
   }
   mClockPowderPatternCalc.Click();
   TAU_PROFILE_STOP(timer5);
   VFN_DEBUG_EXIT("PowderPattern::CalcPowderPattern():End",3);
}

void PowderPattern::CalcPowderPatternIntegrated() const
{
   this->CalcNbPointUsed();
   if(mClockPowderPatternIntegratedCalc>mClockMaster) return;
   
   this->PrepareIntegratedRfactor();
   TAU_PROFILE("PowderPattern::CalcPowderPatternIntegrated()","void ()",TAU_DEFAULT);
   VFN_DEBUG_ENTRY("PowderPattern::CalcPowderPatternIntegrated()",4);
   if(mPowderPatternComponentRegistry.GetNb()==0)
   {
      mPowderPatternIntegratedCalc.resize(0);
      mPowderPatternVarianceIntegrated.resize(0);
      VFN_DEBUG_EXIT("PowderPattern::CalcPowderPatternIntegrated():no components!",4);
      return;
   }
   TAU_PROFILE_TIMER(timer1,"PowderPattern::CalcPowderPatternIntegrated()1:Calc components",\
                     "", TAU_FIELD);
   TAU_PROFILE_TIMER(timer2,"PowderPattern::CalcPowderPatternIntegrated()2:Add comps-scaled"\
                     ,"", TAU_FIELD);
   TAU_PROFILE_TIMER(timer3,"PowderPattern::CalcPowderPatternIntegrated()2:Add backgd1"\
                     ,"", TAU_FIELD);
   TAU_PROFILE_TIMER(timer4,"PowderPattern::CalcPowderPatternIntegrated()2:Add backgd2"\
                     ,"", TAU_FIELD);
   TAU_PROFILE_TIMER(timer5,"PowderPattern::CalcPowderPatternIntegrated()3:Variance"
                     ,"", TAU_FIELD);
   TAU_PROFILE_START(timer1);
   vector< pair<const CrystVector_REAL*,const RefinableObjClock*> > comps;
   for(int i=0;i<mPowderPatternComponentRegistry.GetNb();i++)
   {
      comps.push_back(mPowderPatternComponentRegistry.GetObj(i).
                      GetPowderPatternIntegratedCalc());
   }
   TAU_PROFILE_STOP(timer1);
   bool b=false;
   if(mClockPowderPatternCalc<mClockScaleFactor)
   {
      b=true;
   }
   else
      for(vector< pair<const CrystVector_REAL*,const RefinableObjClock*> >::iterator
          pos=comps.begin();pos!=comps.end();++pos) 
         if(mClockPowderPatternCalc < *(pos->second) )
         {
            b=true;
            break;
         }
      
   if(false==b)
   {
      VFN_DEBUG_EXIT("PowderPattern::CalcPowderPatternIntegrated():no need to recalc",4);
      return;
   }
   VFN_DEBUG_MESSAGE("PowderPattern::CalcPowderPatternIntegrated():Recalc",3);
   mPowderPatternIntegratedCalc.resize(mNbIntegrationUsed);
   int nbBackgd=0;//count number of background phases
   for(int i=0;i<mPowderPatternComponentRegistry.GetNb();i++)
   {
      VFN_DEBUG_MESSAGE("PowderPattern::CalcPowderPatternIntegrated():Adding "\
                        << mPowderPatternComponentRegistry.GetObj(i).GetName(),3);
      if(true==mPowderPatternComponentRegistry.GetObj(i).IsScalable())
      {
         TAU_PROFILE_START(timer2);
         if(0==i)
         {
            const REAL * p1= comps[i].first->data();
            REAL * p0 = mPowderPatternIntegratedCalc.data();
            const REAL s = mScaleFactor(i);
            for(unsigned long j=0;j<mNbIntegrationUsed;j++) *p0++ = s * *p1++;
         }
         else
         {
            const REAL * p1= comps[i].first->data();
            REAL * p0 = mPowderPatternIntegratedCalc.data();
            const REAL s = mScaleFactor(i);
            for(unsigned long j=0;j<mNbIntegrationUsed;j++) *p0++ += s * *p1++;
         }
          TAU_PROFILE_STOP (timer2);
      }
      else
      {// This is a background phase
         TAU_PROFILE_START(timer3);
         if(0==i)
         {
            const REAL * p1= comps[i].first->data();
            REAL * p0 = mPowderPatternIntegratedCalc.data();
            for(unsigned long j=0;j<mNbIntegrationUsed;j++) *p0++ = *p1++;
         }
         else
         {
            const REAL * p1= comps[i].first->data();
            REAL * p0 = mPowderPatternIntegratedCalc.data();
            for(unsigned long j=0;j<mNbIntegrationUsed;j++) *p0++ += *p1++;

         }
         TAU_PROFILE_STOP(timer3);
         TAU_PROFILE_START(timer4);
         // The following is useless if there is only one background phase...
         if(0==nbBackgd)
         {
            mPowderPatternBackgroundIntegratedCalc.resize(mNbIntegrationUsed);
            const REAL * p1= comps[i].first->data();
            REAL * p0 = mPowderPatternBackgroundIntegratedCalc.data();
            for(unsigned long j=0;j<mNbIntegrationUsed;j++) *p0++ = *p1++;
         }
         else
         {
            const REAL * p1= comps[i].first->data();
            REAL * p0 = mPowderPatternBackgroundIntegratedCalc.data();
            for(unsigned long j=0;j<mNbIntegrationUsed;j++) *p0++ += *p1++;
         }
         nbBackgd++;
         TAU_PROFILE_STOP(timer4);
      }
   }
   TAU_PROFILE_START(timer5);
   if(0==nbBackgd) mPowderPatternBackgroundIntegratedCalc.resize(0);
   // Calc variance
   bool useCalcVariance=false;
   for(int i=0;i<mPowderPatternComponentRegistry.GetNb();i++)
      if(mPowderPatternComponentRegistry.GetObj(i).HasPowderPatternCalcVariance())
         if(mPowderPatternComponentRegistry.GetObj(i)
               .GetPowderPatternIntegratedCalcVariance().first->numElements() !=0)
                  useCalcVariance=true;
   if(useCalcVariance)
   {
      VFN_DEBUG_MESSAGE("PowderPattern::CalcPowderPatternIntegrated():variance",3);
      {
         mPowderPatternVarianceIntegrated.resize(mNbIntegrationUsed);
         mIntegratedWeight.resize(mNbIntegrationUsed);
         const REAL * p1= mIntegratedVarianceObs.data();
         REAL * p0 = mPowderPatternVarianceIntegrated.data();
         for(unsigned long j=0;j<mNbIntegrationUsed;j++) *p0++ = *p1++;
      }
      //cout <<"PowderPattern::CalcPowderPatternIntegrated():variance"
      //     <<"obsvarsum="<<log(mIntegratedVarianceObs.sum());
      for(int i=0;i<mPowderPatternComponentRegistry.GetNb();i++)
      {
         if(mPowderPatternComponentRegistry.GetObj(i).HasPowderPatternCalcVariance())
         {
            if(0==mPowderPatternComponentRegistry.GetObj(i)
                    .GetPowderPatternIntegratedCalcVariance().first->numElements()) break;
                    
            const REAL * p1= mPowderPatternComponentRegistry.GetObj(i)
                                .GetPowderPatternIntegratedCalcVariance().first->data();
            //cout <<",sumvar(i)="<<log(mPowderPatternComponentRegistry.GetObj(i)
            //                    .GetPowderPatternIntegratedCalcVariance().first->sum());
            REAL * p0 = mPowderPatternVarianceIntegrated.data();
            
            if(true==mPowderPatternComponentRegistry.GetObj(i).IsScalable())
            {
               const REAL s2 = mScaleFactor(i) * mScaleFactor(i);
               for(unsigned long j=0;j<mNbIntegrationUsed;j++) *p0++ += s2 * *p1++;
            }
            else for(unsigned long j=0;j<mNbIntegrationUsed;j++) *p0++ += *p1++;
            
         }
      }
      //cout <<endl;
      REAL *p0 = mIntegratedWeight.data();
      const REAL *p1=mPowderPatternVarianceIntegrated.data();
      for(unsigned long j=0;j<mNbIntegrationUsed;j++)
         if(*p1 <=0) {*p0++ =0;p1++;}
         else *p0++ = 1. / *p1++;
   }
   else mIntegratedWeight.resize(0);
   mClockPowderPatternIntegratedCalc.Click();
   TAU_PROFILE_STOP(timer5);
   /*
   // Compare-DEBUG ONLY
   {
      VFN_DEBUG_MESSAGE("PowderPattern::CalcPowderPatternIntegrated():Check",10);
      this->CalcPowderPattern();
      CrystVector_REAL integr(mNbIntegrationUsed);
      for(int k=0;k<mPowderPatternComponentRegistry.GetNb();k++)
      {
         VFN_DEBUG_MESSAGE("PowderPattern::CalcPowderPatternIntegrated():Check #"<<k,10);
         integr=0;
         const CrystVector_REAL *v
            =&(mPowderPatternComponentRegistry.GetObj(k).GetPowderPatternCalc());
         for(unsigned int i=0;i<mNbIntegrationUsed;i++)
         {
            integr(i)=0;
            for(int j=mIntegratedPatternMin(i);j<=mIntegratedPatternMax(i);j++)
            {
               integr(i) += (*v)(j);
            }
         }
         cout << "Integrated intensities, Component #"<<k<<endl
              << FormatVertVector<REAL> (integr,*(comps[k].first))<<endl;
      }
   }
   */
   VFN_DEBUG_EXIT("PowderPattern::CalcPowderPatternIntegrated():End",4);
}

void PowderPattern::Init()
{
   this->InitOptions();
   {
      RefinablePar tmp("2Theta0",&m2ThetaZero,-.05,.05,gpRefParTypeScattDataCorrPos,
                        REFPAR_DERIV_STEP_ABSOLUTE,true,true,true,false,RAD2DEG);
      tmp.AssignClock(mClockPowderPattern2ThetaCorr);
      tmp.SetDerivStep(1e-6);
      this->AddPar(tmp);
   }
   {
      RefinablePar tmp("2ThetaDispl",&m2ThetaDisplacement,-.05,.05,gpRefParTypeScattDataCorrPos,
                        REFPAR_DERIV_STEP_ABSOLUTE,true,true,true,false,RAD2DEG);
      tmp.AssignClock(mClockPowderPattern2ThetaCorr);
      tmp.SetDerivStep(1e-6);
      this->AddPar(tmp);
   }
   {
      RefinablePar tmp("2ThetaTransp",&m2ThetaTransparency,-.05,.05,gpRefParTypeScattDataCorrPos,
                        REFPAR_DERIV_STEP_ABSOLUTE,true,true,true,false,RAD2DEG);
      tmp.AssignClock(mClockPowderPattern2ThetaCorr);
      tmp.SetDerivStep(1e-6);
      this->AddPar(tmp);
   }
}
void PowderPattern::PrepareIntegratedRfactor()const
{
   VFN_DEBUG_ENTRY("PowderPattern::PrepareIntegratedRfactor()",3);
   bool needPrep=false;
   CrystVector_long *min,*max;
   for(int i=0;i<mPowderPatternComponentRegistry.GetNb();i++)
   {
      mPowderPatternComponentRegistry.GetObj(i).GetBraggLimits(min,max);
      if(mPowderPatternComponentRegistry.GetObj(i).GetClockBraggLimits()
            >mClockIntegratedFactorsPrep)
      {
         needPrep=true;
         break;
      }
   }
   
   // If using max sin(theta)/lambda
      this->CalcNbPointUsed();
      if(mClockIntegratedFactorsPrep<mClockNbPointUsed) needPrep=true;
   
   if(false==needPrep)
   {
      VFN_DEBUG_EXIT("PowderPattern::PrepareIntegratedRfactor():nothing to do",3);
      return;
   }
   TAU_PROFILE("PowderPattern::PrepareIntegratedRfactor()","void ()",TAU_DEFAULT);
   
   // First get all integration intervals and concatenate the arrays
   long numInterval=0;
   long numNewInterval=0;
   for(int i=0;i<mPowderPatternComponentRegistry.GetNb();i++)
   {
      min=0;
      max=0;
      mPowderPatternComponentRegistry.GetObj(i).GetBraggLimits(min,max);
      //cout << "Component #"<<i<<"  "<<min <<" "<<max<<endl;
      if(0==min) continue;
      //cout <<" : num intervals:"<< min->numElements()<<endl
      //     <<FormatVertVector<long>(*min,*max)<<endl;
      numNewInterval=min->numElements();
      mIntegratedPatternMin.resizeAndPreserve(numInterval+numNewInterval);
      for(int j=0;j<numNewInterval;j++) mIntegratedPatternMin(numInterval+j)=(*min)(j);
      mIntegratedPatternMax.resizeAndPreserve(numInterval+numNewInterval);
      for(int j=0;j<numNewInterval;j++) mIntegratedPatternMax(numInterval+j)=(*max)(j);
      numInterval+=numNewInterval;
   }
   VFN_DEBUG_MESSAGE("PowderPattern::PrepareIntegratedRfactor():2",3);
   //cout <<FormatVertVector<long>(mIntegratedPatternMin,mIntegratedPatternMax)<<endl;
   //sort the arrays USELESS ?
   {
      CrystVector_long index,tmp;
      if(mIntegratedPatternMin.numElements()>0) index=SortSubs(mIntegratedPatternMin);
      tmp=mIntegratedPatternMin;
      for(int i=0;i<numInterval;i++) mIntegratedPatternMin(i)=tmp(index(i));
      tmp=mIntegratedPatternMax;
      for(int i=0;i<numInterval;i++) mIntegratedPatternMax(i)=tmp(index(i));
   }
   //cout<<FormatVertVector<long>(mIntegratedPatternMin,mIntegratedPatternMax)<<endl;
   VFN_DEBUG_MESSAGE("PowderPattern::PrepareIntegratedRfactor():3",3);
   // Check all intervals are within pattern limits, correct them if necessary,
   // remove them if necessary (keep=false)
      CrystVector_bool keep(numInterval);
      keep=true;
      for(int i=0;i<numInterval;i++) 
      {
         if(mIntegratedPatternMin(i)<0) mIntegratedPatternMin(i)=0;
         if(mIntegratedPatternMin(i)>=(long)mNbPointUsed) keep(i)=false;
         if(mIntegratedPatternMax(i)<0) keep(i)=false;
         if(mIntegratedPatternMax(i)>=(long)mNbPointUsed) mIntegratedPatternMax(i)=mNbPointUsed-1;
      }
   VFN_DEBUG_MESSAGE("PowderPattern::PrepareIntegratedRfactor():4",3);
   // Make sure all intervals do not overlap, and correct if necessary, by
   // setting an intermediate point at the middle of the two intervals extremities
      for(int i=0;i<(numInterval-1);i++) 
      {// Here we assume the intervals are distributed ideally
       // :TODO: some more thorough testing may be needed
       // (eg for several phases with different widths...)
         if(false==keep(i)) continue;
         if(mIntegratedPatternMax(i)>=mIntegratedPatternMin(i+1))
         {
            mIntegratedPatternMax(i)=(mIntegratedPatternMax(i)+mIntegratedPatternMin(i+1))/2;
            long j=1;
            while(mIntegratedPatternMin(i + j)<=mIntegratedPatternMax(i))
            {
               mIntegratedPatternMin(i+j)=mIntegratedPatternMax(i)+1;
               if(mIntegratedPatternMin(i+j)>mIntegratedPatternMax(i+j)) keep(i+j)=false;
               if( (i+ ++j)==numInterval) break;
            }
         }
         //just in case...Could it happen ?
         if(mIntegratedPatternMin(i)>mIntegratedPatternMax(i)) keep(i)=false;
      }
   // Take care of excluded regions (change integration areas accordingly)
   // regions are sorted by ascending theta
      const long nbExclude=mExcludedRegionMin2Theta.numElements();
      if(nbExclude>0)
      {
         VFN_DEBUG_MESSAGE("PowderPattern::PrepareIntegratedRfactor():5:Excluded regions("<<nbExclude<<")",3);
         long j=0;
         long minExcl,maxExcl;
         minExcl=this->Get2ThetaCorrPixel(mExcludedRegionMin2Theta(0));
         maxExcl=this->Get2ThetaCorrPixel(mExcludedRegionMax2Theta(0));
         for(int i=0;i<nbExclude;i++)
         {
            while(mIntegratedPatternMax(j)<minExcl)
            {
               j++;
               if(j>=numInterval) break;
            }
            if(j>=numInterval) break;
            while(mIntegratedPatternMin(j)<maxExcl)
            {
               if( (mIntegratedPatternMin(j)>minExcl) &&(mIntegratedPatternMax(j)<maxExcl))
                  keep(j)=false;
               if( (mIntegratedPatternMin(j)<minExcl) &&(mIntegratedPatternMax(j)<maxExcl))
                  mIntegratedPatternMax(j)=minExcl;
               if( (mIntegratedPatternMin(j)>minExcl) &&(mIntegratedPatternMax(j)>maxExcl))
                  mIntegratedPatternMin(j)=maxExcl;
               if(j==(numInterval-1)) break;
               j++;
            }
            minExcl=this->Get2ThetaCorrPixel(mExcludedRegionMin2Theta(i));
            maxExcl=this->Get2ThetaCorrPixel(mExcludedRegionMax2Theta(i));
            //go back if one integration segment is concerned by several exclusion zones...
            if(j!=0)
               while(mIntegratedPatternMax(j)>=minExcl)
               {
                  j--;
                  if(j==0) break;
               }
         }
      }
   // Keep only the selected intervals
   VFN_DEBUG_MESSAGE("PowderPattern::PrepareIntegratedRfactor():6",3);
   long j=0;
   for(int i=0;i<numInterval;i++)
   {
      if(keep(i))
      {
         mIntegratedPatternMin(j  )=mIntegratedPatternMin(i);
         mIntegratedPatternMax(j++)=mIntegratedPatternMax(i);
      }
   }
   numInterval=j;
   mIntegratedPatternMax.resizeAndPreserve(numInterval);
   mIntegratedPatternMin.resizeAndPreserve(numInterval);
  
   VFN_DEBUG_MESSAGE("PowderPattern::PrepareIntegratedRfactor():intervals"<<endl\
                  <<FormatVertVector<long>(mIntegratedPatternMin,mIntegratedPatternMax),2);
   // Integrate Obs and weight arrays
   mIntegratedObs.resize(numInterval);
   mIntegratedVarianceObs.resize(numInterval);
   mIntegratedVarianceObs=0;
   mIntegratedObs=0;
   mIntegratedWeightObs.resize(numInterval);
   for(int i=0;i<numInterval;i++)
   {
      for(int j=mIntegratedPatternMin(i);j<=mIntegratedPatternMax(i);j++)
      {
         mIntegratedObs   (i)+=mPowderPatternObs(j);
         mIntegratedVarianceObs(i)+=mPowderPatternObsSigma(j)*mPowderPatternObsSigma(j);
      }
      if(mIntegratedVarianceObs(i) <= 0) mIntegratedWeightObs(i)=0;
      else mIntegratedWeightObs(i)=1./mIntegratedVarianceObs(i);
   }
   

   //cout<<FormatVertVector<REAL>(mIntegratedPatternMin,
   //                               mIntegratedPatternMax,
   //                               mIntegratedObs,mIntegratedWeight,12,6)<<endl;
   mNbIntegrationUsed=mIntegratedPatternMin.numElements();
   mClockIntegratedFactorsPrep.Click();
   VFN_DEBUG_EXIT("PowderPattern::PrepareIntegratedRfactor()",3);
}
void PowderPattern::CalcNbPointUsed()const
{
   REAL sinth=mMaxSinThetaOvLambda*this->GetWavelength();
   unsigned long tmp;
   if(1>fabs(sinth)) tmp=this->Get2ThetaCorrPixel(2*asin(sinth)); else tmp=mNbPoint;
   if(tmp>mNbPoint) tmp= mNbPoint;
   if(tmp !=mNbPointUsed)
   {
      mNbPointUsed=tmp;
      mClockNbPointUsed.Click();
   }
}

void PowderPattern::InitOptions()
{
   VFN_DEBUG_MESSAGE("PowderPattern::InitOptions()",5)
   static string OptProfileIntegrationName;
   static string OptProfileIntegrationChoices[2];
   
   static bool needInitNames=true;
   if(true==needInitNames)
   {
      OptProfileIntegrationName="Use Integrated Profiles";
      OptProfileIntegrationChoices[0]="Yes (recommended)";
      OptProfileIntegrationChoices[1]="No";
      
      needInitNames=false;//Only once for the class
   }
   mOptProfileIntegration.Init(2,&OptProfileIntegrationName,OptProfileIntegrationChoices);
   this->AddOption(&mOptProfileIntegration);
}

#ifdef __WX__CRYST__
WXCrystObjBasic* PowderPattern::WXCreate(wxWindow* parent)
{
   //:TODO: Check mpWXCrystObj==0
   mpWXCrystObj=new WXPowderPattern(parent,this);
   return mpWXCrystObj;
}
#endif
//######################################################################
//    PROFILE FUNCTIONS (for powder diffraction)
//######################################################################

CrystVector_REAL PowderProfileGauss  (const CrystVector_REAL ttheta,const REAL fwhm,
                                      const REAL asymmetryPar)
{
   TAU_PROFILE("PowderProfileGauss()","Vector (Vector,REAL)",TAU_DEFAULT);
   //:TODO: faster... What if we return to using blitz++ ??
   const long nbPoints=ttheta.numElements();
   CrystVector_REAL result(nbPoints);
   result=ttheta;
   result *= result;
   REAL *p=result.data();
   
   if( fabs(asymmetryPar-1.) < 1e-5)
   {
      //reference: IUCr Monographs on Crystallo 5 - The Rietveld Method (ed RA Young)
      result *= -4.*log(2.)/fwhm/fwhm;
   }
   else
   {
      //reference : Toraya J. Appl. Cryst 23(1990),485-491
      
      //Search values <0
      long middlePt;
      const REAL *pt=ttheta.data();
      for( middlePt=0;middlePt<nbPoints;middlePt++) if( *pt++ > 0) break;
      
      const REAL c1= -(1.+asymmetryPar)/asymmetryPar*log(2.)/fwhm/fwhm;
      const REAL c2= -(1.+asymmetryPar)             *log(2.)/fwhm/fwhm;
      for(long i=0;i<middlePt;i++) *p++ *= c1;
      for(long i=middlePt;i<nbPoints;i++) *p++ *= c2;
   }
   p=result.data();
   for(long i=0;i<nbPoints;i++) { *p = exp(*p) ; p++ ;}
   
   result *= 2. / fwhm * sqrt(log(2.)/M_PI);
   return result;
}

CrystVector_REAL PowderProfileLorentz(const CrystVector_REAL ttheta,const REAL fwhm,
                                      const REAL asymmetryPar)
{
   TAU_PROFILE("PowderProfileLorentz()","Vector (Vector,REAL)",TAU_DEFAULT);
   //:TODO: faster... What if we return to using blitz++ ??
   //reference: IUCr Monographs on Crystallo 5 - The Rietveld Method (ed RA Young)
   const long nbPoints=ttheta.numElements();
   CrystVector_REAL result(nbPoints);
   result=ttheta;
   result *= result;
   REAL *p=result.data();
   if( fabs(asymmetryPar-1.) < 1e-5)
   {
      //reference: IUCr Monographs on Crystallo 5 - The Rietveld Method (ed RA Young)
      result *= 4./fwhm/fwhm;
   }
   else
   {
      //reference : Toraya J. Appl. Cryst 23(1990),485-491
      //Search values <0
      long middlePt;
      const REAL *pt=ttheta.data();
      for( middlePt=0;middlePt<nbPoints;middlePt++) if( *pt++ > 0) break;
      //cout << nbPoints << " " << middlePt <<endl;
      const REAL c1= (1+asymmetryPar)/asymmetryPar*(1+asymmetryPar)/asymmetryPar/fwhm/fwhm;
      const REAL c2= (1+asymmetryPar)*(1+asymmetryPar)/fwhm/fwhm;
      for(long i=0;i<middlePt;i++) *p++ *= c1 ;
      for(long i=middlePt;i<nbPoints;i++)  *p++ *= c2 ;
   }
   p=result.data();
   result += 1. ;
   for(long i=0;i<nbPoints;i++) { *p = 1/(*p) ; p++ ;}
   
   
   result *= 2./M_PI/fwhm;
   return result;
}


}//namespace ObjCryst

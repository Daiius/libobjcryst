/*
* LibCryst++ : a Crystallographic computing library in C++
*
*  (c) 2000 Vincent FAVRE-NICOLIN
*           Laboratoire de Cristallographie
*           24, quai Ernest-Ansermet, CH-1211 Geneva 4, Switzerland
*  Contact: Vincent.Favre-Nicolin@cryst.unige.ch
*           Radovan.Cerny@cryst.unige.ch
*
*/
/*   LSQNumObj.h
*  header file for Least-Squares refinement witn Numerical derivatives Objects
*
*/
#ifndef _LSQOBJNUM_H

#include "CrystVector/CrystVector.h"
#include "RefinableObj/RefinableObj.h"
#include <string>

using namespace std;
using namespace ObjCryst;
/** \brief (Quick & dirty) Least-Squares Refinement Object with Numerical derivatives
*
* This is still highly experimental !
*/
class LSQNumObj
{
	public:
		LSQNumObj(string objName="Unnamed LSQ object");
		~LSQNumObj();
      /// Fix one parameter
      void SetParIsFixed(const string& parName,const bool fix);
      /// Fix one family of parameters
      void SetParIsFixed(const RefParType *type,const bool fix);
      /// UnFix All parameters
      void UnFixAllPar();
      /// Set a parameter to be used
      void SetParIsUsed(const string& parName,const bool use);
      /// Set a family of parameters to be used
      void SetParIsUsed(const RefParType *type,const bool use);
      
		void Refine (int nbCycle=1,bool useLevenbergMarquardt=false);
		CrystVector_REAL Sigma()const;
		CrystMatrix_REAL CorrelMatrix()const;
		REAL Rfactor()const;
		REAL RwFactor()const;
		REAL ChiSquare()const;	//uses the weight if specified
      ///Add an object to refine
		void SetRefinedObj(RefinableObj &obj, const unsigned int LSQFuncIndex=0);
		void SetUseSaveFileOnEachCycle(bool yesOrNo=true);
		void SetSaveFile(string fileName="refine.save");
		void PrintRefResults()const;
		void SetDampingFactor(const REAL newDampFact);
		void PurgeSaveFile();
		void WriteReportToFile()const;
      
		void OptimizeDerivativeSteps();
	protected:
	private:
      /// Prepare mRefParList for the refinement
      void PrepareRefParList();
      // Refined object
         /// The recursive list of all refined sub-objects
         ObjRegistry<RefinableObj> mRecursiveRefinedObjList;
      /// The refinable par list used during refinement. Only a compilation
      /// of the parameters in RefinableObj and its sub-objects
		mutable RefinableObj mRefParList;
      /// Damping factor for the refinement (unused yet...)
		REAL mDampingFactor;
      ///Save result to file after each cycle ?
		bool mSaveReportOnEachCycle;	
      /// Name of the refined object
		string mName;
      /// File name where refinement info is saved
		string mSaveFileName;
		REAL mR,mRw,mChiSq;
      /// Correlation matrix between all refined parameters.
		CrystMatrix_REAL mCorrelMatrix;
      /// Observed values.
		CrystVector_REAL mObs;
      /// Weight corresponding to all observed values.
		CrystVector_REAL mWeight;
      /// Index of the set of saved values for all refinable parameters, before refinement
      /// and before the last cycle.
      int mIndexValuesSetInitial, mIndexValuesSetLast;
      /// If true, then stop at the end of the cycle. Used in multi-threading environment
      bool mStopAfterCycle;
		/// The opitimized object
		RefinableObj *mpRefinedObj;
		/// The index of the LSQ function in the refined object (if there are several...)
		unsigned int mLSQFuncIndex;
};

#endif //_LSQOBJNUM_H

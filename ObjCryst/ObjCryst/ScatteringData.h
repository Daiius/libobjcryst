#ifndef _OBJCRYST_SCATTERINGDATA_H_
#define _OBJCRYST_SCATTERINGDATA_H_

#include "CrystVector/CrystVector.h"

#include "ObjCryst/General.h"

#include "ObjCryst/SpaceGroup.h"
#include "ObjCryst/ScatteringPower.h"
#include "ObjCryst/Scatterer.h"
#include "ObjCryst/Crystal.h"

//#include <stdlib.h>
#include <string>
//#include <iomanip>
//#include <cmath>
//#include <typeinfo>
//#include <fstream>
//#include <ctime>

namespace ObjCryst
{
extern const RefParType *gpRefParTypeScattData;
extern const RefParType *gpRefParTypeScattDataScale;
extern const RefParType *gpRefParTypeScattDataProfile;
extern const RefParType *gpRefParTypeScattDataProfileType;
extern const RefParType *gpRefParTypeScattDataProfileWidth;
extern const RefParType *gpRefParTypeScattDataProfileAsym;
extern const RefParType *gpRefParTypeScattDataCorr;
extern const RefParType *gpRefParTypeScattDataCorrInt;
extern const RefParType *gpRefParTypeScattDataCorrIntAbsorp;
extern const RefParType *gpRefParTypeScattDataCorrIntPolar;
extern const RefParType *gpRefParTypeScattDataCorrIntExtinc;
extern const RefParType *gpRefParTypeScattDataCorrPos;
extern const RefParType *gpRefParTypeScattDataBackground;

extern const RefParType *gpRefParTypeRadiation;
extern const RefParType *gpRefParTypeRadiationWavelength;

//######################################################################
/** \brief Class to define the radiation (type, monochromaticity) of an experiment
*
* This can be developped for more complex experiments, hence the \e vector of
* wavelengths (so far it is not possible to use several wavelengths, though).
*/
//######################################################################
class Radiation: public RefinableObj
{
   public:
      Radiation();
      Radiation(const RadiationType rad,const double wavelength);
      Radiation(const string &XRayTubeElementName,const double alpha2Alpha2ratio=0.5);
      Radiation(const Radiation&);
      ~Radiation();
      virtual const string GetClassName() const;
      
      void operator=(const Radiation&);
      
      RadiationType GetRadiationType()const;
      void SetRadiationType(const RadiationType);

      WavelengthType GetWavelengthType()const;
      
      const CrystVector_double& GetWavelength()const;
      void SetWavelength(const double );
      void SetWavelength(const string &XRayTubeElementName,const double alpha2Alpha2ratio=0.5);
      
      double GetXRayTubeDeltaLambda()const;
      double GetXRayTubeAlpha2Alpha1Ratio()const;
      
      const RefinableObjClock& GetClockWavelength()const ;
      const RefinableObjClock& GetClockRadiation()const ;
      virtual void Output(ostream &os,int indent=0)const;
      virtual void Input(istream &is,const XMLCrystTag &tag);
      virtual void InputOld(istream &is,const IOCrystTag &tag);
      void Print()const;
   private:
      void InitOptions();
      /// Neutron ? X-Ray ? Electron (unimplemented)
      RefObjOpt mRadiationType;
      /// monochromatic ? Alpha1 & Alpha2 ? Multi-Wavelength ?
      RefObjOpt mWavelengthType;
      ///Wavelength of the Experiment, in Angstroems.
      CrystVector_double mWavelength;
      ///Name of the X-Ray tube used, if relevant. ie "Cu", "Fe",etc... 
      /// "CuA1" for Cu-alpha1, etc...
      string mXRayTubeName;
      ///Absolute difference between alpha1 and alpha2, in angstroems
      double mXRayTubeDeltaLambda;
      ///Ratio alpha2/alpha1 (should be 0.5)
      double mXRayTubeAlpha2Alpha1Ratio;
      //Clocks
         RefinableObjClock mClockWavelength;
   #ifdef __WX__CRYST__
   public:
      virtual WXCrystObjBasic* WXCreate(wxWindow*);
      friend class WXRadiation;
   #endif
};

//######################################################################
/** \brief Class to compute structure factors for a set of reflections and a Crystal.
*
* This class only computes structure factor, but no intensity. i.e. it does
* not include any correction such as absorption, Lorentz or Polarization.
*
* Does this really need to be a RefinableObj ?
* \todo Optimize computation for Bijvoet/Friedel mates. To this generate
* an internal list of 'true independent reflections', with two entries for each,
* for both mates, and make the 'real' reflections only a reference to these reflections. 
* 
*/
//######################################################################
class ScatteringData: virtual public RefinableObj
{
   public:
      ScatteringData();
      ScatteringData(const ScatteringData &old);
      ~ScatteringData();
      /// So-called virtual copy constructor
      virtual ScatteringData* CreateCopy()const=0;
      
      /** \brief input H,K,L
      *
      * \param h,k,l: double arrays (vectors with NbRefl elements -same size),
      *with the h, k and l coordinates of all reflections.
      */
      virtual void SetHKL( CrystVector_double const &h,
                           CrystVector_double const &k,
                           CrystVector_double const &l);
      /** \brief Generate a list of h,k,l to describe a full reciprocal space, 
      * up to a given maximum theta value
      *
      * \param maxTheta:maximum theta value
      * \param useMultiplicity: if set to true, equivalent reflections will be removed.
      * Bijvoet (Friedel) pairs
      * are NOT merged, for 'anomalous' reasons, unless you have chosen to ignore the
      * imaginary part of the scattering factor. If true, then multiplicity is stored
      * in the mMultiplicity data member.
      *
      * \warning The ScatteringData object must already have been assigned 
      * a crystal object using SetCrystal(), and the experimental wavelength 
      * must also have been set before calling this function.
      *
      * \todo smarter generation, using spacegroup information to remove extinct reflection
      * rather than brute-force computation.
      */
      virtual void GenHKLFullSpace(const double maxTheta,
                                   const bool useMultiplicity=false);
      
      /// Set : neutron or x-ray experiment ? Wavelength ?
      virtual void SetRadiationType(const RadiationType radiation);
      ///Neutron or x-ray experiment ? Wavelength ?
      RadiationType GetRadiationType()const;
      
      /**Set the crystal for this experiment
      *
      */
      virtual void SetCrystal(Crystal &crystal);
      /// Const access to the data's crystal
      const Crystal& GetCrystal()const ;
      /// Access to the data's crystal
      Crystal& GetCrystal() ;
      
      ///Return the number of reflections in this experiment.
      long GetNbRefl() const;
      ///Return the 1D array of H coordinates for all reflections
      const CrystVector_double& GetH() const;
      ///Return the 1D array of K coordinates
      const CrystVector_double& GetK() const;
      ///Return the 1D array of L coordinates
      const CrystVector_double& GetL() const;
      /// Return the 1D array of H coordinates for all reflections, multiplied by 2*pi
      /// \internal  Should be private
      const CrystVector_double& GetH2Pi() const;
      ///Return the 1D array of K coordinates for all reflections, multiplied by 2*pi
      /// \internal  Should be private
      const CrystVector_double& GetK2Pi() const;
      ///Return the 1D array of L coordinates for all reflections, multiplied by 2*pi
      /// \internal  Should be private
      const CrystVector_double& GetL2Pi() const;
      
      /// Return an array with \f$ \frac{sin(\theta)}{\lambda} = \frac{1}{2d_{hkl}}\f$ 
      ///for all reflections
      const CrystVector_double& GetSinThetaOverLambda()const;
   
      ///  Returns the Array of calculated |F(hkl)|^2 for all reflections. These
      ///are squared structure factor, which may have been corrected for Lorentz, Polarization,
      ///depending on parameters.
      const CrystVector_double& GetFhklCalcSq() const;
      ///Access to real part of F(hkl)calc
      const CrystVector_double& GetFhklCalcReal() const;
      ///Access to imaginary part of F(hkl)calc
      const CrystVector_double& GetFhklCalcImag() const;
      
      ///Set the wavelength of the experiment (in Angstroems). This is 
      ///used to calculate theta angles and get X-Ray anomalous factors
      void SetWavelength(const double lambda);
      
      /** \brief Set the wavelength of the experiment to that of an X-Ray tube.
      *
      *\param XRayTubeElementName : name of the anticathode element name. Known
      *ones are Cr, Fe, Cu, Mo, Ag. 
      *\param alpha2Alpha2ratio: Kalpha2/Kalpha1 ratio (0.5 by default)
      *
      *Alpha1 and alpha2 wavelength are taken
      *from R. Grosse-Kunstleve package, and the average wavelength is calculated
      *using the alpha2/alpha1 weight. All structure factors computation are made 
      *using the average wavelength, and for powder diffraction, profiles are output
      *at the alpha1 and alpha 2 ratio for the calculated pattern.
      *
      *NOTE : if the name of the wavelength is generic (eg"Cu"), 
      *then the program considers that 
      *there are both Alpha1 and Alpha2, and thus automatically changes the WavelengthType 
      *to WAVELENGTH_ALPHA12. If instead either alpha1 or alpha2 (eg "CuA1") is asked for,
      *the WavelengthType is set to WAVELENGTH_MONOCHROMATIC.
      */
      void SetWavelength(const string &XRayTubeElementName,const double alpha2Alpha1ratio=0.5);
      
      ///Set the energy of the experiment (in keV). This is 
      ///used to calculate theta angles and get X-Ray anomalous factors
      void SetEnergy(const double energy);
      ///wavelength of the experiment (in Angstroems)
      CrystVector_double GetWavelength()const;
      
      /// Use of faster, less precise approximations to compute structure factors
      ///and interatomic distances ? (DiffractionData::mUseFastLessPreciseFunc)
      void SetUseFastLessPreciseFunc(const bool useItOrNot);
      /// If true, then the imaginary part of the scattering factor is ignored during
      /// Structure factor computation. (default value=false)
      ///
      /// \todo this should become useless once we take fully advantage of coupled
      /// computation of Structure Factors for Fridel/Bijvoet mates using an internal
      /// list of 'fully unique' reflections. Then only one of the mates need to be computed..
      void SetIsIgnoringImagScattFact(const bool b);
      /// If true, then the imaginary part of the scattering factor is ignored during
      /// Structure factor computation.
      bool IsIgnoringImagScattFact() const;
      // Set an option so that only low-amgle reflections (theta < angle)
      // are used. See DiffractionData::mUseOnlyLowAngleData
      //virtual void SetUseOnlyLowAngleData(const bool useOnlyLowAngle,const double angle)=0;
      /** \brief Print H, K, L F^2 Re(F) Im(F) theta sint(theta/lambda) for all reflections
      *
      */
      virtual void PrintFhklCalc()const;

   protected:
      /// \internal This function should be called after H,K and L arrays have 
      ///been initialized or modified.
      virtual void PrepareHKLarrays() ;
      /// \internal sort reflections by theta values (also get rid of [0,0,0] if present)
      /// If maxTheta >0, then only reflections where theta<maxTheta are kept
      /// \return an array with the subscript of the kept reflections (for inherited classes)
      CrystVector_long SortReflectionByTheta(const double maxTheta=-1.);
      /// \internal Get rid of extinct reflections. Useful after GenHKLFullSpace().
      /// Do not use this if you have a list of observed reflections !
      ///
      /// Currently done using (brute-force) numerical evaluation. Should rather use
      /// SpaceGroup info... To do !
      ///
      /// \return an array with the subscript of the kept reflections (for inherited classes)
      CrystVector_long EliminateExtinctReflections();
      
      //The following functions are used during the calculation of structure factors,
         /// \internal Get the list of scattering components, and check what needs to
         /// be recomputed to get the new structure factors. No calculation is made in
         /// this function. Just getting prepared...
         /// \todo Clean up the code, which is a really unbelievable mess (but working!)
         virtual void PrepareCalcStructFactor()const;
         /// \internal Compute sin(theta)/lambda. 
         /// theta and tan(theta) values are also re-computed, provided a wavelength has
         /// been supplied.
         virtual void CalcSinThetaLambda()const;
         /// \internal Get scattering factors for all ScatteringPower & reflections
         void CalcScattFactor()const;
         /// \internal Compute thermic factors for all ScatteringPower & reflections
         void CalcTemperatureFactor()const;
         /// \internal get f' and f" for ScatteringPower of the crystal, at the exp. wavelength
         ///
         /// This \e could be specialized for multi-wavelength experiments...
         virtual void CalcResonantScattFactor()const;
         
      /**\brief Compute the overall structure factor (real \b and imaginary part).
      *This function is \e optimized \e for \e speed (geometrical structure factors are 
      *computed for all atoms and all reflections in two loops, avoiding re-calculation).
      *So use this function for repetitive calculations.
      *
      *This function recognizes the type of radiation (XRay or neutron) and
      *uses the corresponding scattering factor/length.
      *
      *  \return the result (real and imaginary part of the structure factor)
      * (mRealFhklCalc, mImagFhklCalc) are stored within ScatteringData.
      */
      void CalcStructFactor() const;

      /** \brief Compute the 'Geometrical Structure Factor' for each ScatteringPower
      * of the Crystal
      *
      */
      void CalcGeomStructFactor(const ScatteringComponentList &scattCompList,
                                const SpaceGroup &spg,
                                const CrystVector_long &structFactorIndex,
                                CrystVector_double* rsf2,
                                CrystVector_double* isf2,
                                bool useFastTabulatedTrigFunctions=false) const;
      
      /// Number of H,K,L reflections
      long mNbRefl;
      ///H,K,L coordinates
      CrystVector_double mH, mK, mL ;
      ///H,K,L integer coordinates
      mutable CrystVector_long mIntH, mIntK, mIntL ;
      ///H,K,L coordinates, multiplied by 2PI
      mutable CrystVector_double mH2Pi, mK2Pi, mL2Pi ;

      ///Multiplicity for each reflections (mostly for powder diffraction)
      CrystVector_int mMultiplicity ;
      
      /// real &imaginary parts of F(HKL)calc
      mutable CrystVector_double mFhklCalcReal, mFhklCalcImag ;
      ///F(HKL)^2 calc for each reflection
      mutable CrystVector_double mFhklCalcSq ;
      
      /// Radiation
      Radiation mRadiation;
      
      /** Pointer to the crystal corresponding to this experiment.
      *
      *  This gives an access to the UB matrix for the crystal,
      * as well as to the list of Scatterer.
      */
      Crystal *mpCrystal;
      
      ///Use faster, but less precise, approximations for functions? (integer
      ///approximations to compute sin and cos in structure factors, and also
      ///to compute interatomic distances)
      bool mUseFastLessPreciseFunc;
      
      //The Following members are only kept to avoid useless re-computation
      //during global refinements. They are used \b only by CalcStructFactor() 
      
         ///  \f$ \frac{sin(\theta)}{\lambda} = \frac{1}{2d_{hkl}}\f$ 
         ///for the crystal and the reflections in ReciprSpace
         mutable CrystVector_double mSinThetaLambda;

         /// theta for the crystal and the HKL in ReciprSpace (in radians)
         mutable CrystVector_double mTheta;

         /// tan(theta) for the crystal and the HKL in ReciprSpace (for Caglioti's law)
         /// \note this should be moved to DiffractionDataPowder
         mutable CrystVector_double mTanTheta;

         /// Anomalous X-Ray scattering term f' and f" are stored here for each ScatteringPower 
         /// We assume yet that data is monochromatic, but this could be specialized.
         mutable CrystVector_double mFprime,mFsecond;

         ///Thermic factors as mNbScatteringPower vectors with NbRefl elements
         mutable CrystVector_double* mpTemperatureFactor;

         ///Scattering factors as mNbScatteringPower vectors with NbRefl elements
         mutable CrystVector_double* mpScatteringFactor;
      
         ///Geometrical Structure factor for all reflection and ScatteringPower
         mutable CrystVector_double* mpRealGeomSF,*mpImagGeomSF;
      
      //Public Clocks
         /// Clock for the list of hkl
         RefinableObjClock mClockHKL;
         /// Clock for the structure factor
         mutable RefinableObjClock mClockStructFactor;
         /// Clock for the square modulus of the structure factor
         mutable RefinableObjClock mClockStructFactorSq;
      //Internal Clocks
         /// Clock the last time theta was computed
         mutable RefinableObjClock mClockTheta;
         /// Clock the last time scattering factors were computed
         mutable RefinableObjClock mClockScattFactor;
         /// Clock the last time resonant scattering factors were computed
         mutable RefinableObjClock mClockScattFactorResonant;
         /// Clock the last time the geometrical structure factors were computed
         mutable RefinableObjClock mClockGeomStructFact;
         /// Clock the last time temperature factors were computed
         mutable RefinableObjClock mClockThermicFact;
      
      // Info about the Scattering components and powers
         /// Pointer to the ScatteringComponentList of the crystal.
         mutable const ScatteringComponentList* mpScattCompList;
         /// Index of the storage for scattering information. This array as the same size as
         /// the total number of ScatteringPower, and for each we give the index of where
         /// their scattering, temperature and resonant factors are stored.
         /// THIS IS AWFULLY KLUDGE-ESQUE !!!
         mutable CrystVector_long mScatteringPowerIndex;
         /// This is the reverse index. KLUDGEEEEEEEE !!!
         mutable CrystVector_long mScatteringPowerIndex2;
         /// Total number os ScatteringPower used for this DiffractionData
         mutable long mNbScatteringPower;
      // :TODO: This must be replaced by Clocks...
         mutable ScatteringComponentList mLastScattCompList;
         mutable bool mAnomalousNeedRecalc;
         mutable bool mThermicNeedRecalc;
         mutable bool mScattFactNeedRecalc;
         mutable bool mGeomFhklCalcNeedRecalc;
         mutable bool mFhklCalcNeedRecalc;

      /** \brief Ignore imaginary part of scattering factor.
      *
      * This can be used either to speed up computation, or when f"
      * has a small effect on calculated intensities, mostly for powder
      * diffraction (GenHKLFullSpace will not generate Friedel pairs, reducing
      * the number of reflections by a factor up to 2 for some structures).
      *
      * Practically this makes f"=0 during computation. The real resonant contribution (f')
      * is not affected.
      *
      * This may be removed later on...
      */
      bool mIgnoreImagScattFact;
      
      // Use only low angle data UNIMPLEMENTED
         /* \brief Flag forcing the use of only low-angle reflections
         *
         * Mainly for use during global optimizations, or more generally when
         * the program is far from the real structure. In this case high-angle reflection
         * are not significant and only lead to slower calculations and also in fooling
         * the optimization (bad agreement at low angle and good with the more numerous
         * high-angle reflections). By default this is set to \b false.
         *
         * This is intended for use mainly for powder diffraction but \e can also implemented
         * for other DiffractionData objects. (if not implemented, it should return
         * an exception).
         *
         * Practically, this means that any reflections below 
         * DiffractionData::mLowAngleReflectionLimit is simply ignored, not calculated,
         * not taken into account in statistics. But they should remain present in hkl arrays.
         *
         * Caveheat: for powder diffraction, a reflection just above the angular limit
         * will not be calculated but may be take partially into account in statistics.
         *
         * \warning This should only be used at the beginning of an optimization.
         *
         * \How this is done: when this flag is raised, a copy of all necessary data
         * (observed, sigmas, hkl arrays) is made, and stored until the flag is unraised.
         *
         * This may eventually be moved to inherited classes.
         */
         //bool mUseOnlyLowAngleData;

         /* \brief Limit (theta angle, radian) for the above option.
         *
         */
         //double mUseOnlyLowAngleDataLimit;
};

}//namespace ObjCryst
#endif // _OBJCRYST_SCATTERINGDATA_H_

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
/*   Crystal.h header file for the Crystal object
*
*/
#ifndef _OBJCRYST_CRYSTAL_H_
#define _OBJCRYST_CRYSTAL_H_

#include "CrystVector/CrystVector.h"

#include "ObjCryst/General.h"
#include "RefinableObj/RefinableObj.h"
#include "ObjCryst/SpaceGroup.h"
#include "ObjCryst/ScatteringPower.h"
#include "ObjCryst/Scatterer.h"

//#include <stdlib.h>
#include <string>
//#include <iomanip>
//#include <cmath>
//#include <typeinfo>
//#include <fstream>
//#include <ctime>

namespace ObjCryst
{
class Scatterer; //forward declaration of another header's class :KLUDGE:
extern const RefParType *gpRefParTypeCrystal;
extern const RefParType *gpRefParTypeUnitCell;
extern const RefParType *gpRefParTypeUnitCellLength;
extern const RefParType *gpRefParTypeUnitCellAngle;

//######################################################################
/** \brief Crystal class: Unit cell, spacegroup, scatterers
*
* A  Crystal object has several main characteristics : (1) a unit cell,
* (2) a Spacegroup and (3) a list of Scatterer. Also stored in the Crystal
* is a list of the ScttaringPower used by all the scatterers of this crystal.
*
* The crystal is capable of giving a list of all scattering components
* (ie the list of all unique scattering 'points' (ScatteringComponent, ie atoms) 
* in the unit cell, each associated to a ScatteringPower).
*
* When those scattering components are on a special position or overlapping with
* another component of the same type, it is possible to correct
* dynamically the occupancy of this/these components to effectively have
* only one component instead of several due to the overlapping. This
* method is interesting for global optimization where atoms must not be "locked"
* on a special position. If this "Dynamical Occupancy Correction"
* is used then no occupancy should be corrected for special positions, since
* this will be done dynamically.
*
* A crystal structure can be viewed in 3D using OpenGL.
*
* \todo exporting (and importing) crystal structures to/from other files
* format than ObjCryst's XML (eg CIF, and format used by refinement software)
*
* Currently only 3D crystal structures can be handled, with no magnetic
* structure (that may be done later) and no incommensurate structure.
*/
//######################################################################
class Crystal:public RefinableObj
{
   public:
      /// Default Constructor
      Crystal();
      /** \brief Crystal Constructor (orthorombic)
      *  \param a,b,c : unit cell dimension, in angstroems
      *  \param SpaceGroupId: space group symbol or number
      */
      Crystal(const REAL a, const REAL b, const REAL c,
              const string &SpaceGroupId);
      /** \brief Crystal Constructor (triclinic)
      *  \param a,b,c : unit cell dimension, in angstroems
      *  \param alpha,beta,gamma : unit cell angles, in radians.
      *  \param SpaceGroupId: space group symbol or number
      */
      Crystal(const REAL a, const REAL b, const REAL c, const REAL alpha,
              const REAL beta, const REAL gamma,const string &SpaceGroupId);
              
      /// Crystal copy constructor
      Crystal(const Crystal &oldCryst);
      /// Crystal destructor
      ~Crystal();
      virtual const string& GetClassName() const;
      
      /** \brief Add a scatterer to the crystal.
      *
      * \warning the scatterer \e must be allocated in the heap, since the scatterer
      * will \e not be copied but used directly. A Scatterer can only belong to one Crystal. It
      * will be detroyed when removed or when the Crystal is destroyed.
      * \param scatt : the address of the scatterer to be included in the crystal
      * scatterer names \b must be unique in a given crystal.
      * \note that the ScatteringPower used in the Scatterer should be one
      * of the Crystal (see Crystal::AddScatteringPower())
      *
      */
      void AddScatterer(Scatterer *scatt);
      /// Remove a Scatterer. This also deletes the scatterer.
      void RemoveScatterer(Scatterer *scatt);
      
      /// Number of scatterers in the crystal
      long GetNbScatterer()const;
      /** \brief Provides an access to the scatterers
      *
      * \param scattName the name of the scatterer to access
      */
      Scatterer & GetScatt(const string &scattName);
      /** \brief Provides a const access to the scatterers
      *
      * \param scattName the name of the scatterer to access
      */
      const Scatterer & GetScatt(const string &scattName) const;
      /** \brief Provides an access to the scatterers
      *
      * \param scattIndex the number of the scatterer to access
      */
      Scatterer & GetScatt(const long scattIndex);
      /** \brief Provides a const access to the scatterers
      *
      * \param scattIndex the number of the scatterer to access
      */
      const Scatterer & GetScatt(const long scattIndex) const;
      
      /// Get the registry of scatterers
      ObjRegistry<Scatterer>& GetScattererRegistry();
      /// Get the registry of ScatteringPower included in this Crystal.
      ObjRegistry<ScatteringPower>& GetScatteringPowerRegistry();
      /// Get the registry of ScatteringPower included in this Crystal.
      const ObjRegistry<ScatteringPower>& GetScatteringPowerRegistry()const;

      /// Add a ScatteringPower for this Crystal. It must be allocated in the heap,
      /// and not used by any other Crystal.
      void AddScatteringPower(ScatteringPower *scattPow);
      /// Remove a ScatteringPower for this Crystal. (the Scattering power is deleted).
      /// This function should check that it is not used any more before removing it.
      void RemoveScatteringPower(ScatteringPower *scattPow);
      /// Find a ScatteringPower from its name. Names must be unique in a given Crystal.
      ScatteringPower& GetScatteringPower(const string &name);
      
      /** \brief Get the list of all scattering components
      * \param useDynPopCorr: if true, then the dynamical polpulation correction
      * will be computed and stored in the list for each component. Else the
      * dynamical occupancy correction will be set to 1 for all components.
      */
      virtual const ScatteringComponentList& GetScatteringComponentList()const;
      /// Lattice parameters (a,b,c,alpha,beta,gamma) as a 6-element vector in Angstroems 
      /// and radians.
      CrystVector_REAL GetLatticePar() const;
      /// Return one of the 6 Lattice parameters, 0<= whichPar <6 (a,b,c,alpha,beta,gamma),
      /// returned in Angstroems and radians. 
      REAL GetLatticePar(const int whichPar)const;
      /** \brief Get the 'B' matrix (Crystal::mBMatrix)for the crystal (orthogonalization 
      * matrix for the given lattice, in the reciprocal space)
      *
      * The convention is taken following Giacovazzo, "Fundamentals of Crystallography", p.69
      * "e1 is chosen along a*, e2 in the (a*,b*) plane, then e3 is along c".
      */
      const CrystMatrix_REAL& GetBMatrix() const;
      /** \brief Get orthonormal cartesian coordinates for a set of (x,y,z)
      * fractional coordinates.
      *
      * Results are given in Amgstroems.
      * The convention is taken following :
      * e1 is chosen along a, e2 in the (a,b) plane, then e3 is along c*
      */
      CrystVector_REAL GetOrthonormalCoords(const REAL x,const REAL y,const REAL z) const;
      /** \brief Get orthonormal cartesian coordinates for a set of (x,y,z)
      * fractional coordinates.
      *
      * X,y and z input are changed to Amgstroems values
      * The convention is taken following :
      * e1 is chosen along a, e2 in the (a,b) plane, then e3 is along c*
      */
      void FractionalToOrthonormalCoords(REAL &x,REAL &y,REAL &z) const;
      /** \brief Get fractional cartesian coordinates for a set of (x,y,z)
      * orthonormal coordinates.
      *
      * Result is stored into x,y and z
      * The convention is taken following :
      * e1 is chosen along a, e2 in the (a,b) plane, then e3 is along c*
      */
      void OrthonormalToFractionalCoords(REAL &x,REAL &y,REAL &z) const;
      /** \brief Get Miller H,K, L indices from orthonormal coordinates 
      * in reciprocal space.
      *
      * Result is stored into x,y and z
      */
      void MillerToOrthonormalCoords(REAL &x,REAL &y,REAL &z) const;
      /** \brief Get orthonormal coordinates given a set of H,K, L indices
      * in reciprocal space.
      *
      * Result is stored into x,y and z
      */
      void OrthonormalToMillerCoords(REAL &x,REAL &y,REAL &z) const;
      /// Prints some info about the crystal
      /// \todo one function to print on one line and a PrintLong() function
      /// \param os the stream to which the information is outputed (default=cout)
      void Print(ostream &os=cout) const;
            
      /// Access to the crystal SpaceGroup object
      const SpaceGroup & GetSpaceGroup()const;
      /// Access to the crystal SpaceGroup object.
      SpaceGroup & GetSpaceGroup();
      
      /** \brief Minimum interatomic distance between all scattering components (atoms) in
      * the crystal.
      *
      * This will return a symmetrical matrix with NbComp rows and cols, where 
      * NbComp is the number of independent scattering components in the unit cell. 
      * All distances are given in Angstroems.
      *
      * Note that the distance of a given atom with 'itself' is not generally equal
      * to 0 (except full special position), but equal to the min distance with its 
      * symmetrics.
      * 
      * \param minDistance : atoms who are less distant than (minDistance,in Angstroems) 
      * are considered equivalent. So the smallest distance between any atoms will
      * be at least minDistance.
      */
      CrystMatrix_REAL GetMinDistanceTable(const REAL minDistance=0.1) const;
      /** \brief Print the minimum distance table between all scattering centers
      * (atoms) in the crystal.
      * \param os the stream to which the information is outputed (default=cout)
      */
      void PrintMinDistanceTable(const REAL minDistance=0.1,ostream &os=cout) const;

      /** \brief XMLOutput POV-Ray Description for this Crystal
      *
      * \param onlyIndependentAtoms if false, all symmetrics are showed in the
      * drawing.
      *
      * \warning This currently needs some fixing (ZScatterer does not work ?)
      * Use rather the OpenGL 3D display which is more useful.
      *
      * \param os the stream to which the information is outputed (default=cout)
      */
      ostream& POVRayDescription(ostream &os,bool onlyIndependentAtoms=false)const;
      
      /** Create an OpenGL DisplayList of the crystal.
      * \param onlyIndependentAtoms if false (the default), then all symmetrics
      * are displayed within the given limits
      * \ param xMin,xMax,yMin,yMax,zMin,zMax: in fractionnal coordinates, the region
      * in which we want scaterrers to be displayed. The test is made on the center
      * of the scatterer (eg a ZScatterer (molecule) will not be 'cut' on the border).
      */
      virtual void GLInitDisplayList(const bool onlyIndependentAtoms=false,
                                     const REAL xMin=-.1,const REAL xMax=1.1,
                                     const REAL yMin=-.1,const REAL yMax=1.1,
                                     const REAL zMin=-.1,const REAL zMax=1.1)const;
      
      /** \internal \brief Compute the 'Dynamical population correction for all atoms.
      * Atoms which are considered "equivalent" (ie currently with the same Z number)
      * and which are overlapping see their Dynamical occupancy changed so that when they
      * fully overlap, they are equivalent to 1 atom.
      *
      *
      *\param overlapDist : distance below which atoms (ScatteringComponents, to be more precise)
      * are considered overlapping and
      * should be corrected. The correction changes the dynamical occupancy from
      * 1 to 1/nbAtomOverlapping, progressively as the distance falls from \e overlapDist
      * to \e mergeDist.
      *\param mergeDist : distance below which atoms are considered fully overlapping.
      * If 3 atoms are 'fully' overlapping, then all have a dynamical population 
      * correction equal to 1/3
      *
      * This is const since ScatteringComponent::mDynPopCorr is mutable.
      *
      * \warning. Do not call this function, which will turn private. This is
      * called by \e only Crystal::GetScatteringComponentList()
      */
      void CalcDynPopCorr(const REAL overlapDist=1., const REAL mergeDist=.0)const ;
      /// Reset Dynamical Population Correction factors (ie set it to 1)
      void ResetDynPopCorr()const ;
      /** Set the use of dynamical population correction (Crystal::mUseDynPopCorr).
      * Atoms which are considered "equivalent" (ie currently with the same Z number)
      * and which are overlapping see their Dynamical occupancy changed so that when they
      * fully overlap, they are equivalent to 1 atom.
      *
      * The Dynamical Occupancy correction will be performed in
      * Crystal::GetScatteringComponentList() automatically.
      *
      * This \e seriously affects the speed of the calculation, since computing
      * interatomic distances is lenghty.
      * \param use set to 1 to use, 0 not to use it.
      */
      void SetUseDynPopCorr(const int use);
      /** Get the Anti-bumping/pro-Merging cost function. Only works (ie returnes a non-null
      * value) if you have added antibump distances using Crystal::SetBumpMergeDistance().
      *
      */
      REAL GetBumpMergeCostFunction() const;
      /** Set the Anti-bumping distance between two scattering types
      * 
      */
      void SetBumpMergeDistance(const ScatteringPower &scatt1,
                                const ScatteringPower &scatt2, const REAL dist=1.5);
      /// Set the Anti-bumping distance between two scattering types.
      void SetBumpMergeDistance(const ScatteringPower &scatt1,
                                const ScatteringPower &scatt2, const REAL dist,
                                const bool allowMerge);
      /// When were lattice parameters last changed ?
      const RefinableObjClock& GetClockLatticePar()const;
      /// When was the list of scatterers last changed ?
      const RefinableObjClock& GetClockScattererList()const;
      //Cost functions
         unsigned int GetNbCostFunction()const;
         const string& GetCostFunctionName(const unsigned int)const;
         const string& GetCostFunctionDescription(const unsigned int)const;
         virtual REAL GetCostFunctionValue(const unsigned int);
         
      virtual void XMLOutput(ostream &os,int indent=0)const;
      virtual void XMLInput(istream &is,const XMLCrystTag &tag);
      //virtual void XMLInputOld(istream &is,const IOCrystTag &tag);
      
      virtual void GlobalOptRandomMove(const REAL mutationAmplitude);
      /** \brief output Crystal structure as a cif file (EXPERIMENTAL !)
      *
      * \warning This is very crude and EXPERIMENTAL so far: only isotropic scattering power
      * are supported, and there is not much information beside atom
      * positions... 
      */
      virtual void CIFOutput(ostream &os)const;
      
      virtual void GetGeneGroup(const RefinableObj &obj, 
                                CrystVector_uint & groupIndex,
                                unsigned int &firstGroup) const;
      virtual void BeginOptimization(const bool allowApproximations=false,
                                     const bool enableRestraints=false);
   protected:
   
   private:
      /** \brief Init all Crystal parameters
      *  \param a,b,c : unit cell dimension, in angstroems
      *  \param alpha,beta,gamma : unit cell angles
      *  \param SpcGroup: space group number (1..230)
      *  \param name: name for the crystal, : '(TaSe4)2I'
      */
      void Init(const REAL a, const REAL b, const REAL c, const REAL alpha,
                const REAL beta, const REAL gamma,const string &SpaceGroupId,
                const string& name);
      
      /// Find a scatterer (its index # in mpScatterrer[]) with a given name
      /// \warning There should be no duplicate names !!! :TODO: test in AddScatterer()
      int FindScatterer(const string &scattName)const;
      
      /// \internal.Init the (de)orthogonalization matrices. 
      /// They are re-computed only if parameters have changed since last call.
      void InitMatrices() const;
      /// \internal
      /// Update cell parameters for tetragonal, trigonal, hexagonal, cubic lattices.
      /// Also set angular parameters for those group which need it. This is needed during
      /// Refinement, since for example in a quadratic spg, only a is refined and
      /// we need to have b=a...
      void UpdateLatticePar();

      /** \brief Prepare the refinable parameters list
      *
      * This is called once when creating the crystal.
      * OBSOLETE ?
      */
      void InitRefParList();
      
      /** \internal \brief Compute the distance Table (mDistTable) for all scattering components
      * \param fast : if true, the distance calculations will be made using
      * integers, thus with a lower precision but faster. Less atoms will also
      * be involved (using the AsymmetricUnit) to make it even faster.
      * \param asymUnitMargin (in Angstroem). This is used only if fast=true.
      * In that case, the distance is calculated between (i) independent atoms in the
      * asymmetric unit cell and (ii) all atoms which are inside the asymmetric unit cell
      * or less than 'asymUnitMargin' distant from the asymmetric unit borders.
      * This parameter should be used when only the shortest distances need to be calculated
      * (typically for dynamical population correction). Using a too short  margin will
      * result in having some distances calculated wrongly (ie one atom1 in the unit cell
      * could have an atom2 neighbor just outside the sym unit: if margin=0, then the distance
      * is calculated between atom1 and the atom2 symmetric inside the asym unit).
      * \warning Crystal::GetScatteringComponentList() \b must be called beforehand,
      * since this will not be done here.
      *
      * \return see Crystal::mDistTableSq and Crystal::mDistTableIndex
      * \todo sanitize the result distance table in a more usable structure than the currently
      * used Crystal::mDistTableSq and Crystal::mDistTableIndex.
      * \warning \e not using the fast option has not been very much tested...
      * \todo optimize again. Test if recomputation is needed using Clocks.
      * Use a global option instead of asymUnitMargin.
      */
      void CalcDistTable(const bool fast,const REAL asymUnitMargin=4)const;
            
      /// a,b and c in Angstroems, angles (stored) in radians
      /// For cubic, rhomboedric crystals, only the 'a' parameter is relevant.
      /// For quadratic and hexagonal crystals, only a and c parameters are relevant.
      /// The MUTABLE is temporary ! It should not be !
      CrystVector_REAL mCellDim;
      /// The space group of the crystal
      SpaceGroup mSpaceGroup ;
      /// The registry of scatterers for this crystal
      ObjRegistry<Scatterer> mScattererRegistry ;
      
      /** \brief B Matrix (Orthogonalization matrix for reciprocal space)
      * \f[ B= \left[ \begin {array}{ccc} a^* & b^*\cos(\gamma^*) & c^*\cos(\beta^*) \\
      *                            0 & b^*\sin(\gamma^*) & -c^*\sin(\beta^*)\cos(\alpha) \\
      *                            0 & 0 & \frac{1}{c}\end{array} \right]\f]
      *\f[ \left[ \begin{array}{c} k_x \\ k_y \\ k_z \end{array} \right]_{orthonormal}  
      *  = B \times \left[ \begin{array}{c} h \\ k \\ l \end{array}\right]_{integer} \f]
      * \note this matrix is and must remain upper triangular. this is assumed for
      * some optimizations.
      */
      mutable CrystMatrix_REAL mBMatrix;
      /// inverse of B Matrix (i.e. inverse of orthogonalization matrix for direct space)
      mutable CrystMatrix_REAL mBMatrixInvert;
      /** \brief Eucl Matrix (Orthogonalization matrix for direct space)
      * \f[ M_{orth}= \left[ \begin {array}{ccc} a & b\cos(\gamma) & c\cos(\beta) \\
      *                            0 & b\sin(\gamma) & -c\sin(\beta)\cos(\alpha^*) \\
      *                            0 & 0 & \frac{1}{c^*}\end{array} \right]\f]
      * \f[ \left[ \begin{array}{c} x \\ y \\ z \end{array} \right]_{orthonormal}  
      *  = M_{orth} \times \left[ \begin{array}{c} x \\ y \\ z \end{array}
      *                    \right]_{fractional coords}\f]
      * \note this matrix is and must remain upper triangular. this is assumed for
      * some optimizations.
      */
      mutable CrystMatrix_REAL mOrthMatrix;
      /// inverse of Eucl Matrix (i.e. inverse of de-orthogonalization matrix for direct space)
      mutable CrystMatrix_REAL mOrthMatrixInvert;
            
      /** \brief Distance table (squared) between all scattering components in the crystal
      *
      * Matrix, in Angstroems^2 (the square root is not computed
      *for optimization purposes), with as many columns as there are components
      * in Crystal::mScattCompList, and as many rows as necessary (to include
      * symmetrics in and nearby the Asymmetric Unit). See Crystal::CalcDistTable()
      *
      * The order of columns follows the order in Crystal::mScattCompList. The order
      * of rows is given in Crystal::mDistTableIndex
      */
      mutable CrystMatrix_REAL mDistTableSq;
      /** \brief Index of scattering components for the Distance table
      *
      * These are the index of the scattering components corresponding to each row in
      * Crystal::mDistTableSq.
      */
      mutable CrystVector_long  mDistTableIndex;
      /// The list of all scattering components in the crystal
      mutable ScatteringComponentList mScattCompList;
         
      /// Clock for lattice paramaters.
      RefinableObjClock mLatticeClock;
      /// Use Dynamical population correction (ScatteringComponent::mDynPopCorr) during Structure
      /// factor calculation ?
      RefObjOpt mUseDynPopCorr;
      /// Matrix of "bumping" (squared) distances
      CrystMatrix_REAL mBumpDistanceMatrix;
      /// Allow merging of atoms in the bump/merge function(should be true for identical atoms)?
      CrystMatrix_bool mAllowMerge;
      
      /// The registry of ScatteringPower for this Crystal.
      ObjRegistry<ScatteringPower> mScatteringPowerRegistry;
      
      //Clocks
      /// Last time the list of Scatterers was changed
      RefinableObjClock mClockScattererList;
      /// Last time lattice parameters were changed
      RefinableObjClock mClockLatticePar;
      /// \internal Last time metric matrices were computed
      mutable RefinableObjClock mClockMetricMatrix;
      /// \internal Last time the ScatteringComponentList was generated
      mutable RefinableObjClock mClockScattCompList;
      /// \internal Last time the Neighbor Table was generated
      mutable RefinableObjClock mClockNeighborTable;
      /// \internal Last time the lattice parameters whre updated
      mutable RefinableObjClock mClockLatticeParUpdate;
      /// \internal Last time the dynamical population correction was computed
      mutable RefinableObjClock mClockDynPopCorr;
      
      /// Display the enantiomeric (mirror along x) structure in 3D? This can
      /// be helpful for non-centrosymmetric structure which have been solved using
      /// powder diffraction (which only gives the relative configuration).
      RefObjOpt mDisplayEnantiomer;
      
   #ifdef __WX__CRYST__
   public:
      virtual WXCrystObjBasic* WXCreate(wxWindow*);
      friend class WXCrystal;
   #endif
};

/// Global registry for all Crystal objects
extern ObjRegistry<Crystal> gCrystalRegistry;


}// namespace


#endif //_OBJCRYST_CRYSTAL_H_

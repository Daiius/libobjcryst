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

#ifndef _VFN_WX_POWDERPATTERN_H_
#define _VFN_WX_POWDERPATTERN_H_

#include "wxCryst/wxRefinableObj.h"
#include "ObjCryst/PowderPattern.h"
namespace ObjCryst
{
class WXPowderPatternGraph;

/// WX Class for PowderPattern objects
class WXPowderPattern: public WXRefinableObj
{
   public:
      WXPowderPattern(wxWindow *parent, PowderPattern*);
      virtual void CrystUpdate();
      void OnMenuAddCompBackgd(wxCommandEvent & WXUNUSED(event));
      void OnMenuAddCompCryst(wxCommandEvent & WXUNUSED(event));
      void OnMenuShowGraph(wxCommandEvent & WXUNUSED(event));
      void OnMenuSaveText(wxCommandEvent & WXUNUSED(event));
      void OnMenuSimulate(wxCommandEvent & WXUNUSED(event));
      void OnMenuImportFullProf(wxCommandEvent & WXUNUSED(event));
      void OnMenuImportPSI(wxCommandEvent & WXUNUSED(event));
      void OnMenuImportILL(wxCommandEvent & WXUNUSED(event));
      void OnMenuImportXdd(wxCommandEvent & WXUNUSED(event));
      void OnMenuImportCPI(wxCommandEvent & WXUNUSED(event));
      void OnMenuImport2ThetaObsSigma(wxCommandEvent & WXUNUSED(event));
      void OnMenuImport2ThetaObs(wxCommandEvent & WXUNUSED(event));
      void OnMenuFitScaleForR(wxCommandEvent & WXUNUSED(event));
      void OnMenuFitScaleForRw(wxCommandEvent & WXUNUSED(event));
      void OnMenuSavePattern(wxCommandEvent & WXUNUSED(event));
      void OnMenuSetWavelength(wxCommandEvent &event);
      void OnMenuAdd2ThetaExclude(wxCommandEvent & WXUNUSED(event));
      void NotifyDeleteGraph();
      const PowderPattern& GetPowderPattern()const;
      void UpdateUI();
   private:
      PowderPattern *mpPowderPattern;
      WXRegistry<PowderPatternComponent> *mpWXComponent;
      WXPowderPatternGraph *mpGraph;
   DECLARE_EVENT_TABLE()
};
/** Class to display a Powder Pattern (calc,obs) in a graphic window.
*
* So far only displays calc and obs patterns.
* \todo display the difference pattern. Allow to zoom. Display
* reflection positions for crystalline phases.
*/
class WXPowderPatternGraph: public wxWindow
{
   public:
      /// Constructor. The top frame should have a Status bar with two fields (at least)
      WXPowderPatternGraph(wxFrame *frame, WXPowderPattern* parent);
      ~WXPowderPatternGraph();
      /// Redraw the spectrum
      void OnPaint(wxPaintEvent& WXUNUSED(event));
      /// Display the Theta and intensity values at the mouse position, in the status bar
      void OnMouse(wxMouseEvent &event);
      /// Update the powder spectrum, at the user's request. This calls
      /// the WXPowderPattern::CrystUpdate().
      void OnUpdate(wxCommandEvent & WXUNUSED(event));
      /// Update the spectrum. This is called by the WXPowderPattern parent.
      void SetPattern(const CrystVector_REAL &obs,
                       const CrystVector_REAL &calc,
                       const REAL tthetaMin,const REAL tthetaStep);
      /// Redraw the pattern (special function to ensure complete redrawing under windows...)
      void OnRedrawNewPattern(wxUpdateUIEvent& WXUNUSED(event));
   private:
      /// Reset the limits of the axis to full range.
      void ResetAxisLimits();
      WXPowderPattern *mpPattern;
      CrystVector_REAL mObs,mCalc,m2theta;
      const long mMargin;
      const REAL mDiffPercentShift;
      REAL mMaxIntensity,mMinIntensity,mMin2Theta,mMax2Theta;
      wxFrame *mpParentFrame;
      bool mCalcPatternIsLocked;
      /// Pop-up menu
      wxMenu* mpPopUpMenu;
      /// Are we within a dragging event ?
      bool mIsDragging;
      /// Remember coordinates at the beginning of the dragging
      REAL mDragging2Theta0,mDraggingIntensity0;
      /// Index of the first and last points drawn of the pattern
      mutable long mFirst,mLast;
      DECLARE_EVENT_TABLE()
};

//:TODO:
#if 0 
// Class to display one Background point for WXPowderPatternBackgound
// It is just a RefPar, with a field inserted for the position of the Background point.
// Only the value of the intensity is refinable.
class WXFieldRefParBackground:public WXFieldRefPar
{
   public:
      WXFieldRefParBackground(wxWindow *parent,RefinablePar *refpar,
                              CrystVector_REAL *backgd2Theta, const int numPt);
   private:
      wxTextCtrl *mpField2Theta;
      /// The vector of 2theta values
      CrystVector_REAL* mpBackgd2Theta;
      const int mBackgdPointNum;
}
#endif

/** Class to display a Powder Pattern Background
*
* Still very limited ! Only allows to import a list of background
* points...
* \todo Display the list of background points with the refinable 
* intensity. Add th possibility to change the points.
*/
class WXPowderPatternBackground: public WXRefinableObj
{
   public:
      WXPowderPatternBackground(wxWindow *parent, PowderPatternBackground*);
      void OnMenuImportUserBackground(wxCommandEvent & WXUNUSED(event));
   private:
      PowderPatternBackground *mpPowderPatternBackground;
   DECLARE_EVENT_TABLE()
};

/** Class to display a Powder Pattern for a crystalline phase
*
* Allows to choose the Crystal, as well as the profile
* associated to this crystalline phase.
*/
class WXPowderPatternDiffraction: public WXRefinableObj
{
   public:
      WXPowderPatternDiffraction(wxWindow *parent, PowderPatternDiffraction*);
      void OnChangeCrystal(wxCommandEvent & WXUNUSED(event));
      void OnMenuSaveHKLFcalc(wxCommandEvent & WXUNUSED(event));
      virtual void UpdateUI();
   private:
      PowderPatternDiffraction *mpPowderPatternDiffraction;
      WXFieldChoice* mpFieldCrystal;
   DECLARE_EVENT_TABLE()
};

} //namespace

#endif //_VFN_WX_POWDERPATTERN_H_

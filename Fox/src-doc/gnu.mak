DIR_CRYST := ../ObjCryst

all: doc

#make sure we only re-generate the doc if necessary
../html/index.html:algorithm.map crystal.png example.dox Fox.png header.html powder.png tutorial-pbso4.dox algorithm.png development.dox faq.dox fox-top.map index.dox screenshot.dox AlMePO.png diffractionsinglecrystal.map footer.html fox-top.png LaMg2NiD7.png tricks.dox compile.dox diffractionsinglecrystal.png fox.css manual.dox TriPhenylPhosphite.png crystal.map Doxyfile gnu.mak powder.map tutorial-cimetidine.dox
	doxygen
	cp -f *.png ../html

doc: ../html/index.html

cvsignore:
	cp -f ${DIR_CRYST}/.cvsignore ./

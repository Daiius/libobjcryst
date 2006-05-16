default: all

all: G3 G4 G5

Aqua-G3:
	xcodebuild -project Fox.xcodeproj -target Fox -configuration G3 

Aqua-G4:
	xcodebuild -project Fox.xcodeproj -target Fox -configuration G4

Aqua-G5:
	xcodebuild -project Fox.xcodeproj -target Fox -configuration G5

Fox-nogui: NoGUI-G3

NoGUI-G3:
	xcodebuild -project Fox.xcodeproj -target Fox-NoGUI -configuration G3

NoGUI-G4:
	xcodebuild -project Fox.xcodeproj -target Fox-NoGUI -configuration G4

NoGUI-G5:
	xcodebuild -project Fox.xcodeproj -target Fox-NoGUI -configuration G5

G3: Aqua-G3 NoGUI-G3

G4: Aqua-G4 NoGUI-G4

G5: Aqua-G5 NoGUI-G5

dist-G3:G3
	rm -Rf Fox-G3-`date "+-%Y-%m-%d"` Fox`date "+-%Y-%m-%d"`.dmg
	mkdir Fox-G3-`date "+-%Y-%m-%d"`
	cp -R build/G3/Fox.app build/G3/Fox-NoGUI example Fox-G3-`date "+-%Y-%m-%d"`/
	hdiutil create -srcfolder Fox-G3-`date "+-%Y-%m-%d"` Fox-G3-`date "+-%Y-%m-%d"`.dmg
	rm -Rf Fox-G3-`date "+-%Y-%m-%d"`

dist-G4:G4
	rm -Rf Fox-G4-`date "+-%Y-%m-%d"` Fox-G4-`date "+-%Y-%m-%d"`.dmg
	mkdir Fox-G4-`date "+-%Y-%m-%d"`
	cp -R build/G4/Fox.app build/G4/Fox-NoGUI example Fox-G4-`date "+-%Y-%m-%d"`/
	hdiutil create -srcfolder Fox-G4-`date "+-%Y-%m-%d"` Fox-G4-`date "+-%Y-%m-%d"`.dmg
	rm -Rf Fox-G4-`date "+-%Y-%m-%d"`

dist-G5:G5
	rm -Rf Fox-G5-`date "+-%Y-%m-%d"` Fox-G5-`date "+-%Y-%m-%d"`.dmg
	mkdir Fox-G5-`date "+-%Y-%m-%d"`
	cp -R build/G5/Fox.app build/G5/Fox-NoGUI example Fox-G5-`date "+-%Y-%m-%d"`/
	hdiutil create -srcfolder Fox-G5-`date "+-%Y-%m-%d"` Fox-G5-`date "+-%Y-%m-%d"`.dmg
	rm -Rf Fox-G5-`date "+-%Y-%m-%d"`

all: G3 G4 G5

dist-all: dist-G3 dist-G4 dist-G5

clean:
	rm -Rf build/Fox.build

update:
	svn update
	cd ObjCryst ; svn update
   

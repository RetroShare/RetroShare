#!/bin/sh

APP="RetroShare"
RSVERSION="0.6.7a"
QTVERSION="Qt-5.14.1"

# Install the 7z to create dmg archives.
#brew list p7zip || brew install p7zip

# Package your app
echo "Packaging retroshare..."
#cd ${project_dir}/build/macOS/clang/x86_64/release/
cd retroshare-gui/src/

# Remove build directories that you don't want to deploy
rm -rf moc
rm -rf obj
rm -rf qrc

# This automatically creates retroshare.dmg

echo "Creating dmg archive..."
macdeployqt retroshare.app -dmg

DATE=`date +"%m-%d-%Y"`
MACVERSION=`sw_vers -productVersion`

mv $APP.dmg "$APP-$RSVERSION-$DATE-MacOS-$MACVERSION-$QTVERSION.dmg"

# You can use the appdmg command line app to create your dmg file if
# you want to use a custom background and icon arrangement. I'm still
# working on this for my apps, myself. If you want to do this, you'll
# remove the -dmg option above.
# appdmg json-path YourApp_${TRAVIS_TAG}.dmg

# Copy other project files
cp "../../libbitdht/src/bitdht/bdboot.txt" "retroshare.app/Contents/Resources/"
cp "../../plugins/FeedReader/lib/libFeedReader.dylib" "retroshare.app/Contents/Resources/"

# cp "${project_dir}/README.md" "README.md"
# cp "${project_dir}/LICENSE" "LICENSE"
# cp "${project_dir}/Qt License" "Qt License"

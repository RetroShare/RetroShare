#!/bin/sh

# Install the 7z to create dmg archives.
brew install p7zip

# Package your app
echo "Packaging retroshare..."
#cd ${project_dir}/build/macOS/clang/x86_64/release/
cd retroshare-gui/src/

# Remove build directories that you don't want to deploy
rm -rf moc
rm -rf obj
rm -rf qrc

# if test "${TAG_NAME}" = "" ; then 
# 	TAG_NAME = "no_tag" ;
# 	echo No specific tag used.
# fi

# This automatically creates retroshare.dmg

echo "Creating dmg archive..."
macdeployqt retroshare.app -dmg

#mv retroshare.dmg "retroshare_${TAG_NAME}.dmg"

# You can use the appdmg command line app to create your dmg file if
# you want to use a custom background and icon arrangement. I'm still
# working on this for my apps, myself. If you want to do this, you'll
# remove the -dmg option above.
# appdmg json-path YourApp_${TRAVIS_TAG}.dmg

# Copy other project files
# cp "${project_dir}/README.md" "README.md"
# cp "${project_dir}/LICENSE" "LICENSE"
# cp "${project_dir}/Qt License" "Qt License"

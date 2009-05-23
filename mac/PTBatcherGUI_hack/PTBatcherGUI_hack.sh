#PTBatcherGUI hack 2009-05-22 Harry van der Wolf

contentsdir="$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents"
resdir="$contentsdir/Resources"
PTBG_hackdir="PTBatcherGUI_hack"

# First copy neccessary files to PTBatcherGUI.app
cp $PTBG_hackdir/Contents/Info.plist $contentsdir
cp $PTBG_hackdir/Contents/Resources/appIcon.icns $contentsdir/Resources
cp $PTBG_hackdir/Contents/Resources/AppSettings.plist $contentsdir/Resources
cp $PTBG_hackdir/Contents/Resources/script $contentsdir/Resources

# rename current PTBatcherGUI and copy platypus version
mv $contentsdir/MacOS/PTBatcherGui $contentsdir/MacOS/PTBG
cp $PTBG_hackdir/Contents/MacOS/PTBatcherGui $contentsdir/MacOS

# Copy project files
cp -Rf  $PTBG_hackdir/Contents/Resources/English.lproj/* "$resdir/en.lproj"

if [ $lang = "en" ]
 then
       continue
 else
       ln -s "$resdir/en.lproj/InfoPlist.strings" "$localisedresdir/InfoPlist.strings"
       ln -s "$resdir/en.lproj/MainMenu.nib" "$localisedresdir/MainMenu.nib"
 fi



#!/bin/sh

# $Id: localised.sh 2004 2007-05-11 00:17:50Z ippei $

resdir="$TARGET_BUILD_DIR/Hugin.app/Contents/Resources"
huginsrcdir="../src/hugin1/hugin"
xrcsrcdir="$huginsrcdir/xrc"

mkdir -p "$resdir"

for helplang in "en_EN fr_FR"
do
 
  localisedresdir="$resdir/$(echo en_EN  | grep -o '^[^_]*').lproj"
 
  if [ -d "$xrcsrcdir/data/help_$helplang" ]
  then
   echo "moving help_$helplang to $localisedresdir/help"
   cp -R "$resdir/xrc/data/help_$helplang" "$localisedresdir/help"
   for file in `ls $localisedresdir/help | grep .html`
   do
    echo  rewriting \'src=\"../help_common\' to \'src=\"../../xrc/data/help_common\'
    sed s/src\=\"..\\/help_common/src\=\"..\\/..\\/xrc\\/data\\/help_common/ "$localisedresdir/help/$file" > $localisedresdir/help/$file-copy
    mv $localisedresdir/help/$file-copy $localisedresdir/help/$file
   done
  fi
 
done
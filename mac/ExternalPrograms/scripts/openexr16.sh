# ------------------
#     openexr
# ------------------
# $Id: openexr.sh 2004 2007-05-11 00:17:50Z ippei $
# Copyright (c) 2007, Ippei Ukai


# prepare

# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository" \
# ARCHS="ppc i386" \
# ppcTARGET="powerpc-apple-darwin7" \
# i386TARGET="i386-apple-darwin8" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.3.9.sdk" \
#  ppcONLYARG="-mcpu=G3 -mtune=G4" \
#  i386ONLYARG="-mfpmath=sse -msse2 -mtune=pentium-m -ftree-vectorize" \
#  ppc64ONLYARG="-mcpu=G5 -mtune=G5 -ftree-vectorize" \
#  OTHERARGs="";



EXRVER_M="6"
EXRVER_FULL="$EXRVER_M.0.0"

NATIVE_LIBHALF_DIR="$REPOSITORYDIR/lib"


# init

let NUMARCH="0"

for i in $ARCHS
do
  NUMARCH=$(($NUMARCH + 1))
done

mkdir -p "$REPOSITORYDIR/bin";
mkdir -p "$REPOSITORYDIR/lib";
mkdir -p "$REPOSITORYDIR/include";


g++ -I"$REPOSITORYDIR/include/OpenEXR" "./IlmImf/b44ExpLogTable.cpp" \
 -L"$NATIVE_LIBHALF_DIR" -lHalf\
 -o "./IlmImf/b44ExpLogTable-native"

if [ -f "./IlmImf/Makefile.in-original" ]
then
 echo "original already exists!";
else
 mv "./IlmImf/Makefile.in" "./IlmImf/Makefile.in-original"
fi
sed -e 's/\.\/b44ExpLogTable/\.\/b44ExpLogTable-native/' \
    "./IlmImf/Makefile.in-original" > "./IlmImf/Makefile.in"


# compile

for ARCH in $ARCHS
do

 mkdir -p "$REPOSITORYDIR/arch/$ARCH/bin";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/lib";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/include";

 ARCHARGs=""
 MACSDKDIR=""

 if [ $ARCH = "i386" -o $ARCH = "i686" ]
 then
  TARGET=$i386TARGET
  MACSDKDIR=$i386MACSDKDIR
  ARCHARGs="$i386ONLYARG"
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
 then
  TARGET=$ppcTARGET
  MACSDKDIR=$ppcMACSDKDIR
  ARCHARGs="$ppcONLYARG"
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ]
 then
  TARGET=$ppc64TARGET
  MACSDKDIR=$ppc64MACSDKDIR
  ARCHARGs="$ppc64ONLYARG"
 elif [ $ARCH = "x86_64" ]
 then
  TARGET=$x64TARGET
  MACSDKDIR=$x64MACSDKDIR
  ARCHARGs="$x64ONLYARG"
 fi

 env CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include" \
  LDFLAGS="-L$REPOSITORYDIR/lib -dead_strip -prebind" \
  NEXT_ROOT="$MACSDKDIR" \
  PKG_CONFIG_PATH="$REPOSITORYDIR/lib/pkgconfig" \
  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
  --enable-shared --enable-static;

 mv "libtool" "libtool-bk";
 sed -e "s/-dynamiclib/-dynamiclib -arch $ARCH -isysroot $(echo $MACSDKDIR | sed 's/\//\\\//g')/g" "libtool-bk" > "libtool";

 #hack for apple-gcc 4.2
 if [ $CC != "" ]
 then
  for dir in IlmImf exrenvmap exrheader exrmakepreview exrmaketiled exrstdattr
  do
   mv $dir/Makefile $dir/Makefile.bk
   sed 's/-Wno-long-double//g' $dir/Makefile.bk > $dir/Makefile
  done
 fi

 make clean;
 make $OTHERMAKEARGs all;
 make install;

done


# merge

for liba in lib/libIlmImf.a lib/libIlmImf.$EXRVER_FULL.dylib
do

 if [ $NUMARCH -eq 1 ]
 then
  mv "$REPOSITORYDIR/arch/$ARCHS/$liba" "$REPOSITORYDIR/$liba";
  if [[ $liba == *.a ]]
  then 
   ranlib "$REPOSITORYDIR/$liba";
  fi
  continue
 fi

 LIPOARGs=""
 
 for ARCH in $ARCHS
 do
  LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$liba"
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$liba";
 if [[ $liba == *.a ]]
 then 
  ranlib "$REPOSITORYDIR/$liba";
 fi

done

if [ -f "$REPOSITORYDIR/lib/libIlmImf.$EXRVER_FULL.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libIlmImf.$EXRVER_M.dylib" "$REPOSITORYDIR/lib/libIlmImf.$EXRVER_FULL.dylib";
 ln -sfn "libIlmImf.$EXRVER_FULL.dylib" "$REPOSITORYDIR/lib/libIlmImf.$EXRVER_M.dylib";
 ln -sfn "libIlmImf.$EXRVER_FULL.dylib" "$REPOSITORYDIR/lib/libIlmImf.dylib";
fi


#pkgconfig

for ARCH in $ARCHS
do
 mkdir -p $REPOSITORYDIR/lib/pkgconfig
 sed 's/^exec_prefix.*$/exec_prefix=\$\{prefix\}/' $REPOSITORYDIR/arch/$ARCH/lib/pkgconfig/OpenEXR.pc > $REPOSITORYDIR/lib/pkgconfig/OpenEXR.pc
 break;
done



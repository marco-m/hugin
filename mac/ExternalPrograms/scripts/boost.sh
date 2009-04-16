# ------------------
#      boost
# ------------------
# $Id: boost.sh 1902 2007-02-04 22:27:47Z ippei $
# Copyright (c) 2007-2008, Ippei Ukai

# prepare

# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository" \
#  ARCHS="ppc i386" \
#  ppcTARGET="powerpc-apple-darwin8" \
#  ppcOSVERSION="10.4" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  ppcOPTIMIZE="-mcpu=G3 -mtune=G4" \
#  i386TARGET="i386-apple-darwin8" \
#  i386OSVERSION="10.4" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  i386OPTIMIZE ="-march=prescott -mtune=pentium-m -ftree-vectorize" \
#  OTHERARGs="";


BOOST_VER="1_38"

# install headers

mkdir -p "$REPOSITORYDIR/include"
rm -rf "$REPOSITORYDIR/include/boost";
cp -R "./boost" "$REPOSITORYDIR/include/";


# compile bjab

cd "./tools/jam/src";
sh "build.sh";
cd "../../../";
BJAM=$(ls ./tools/jam/src/bin.mac*/bjam)


# init

let NUMARCH="0"

for i in $ARCHS
do
  NUMARCH=$(($NUMARCH + 1))
done

mkdir -p "$REPOSITORYDIR/lib";


# compile boost_thread

for ARCH in $ARCHS
do

 rm -rf "stage-$ARCH";
 mkdir -p "stage-$ARCH";

 if [ $ARCH = "i386" -o $ARCH = "i686" ]
 then
  MACSDKDIR=$i386MACSDKDIR
  OSVERSION=$i386OSVERSION
  OPTIMIZE=$i386OPTIMIZE
  boostARCHITECTURE="x86"
  boostADDRESSMODEL="32"
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
 then
  MACSDKDIR=$ppcMACSDKDIR
  OSVERSION=$ppcOSVERSION
  OPTIMIZE=$ppcOPTIMIZE
  boostARCHITECTURE="power"
  boostADDRESSMODEL="32"
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ]
 then
  MACSDKDIR=$ppc64MACSDKDIR
  OSVERSION=$ppc64OSVERSION
  OPTIMIZE=$ppc64OPTIMIZE
  boostARCHITECTURE="power"
  boostADDRESSMODEL="64"
 elif [ $ARCH = "x86_64" ]
 then
  MACSDKDIR=$x64MACSDKDIR
  OSVERSION=$x64OSVERSION
  OPTIMIZE=$x64OPTIMIZE
  boostARCHITECTURE="x86"
  boostADDRESSMODEL="64"
 fi

 SDKVRSION=$(echo $MACSDKDIR | sed 's/^[^1]*\([[:digit:]]*\.[[:digit:]]*\).*/\1/')

 if [ $CXX = "" ]
 then 
  boostTOOLSET="--toolset=darwin"
  CXX="g++"
 else
  echo "using darwin : : $CXX ;" > ./TEMP-userconf.jam
  boostTOOLSET="--user-config=./TEMP-userconf.jam"
 fi
 
 # hack that sends extra arguments to g++
 $BJAM -a --stagedir="stage-$ARCH" --prefix=$REPOSITORYDIR $boostTOOLSET -n stage \
  --with-thread \
  variant=release link=static \
  architecture="$boostARCHITECTURE" address-model="$boostADDRESSMODEL" \
  macosx-version="$SDKVRSION" macosx-version-min="$OSVERSION" \
  | grep "^    " | sed 's/"//g' | sed s/$CXX/$CXX\ "$OPTIMIZE"/ | sed 's/-O3/-O2/g' \
  | while read COMMAND
    do
     echo "running command: $COMMAND"
     $COMMAND
    done;
 
 # hack that sends extra arguments to g++
 $BJAM -a --stagedir="stage-$ARCH" --prefix=$REPOSITORYDIR $boostTOOLSET -n stage \
  --with-thread \
  variant=release \
  architecture="$boostARCHITECTURE" address-model="$boostADDRESSMODEL" \
  macosx-version="$SDKVRSION" macosx-version-min="$OSVERSION" \
  | grep "^    " | sed 's/"//g' | sed s/$CXX/$CXX\ "$OPTIMIZE"/ | sed 's/-O3/-O2/g' \
  | while read COMMAND
    do
     echo "running command: $COMMAND"
     $COMMAND
    done;

 mv ./stage-$ARCH/lib/libboost_thread-*.dylib ./stage-$ARCH/lib/libboost_thread-$BOOST_VER.dylib
 mv ./stage-$ARCH/lib/libboost_thread-*.a ./stage-$ARCH/lib/libboost_thread-$BOOST_VER.a
done


# merge libboost_thread

for liba in "lib/libboost_thread-$BOOST_VER.a" "lib/libboost_thread-$BOOST_VER.dylib"
do

 if [ $NUMARCH -eq 1 ]
 then
  mv "stage-$ARCH/$liba" "$REPOSITORYDIR/$liba";
  if [[ $liba == *.a ]]
  then 
   ranlib "$REPOSITORYDIR/$liba";
  fi
  continue
 fi

 LIPOARGs=""
 
 for ARCH in $ARCHS
 do
  LIPOARGs="$LIPOARGs stage-$ARCH/$liba"
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$liba";
 if [[ $liba == *.a ]]
 then 
  ranlib "$REPOSITORYDIR/$liba";
 fi

done


if [ -f "$REPOSITORYDIR/lib/libboost_thread-$BOOST_VER.a" ]
then
 ln -sfn libboost_thread-$BOOST_VER.a $REPOSITORYDIR/lib/libboost_thread.a;
fi
if [ -f "$REPOSITORYDIR/lib/libboost_thread-$BOOST_VER.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libboost_thread-$BOOST_VER.dylib" "$REPOSITORYDIR/lib/libboost_thread-$BOOST_VER.dylib";
 ln -sfn libboost_thread-$BOOST_VER.dylib $REPOSITORYDIR/lib/libboost_thread.dylib;
fi

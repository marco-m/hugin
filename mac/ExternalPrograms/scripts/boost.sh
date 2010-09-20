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

# -------------------------------
# 20091206.0 sg Script tested and used to build 2009.4.0-RC3
# 20100121.0 sg Script updated for 1_41
# 20100121.1 sg Script reverted to 1_40
# 20100624.0 hvdw More robust error checking on compilation
# 20100831.0 hvdw Upgraded to 1_44
# 20100920.0 hvdw Acc removed libboost_system again and add iostreams and regex
# -------------------------------

fail()
{
        echo "** Failed at $1 **"
        exit 1
}


BOOST_VER="1_44"

# install headers

mkdir -p "$REPOSITORYDIR/include"
rm -rf "$REPOSITORYDIR/include/boost";
cp -R "./boost" "$REPOSITORYDIR/include/";


# compile bjam

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


# compile boost_thread, filesystem, system, regex and iostreams

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
  export CC=$i386CC;
  export CXX=$i386CXX;
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
 then
  MACSDKDIR=$ppcMACSDKDIR
  OSVERSION=$ppcOSVERSION
  OPTIMIZE=$ppcOPTIMIZE
  boostARCHITECTURE="power"
  boostADDRESSMODEL="32"
  export CC=$ppcCC;
  export CXX=$ppcCXX;
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ]
 then
  MACSDKDIR=$ppc64MACSDKDIR
  OSVERSION=$ppc64OSVERSION
  OPTIMIZE=$ppc64OPTIMIZE
  boostARCHITECTURE="power"
  boostADDRESSMODEL="64"
  export CC=$ppc64CC;
  export CXX=$ppc64CXX;
 elif [ $ARCH = "x86_64" ]
 then
  MACSDKDIR=$x64MACSDKDIR
  OSVERSION=$x64OSVERSION
  OPTIMIZE=$x64OPTIMIZE
  boostARCHITECTURE="x86"
  boostADDRESSMODEL="64"
  export CC=$x64CC;
  export CXX=$x64CXX;
 fi

 SDKVRSION=$(echo $MACSDKDIR | sed 's/^[^1]*\([[:digit:]]*\.[[:digit:]]*\).*/\1/')

if [ "$CXX" = "" ] 
then
  boostTOOLSET="--toolset=darwin"
  CXX="g++"
 else
  echo "using darwin : : $CXX ;" > ./TEMP-userconf.jam
  boostTOOLSET="--user-config=./TEMP-userconf.jam"
 fi
 
 # hack that sends extra arguments to g++
# $BJAM -a --stagedir="stage-$ARCH" --prefix=$REPOSITORYDIR $boostTOOLSET -n stage \
#  --with-thread --with-filesystem \
#  variant=release link=static \
#  architecture="$boostARCHITECTURE" address-model="$boostADDRESSMODEL" \
#  macosx-version="$SDKVRSION" macosx-version-min="$OSVERSION" \
#  | grep "^    " | sed 's/"//g' | sed s/$CXX/$CXX\ "$OPTIMIZE"/ | sed 's/-O3/-O2/g' \
#  | while read COMMAND
#    do
#     echo "running command: $COMMAND"
#     $COMMAND
#    done;
 
 # hack that sends extra arguments to g++
 $BJAM -a --stagedir="stage-$ARCH" --prefix=$REPOSITORYDIR $boostTOOLSET -n stage \
  --with-thread --with-filesystem --with-system --with-regex --with-iostreams \
  variant=release \
  architecture="$boostARCHITECTURE" address-model="$boostADDRESSMODEL" \
  macosx-version="$SDKVRSION" macosx-version-min="$OSVERSION" \
  | grep "^    " | sed 's/"//g' | sed s/$CXX/$CXX\ "$OPTIMIZE"/  \
  | while read COMMAND
    do
     echo "running command: $COMMAND"
     $COMMAND
    done;

 mv ./stage-$ARCH/lib/libboost_thread.dylib ./stage-$ARCH/lib/libboost_thread-$BOOST_VER.dylib
 mv ./stage-$ARCH/lib/libboost_thread.a ./stage-$ARCH/lib/libboost_thread-$BOOST_VER.a
 mv ./stage-$ARCH/lib/libboost_filesystem.dylib ./stage-$ARCH/lib/libboost_filesystem-$BOOST_VER.dylib
 mv ./stage-$ARCH/lib/libboost_filesystem.a ./stage-$ARCH/lib/libboost_filesystem-$BOOST_VER.a
 mv ./stage-$ARCH/lib/libboost_system.dylib ./stage-$ARCH/lib/libboost_system-$BOOST_VER.dylib
 mv ./stage-$ARCH/lib/libboost_system.a ./stage-$ARCH/lib/libboost_system-$BOOST_VER.a
 mv ./stage-$ARCH/lib/libboost_regex.dylib ./stage-$ARCH/lib/libboost_regex-$BOOST_VER.dylib
 mv ./stage-$ARCH/lib/libboost_regex.a ./stage-$ARCH/lib/libboost_regex-$BOOST_VER.a
 mv ./stage-$ARCH/lib/libboost_iostreams.dylib ./stage-$ARCH/lib/libboost_iostreams-$BOOST_VER.dylib
 mv ./stage-$ARCH/lib/libboost_iostreams.a ./stage-$ARCH/lib/libboost_iostreams-$BOOST_VER.a
done

#read pipo

# merge libboost_thread libboost_filesystem libboost_system libboost_regex libboost_iostreams

for liba in "lib/libboost_thread-$BOOST_VER.a" "lib/libboost_filesystem-$BOOST_VER.a" "lib/libboost_system-$BOOST_VER.a" "lib/libboost_regex-$BOOST_VER.a" "lib/libboost_iostreams-$BOOST_VER.a" "lib/libboost_thread-$BOOST_VER.dylib"  "lib/libboost_filesystem-$BOOST_VER.dylib" "lib/libboost_system-$BOOST_VER.dylib" "lib/libboost_regex-$BOOST_VER.dylib"  "lib/libboost_iostreams-$BOOST_VER.dylib"
do

 if [ $NUMARCH -eq 1 ] ; then
   if [ -f stage-$ARCHS/$liba ] ; then
		 echo "Moving stage-$ARCHS/$liba to $liba"
  	 mv "stage-$ARCHS/$liba" "$REPOSITORYDIR/$liba";
	   #Power programming: if filename ends in "a" then ...
	   [ ${liba##*.} = a ] && ranlib "$REPOSITORYDIR/$liba";
  	 continue
	 else
		 echo "Program arch/$ARCHS/$liba not found. Aborting build";
		 exit 1;
	 fi
 fi

 LIPOARGs=""
 
 for ARCH in $ARCHS
 do
  if [ -f stage-$ARCH/$liba ] ; then
		echo "Adding stage-$ARCH/$liba to bundle"
  	LIPOARGs="$LIPOARGs stage-$ARCH/$liba"
	else
		echo "File stage-$ARCH/$liba was not found. Aborting build";
		exit 1;
	fi
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$liba";
 #Power programming: if filename ends in "a" then ...
 [ ${liba##*.} = a ] && ranlib "$REPOSITORYDIR/$liba";

done


if [ -f "$REPOSITORYDIR/lib/libboost_thread-$BOOST_VER.a" ] ; then
  ln -sfn libboost_thread-$BOOST_VER.a $REPOSITORYDIR/lib/libboost_thread.a;
fi
if [ -f "$REPOSITORYDIR/lib/libboost_thread-$BOOST_VER.dylib" ] ; then
 install_name_tool -id "$REPOSITORYDIR/lib/libboost_thread-$BOOST_VER.dylib" "$REPOSITORYDIR/lib/libboost_thread-$BOOST_VER.dylib";
 ln -sfn libboost_thread-$BOOST_VER.dylib $REPOSITORYDIR/lib/libboost_thread.dylib;
fi
if [ -f "$REPOSITORYDIR/lib/libboost_filesystem-$BOOST_VER.a" ] ; then
  ln -sfn libboost_filesystem-$BOOST_VER.a $REPOSITORYDIR/lib/libboost_fileystem.a;
fi
if [ -f "$REPOSITORYDIR/lib/libboost_filesystem-$BOOST_VER.dylib" ]; then
 install_name_tool -id "$REPOSITORYDIR/lib/libboost_filesystem-$BOOST_VER.dylib" "$REPOSITORYDIR/lib/libboost_filesystem-$BOOST_VER.dylib";
 ln -sfn libboost_filesystem-$BOOST_VER.dylib $REPOSITORYDIR/lib/libboost_filesystem.dylib;
fi
if [ -f "$REPOSITORYDIR/lib/libboost_system-$BOOST_VER.a" ] ; then
  ln -sfn libboost_system-$BOOST_VER.a $REPOSITORYDIR/lib/libboost_system.a;
fi
if [ -f "$REPOSITORYDIR/lib/libboost_system-$BOOST_VER.dylib" ]; then
 install_name_tool -id "$REPOSITORYDIR/lib/libboost_system-$BOOST_VER.dylib" "$REPOSITORYDIR/lib/libboost_system-$BOOST_VER.dylib";
 ln -sfn libboost_system-$BOOST_VER.dylib $REPOSITORYDIR/lib/libboost_system.dylib;
fi
if [ -f "$REPOSITORYDIR/lib/libboost_regex-$BOOST_VER.a" ] ; then
  ln -sfn libboost_regex-$BOOST_VER.a $REPOSITORYDIR/lib/libboost_regex.a;
fi
if [ -f "$REPOSITORYDIR/lib/libboost_regex-$BOOST_VER.dylib" ]; then
 install_name_tool -id "$REPOSITORYDIR/lib/libboost_regex-$BOOST_VER.dylib" "$REPOSITORYDIR/lib/libboost_regex-$BOOST_VER.dylib";
 ln -sfn libboost_regex-$BOOST_VER.dylib $REPOSITORYDIR/lib/libboost_regex.dylib;
fi
if [ -f "$REPOSITORYDIR/lib/libboost_iostreams-$BOOST_VER.a" ] ; then
  ln -sfn libboost_iostreams-$BOOST_VER.a $REPOSITORYDIR/lib/libboost_iostreams.a;
fi
if [ -f "$REPOSITORYDIR/lib/libboost_iostreams-$BOOST_VER.dylib" ]; then
 install_name_tool -id "$REPOSITORYDIR/lib/libboost_iostreams-$BOOST_VER.dylib" "$REPOSITORYDIR/lib/libboost_iostreams-$BOOST_VER.dylib";
 ln -sfn libboost_iostreams-$BOOST_VER.dylib $REPOSITORYDIR/lib/libboost_iostreams.dylib;
fi


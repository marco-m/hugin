# ------------------
#     pano13
# ------------------
# $Id: pano13.sh 1904 2007-02-05 00:10:54Z ippei $
# Copyright (c) 2007, Ippei Ukai

# prepare

# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository" \
# ARCHS="ppc i386" \
#  ppcTARGET="powerpc-apple-darwin8" \
#  i386TARGET="i386-apple-darwin8" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  ppcONLYARG="-mcpu=G3 -mtune=G4" \
#  i386ONLYARG="-mfpmath=sse -msse2 -mtune=pentium-m -ftree-vectorize" \
#  ppc64ONLYARG="-mcpu=G5 -mtune=G5 -ftree-vectorize" \
#  OTHERARGs="";
  
# -------------------------------
# 20091206.0 sg Script tested and used to build 2009.4.0-RC3
# 20100113.0 sg Script adjusted for libpano13-2.9.15
# 20100117.0 sg Move code for detecting which version of pano13 to top for visibility
# 20100119.0 sg Support the SVN version of panotools - 2.9.16
# -------------------------------

# init

# AC_INIT([pano13], [2.9.14], BUG-REPORT-ADDRESS)
libpanoVsn=$(grep "AC_INIT" configure.ac|cut -f 2 -d ,|cut -c 7-8)
case $libpanoVsn in
  "14")
        GENERATED_DYLIB_NAME="libpano13.1.0.0.dylib";
				GENERATED_DYLIB_INSTALL_NAME="libpano13.1.dylib";
				;;
  "15")
        GENERATED_DYLIB_NAME="libpano13.2.0.0.dylib";
				GENERATED_DYLIB_INSTALL_NAME="libpano13.2.dylib";
				;;
  "16")
        GENERATED_DYLIB_NAME="libpano13.2.dylib";
                                GENERATED_DYLIB_INSTALL_NAME="libpano13.dylib";
                                ;;
  "17")
        GENERATED_DYLIB_NAME="libpano13.2.dylib";
                                GENERATED_DYLIB_INSTALL_NAME="libpano13.dylib";
                                ;;
     *)
        echo "Unknown libpano version $libpanoVsn. Program aborting."
        exit 1 
        ;;
esac

let NUMARCH="0"
for i in $ARCHS
do
  NUMARCH=$(($NUMARCH + 1))
done

mkdir -p "$REPOSITORYDIR/bin";
mkdir -p "$REPOSITORYDIR/lib";
mkdir -p "$REPOSITORYDIR/include";


# compile

for ARCH in $ARCHS
do

 mkdir -p "$REPOSITORYDIR/arch/$ARCH/bin";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/lib";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/include";

 ARCHARGs=""
 MACSDKDIR=""

 if [ $ARCH = "i386" -o $ARCH = "i686" ] ; then
   TARGET=$i386TARGET
   MACSDKDIR=$i386MACSDKDIR
   ARCHARGs="$i386ONLYARG"
   OSVERSION="$i386OSVERSION"
   CC=$i386CC
   CXX=$i386CXX
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ] ; then
   TARGET=$ppcTARGET
   MACSDKDIR=$ppcMACSDKDIR
   ARCHARGs="$ppcONLYARG"
   OSVERSION="$ppcOSVERSION"
   CC=$ppcCC
   CXX=$ppcCXX
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ] ; then
   TARGET=$ppc64TARGET
   MACSDKDIR=$ppc64MACSDKDIR
   ARCHARGs="$ppc64ONLYARG"
   OSVERSION="$ppc64OSVERSION"
   CC=$ppc64CC
   CXX=$ppc64CXX
 elif [ $ARCH = "x86_64" ] ; then
   TARGET=$x64TARGET
   MACSDKDIR=$x64MACSDKDIR
   ARCHARGs="$x64ONLYARG"
   OSVERSION="$x64OSVERSION"
   CC=$x64CC
   CXX=$x64CXX
 fi
 
 [ -d ./.libs ] && rm -R ./.libs
 env \
  CC=$CC CXX=$CXX \
  CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include" \
  LDFLAGS="-L$REPOSITORYDIR/lib -mmacosx-version-min=$OSVERSION -dead_strip -prebind" \
  NEXT_ROOT="$MACSDKDIR" \
  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
  --without-java \
  --with-zlib=/usr \
  --with-png=$REPOSITORYDIR \
  --with-jpeg=$REPOSITORYDIR \
  --with-tiff=$REPOSITORYDIR \
  --enable-shared --enable-static;

 #Stupid libtool... (perhaps could be done by passing LDFLAGS to make and install)
 [ -f libtool-bk ] && rm libtool-bak
 mv "libtool" "libtool-bk"; # could be created each time we run configure
 sed -e "s#-dynamiclib#-dynamiclib -arch $ARCH -isysroot $MACSDKDIR#g" "libtool-bk" > "libtool";
 chmod +x libtool
 
 make clean;
 make;
 make install;

done

# merge libpano13

for liba in lib/libpano13.a lib/$GENERATED_DYLIB_NAME
do

 if [ $NUMARCH -eq 1 ] ; then
   if [ -f $REPOSITORYDIR/arch/$ARCHS/$liba ] ; then
		 echo "Moving arch/$ARCHS/$liba to $liba"
  	 mv "$REPOSITORYDIR/arch/$ARCHS/$liba" "$REPOSITORYDIR/$liba";
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
	if [ -f $REPOSITORYDIR/arch/$ARCH/$liba ] ; then
		echo "Adding arch/$ARCH/$liba to bundle"
		LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$liba"
	else
		echo "File arch/$ARCH/$liba was not found. Aborting build";
		exit 1;
	fi
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$liba";
 #Power programming: if filename ends in "a" then ...
 [ ${liba##*.} = a ] && ranlib "$REPOSITORYDIR/$liba";

done

if [ -f "$REPOSITORYDIR/lib/$GENERATED_DYLIB_NAME" ] ; then
  install_name_tool -id "$REPOSITORYDIR/lib/$GENERATED_DYLIB_NAME" "$REPOSITORYDIR/lib/$GENERATED_DYLIB_NAME";
	ln -sfn $GENERATED_DYLIB_NAME $REPOSITORYDIR/lib/libpano13.dylib;
  if [ $GENERATED_DYLIB_NAME = $GENERATED_DYLIB_INSTALL_NAME ] ; then : ;
	else ln -sfn $GENERATED_DYLIB_NAME $REPOSITORYDIR/lib/$GENERATED_DYLIB_INSTALL_NAME ;
  fi
fi


# merge execs

for program in bin/panoinfo bin/PTblender bin/PTcrop bin/PTinfo bin/PTmasker bin/PTmender bin/PToptimizer bin/PTroller bin/PTtiff2psd bin/PTtiffdump bin/PTuncrop
do

 LIPOARGs=""

 if [ $NUMARCH -eq 1 ] ; then
   mv "$REPOSITORYDIR/arch/$ARCHS/$program" "$REPOSITORYDIR/$program";
   install_name_tool \
     -change "$REPOSITORYDIR/arch/$ARCHS/lib/$GENERATED_DYLIB_INSTALL_NAME" "$REPOSITORYDIR/lib/libpano13.dylib" \
     "$REPOSITORYDIR/$program";
   continue
 fi

 for ARCH in $ARCHS
 do
  LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$program"
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$program";
 
 # why are we fixing the individual programs and not the universal one?
 # Here's what the link to libpano13 looks like
 # /PATHTOREPOSITORY/repository/arch/i386/lib/libpano13.2.dylib (compatibility version 3.0.0, current version 3.0.0)
 for ARCH in $ARCHS
 do
  install_name_tool \
    -change "$REPOSITORYDIR/arch/$ARCH/lib/$GENERATED_DYLIB_INSTALL_NAME" "$REPOSITORYDIR/lib/libpano13.dylib" \
    "$REPOSITORYDIR/$program";
 done

done


# libomp5

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

 ARCHARGs=""
 MACSDKDIR=""

 if [ $ARCH = "i386" -o $ARCH = "i686" ] ; then
   TARGET=$i386TARGET
   MACSDKDIR=$i386MACSDKDIR
   ARCHARGs="$i386ONLYARG"
   OSVERSION="$i386OSVERSION"
   CC=$i386CC
   CXX=$i386CXX
   ARCHFLAG="-m32"
 else [ $ARCH = "x86_64" ] ;
   TARGET=$x64TARGET
   MACSDKDIR=$x64MACSDKDIR
   ARCHARGs="$x64ONLYARG"
   OSVERSION="$x64OSVERSION"
   CC=$x64CC
   CXX=$x64CXX
   ARCHFLAG="-m64"
 fi

  env \
  CC=$CC CXX=$CXX \
    CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2" \
  CPPFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -I$REPOSITORYDIR/include" \
  OBJCFLAGS="-arch $ARCH" \
  OBJCXXFLAGS="-arch $ARCH" \
  LDFLAGS="-L$REPOSITORYDIR/lib -arch $ARCH -mmacosx-version-min=$OSVERSION -dead_strip -prebind" \
  cmake -DCMAKE_INSTALL_PREFIX="$REPOSITORYDIR" \
  -DCMAKE_OSX_SYSROOT="macosx10.11" -DCMAKE_OSX_DEPLOYMENT_TARGET=$OSVERSION \
  -DLIBOMP_ARCH="32e"

done

make
make install

install_name_tool -id "$REPOSITORYDIR/lib/libiomp5.dylib" "$REPOSITORYDIR/lib/libiomp5.dylib"
ln -sf "$REPOSITORYDIR/lib/libiomp5.dylib" "$REPOSITORYDIR/lib/libomp.dylib"

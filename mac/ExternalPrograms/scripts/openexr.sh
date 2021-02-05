#!/usr/bin/env bash
# ------------------
#     openexr
# ------------------
# $Id: openexr.sh 2004 2007-05-11 00:17:50Z ippei $
# Copyright (c) 2007, Ippei Ukai


# prepare

# -------------------------------
# 20091206.0 sg Script tested and used to build 2009.4.0-RC3
# 20100624.0 hvdw More robust error checking on compilation
# 201204013.0 hvdw update openexr to 17, e.g. rename script
# 20121010.0 hvdw remove ppc and ppc64 stuff
# -------------------------------

mkdir -p build
cd build

CC="$CC" CXX="$CXX" \
LDFLAGS="$LDARGS" \
cmake .. -DCMAKE_INSTALL_PREFIX="$REPOSITORYDIR" -DBUILD_TESTING=OFF \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOY_TARGET" -DCMAKE_OSX_SYSROOT="$MACSDKDIR" \
  -DCMAKE_INSTALL_NAME_DIR="$REPOSITORYDIR/lib" \
  || fail "cmake step";

make $MAKEARGS || fail "make step";
make install || fail "make install step";

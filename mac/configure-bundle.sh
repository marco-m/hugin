TYPE="Release"
PREFIX="/"

SDKVERSION=$(xcrun --show-sdk-version)
REPOSITORYDIR=$(cd .. && pwd)"/hugin/mac/ExternalPrograms/repository"

MACOSX_DEPLOYMENT_TARGET=10.10 \
PKG_CONFIG_PATH=$REPOSITORYDIR/lib/pkgconfig \
CC=/usr/local/opt/llvm/bin/clang CXX=/usr/local/opt/llvm/bin/clang++ \
cmake ../hugin -B. \
-DCMAKE_OSX_SYSROOT="macosx${SDKVERSION}" \
-DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_FIND_ROOT_PATH="$REPOSITORYDIR" \
-DBUILD_HSI=OFF -DENABLE_LAPACK=ON -DMAC_SELF_CONTAINED_BUNDLE=ON \
-DCMAKE_BUILD_TYPE="$TYPE" -G "Unix Makefiles"

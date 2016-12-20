cd ../../build
sudo checkinstall \
    --install=no \
    --fstrans=yes \
    --nodoc \
    --maintainer="Google" \
    --pkgsource="https://github.com/google/protobuf.git" \
    --pkglicense=BSD \
    --pkggroup=net \
    --pkgname=iibprotobuf10 \
    --pkgversion=3.0.0-b1 \
    --pkgarch=$(dpkg --print-architecture) \
    $@
cd ../package/locnet


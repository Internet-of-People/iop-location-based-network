sudo checkinstall \
    --install=no \
    --fstrans=yes \
    --nodoc \
    --maintainer="Google" \
    --pkgsource="https://github.com/google/protobuf.git" \
    --pkglicense=BSD \
    --pkggroup=net \
    --pkgname=libprotobuf10 \
    --pkgversion=3.5.0-b1 \
    --pkgarch=$(dpkg --print-architecture) \
    $@

cp description-pak ../../build/
cd ../../build
sudo checkinstall \
    --install=no \
    --fstrans=yes \
    --nodoc \
    --maintainer="IoP Developers \<developer@iop-ventures.com\>" \
    --pkgsource="https://github.com/Internet-of-People/iop-location-based-network.git" \
    --pkglicense=GPLv3 \
    --pkggroup=net \
    --pkgname=iop-locnet \
    --pkgversion=2.0.0-a1 \
    --pkgarch=$(dpkg --print-architecture) \
    $@

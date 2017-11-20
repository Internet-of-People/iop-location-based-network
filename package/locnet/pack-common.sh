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
    --pkgversion=1.2.0-b1 \
    --pkgarch=$(dpkg --print-architecture) \
    $@

cp description-pak ../../build/
cd ../../build
sudo checkinstall \
    --install=no \
    --fstrans=yes \
    --nodoc \
    --maintainer="Fermat Developers \<fermat-developers.group@fermat.org\>" \
    --pkgsource="https://github.com/Fermat-ORG/iop-location-based-network.git" \
    --pkglicense=MIT \
    --pkggroup=net \
    --pkgname=iop-locnet \
    --pkgversion=1.0.0-a1 \
    --pkgarch=$(dpkg --print-architecture) \
    $@

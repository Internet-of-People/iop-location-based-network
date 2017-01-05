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
    --pkgversion=0.0.1-a4 \
    --pkgarch=$(dpkg --print-architecture) \
    $@
cd ../package/locnet


# Package Description: IoP is a digital currency from Fermat for the Internet of People that enables instant payments to anyone, anywhere in the world.

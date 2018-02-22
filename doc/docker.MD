#How to Build

## ARM 

docker build  -f Dockerfile.libspatiallite-arm7-build -t build .

docker run -it build /bin/sh

docker ps

docker cp 8eba45b4af2d:/iop-locnet/build/src/iop-locnetd  docker/iop-locnetd

docker build  -f Dockerfile.arm7 -t libertaria/iop-loc:arm7-v2.0.0 .

## x64

docker push libertaria/iop-loc:amd64-v2.0.0

## Manifest

 manifest-tool push from-spec docker/iop-loc.yaml

### Installing manifest tool
wget https://github.com/estesp/manifest-tool/releases/download/v0.7.0/manifest-tool-linux-arm64
mv manifest-tool-linux-arm64 manifest-tool
chmod +x manifest-tool
./manifest-tool --help # Just to check that it's working
mv manifest-tool /usr/local/bin/



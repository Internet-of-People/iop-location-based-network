#How to Build

## ARM 

Build the build container
```
docker build  -f Dockerfile.libspatiallite-arm7-build -t build .
docker run -it build /bin/sh
```


Find the Running container
```
docker ps
```

Copy the artifact

```
docker cp 8eba45b4af2d:/iop-locnet/build/src/iop-locnetd  docker/iop-locnetd
```

Build the container
```
docker build  -f Dockerfile.arm7 -t libertaria/iop-loc:arm7-v2.0.0 .
```

## x64

First, you have to install all external dependencies.  
If you are on Ubuntu 17.04 you are lucky and just have to do:

    apt-get install -y g++ cmake protobuf-compiler libprotobuf-dev libspatialite-dev

Otherwise, for others Ubuntu distributions from 14.04, you will have to compile a protobuf compiler by yourself:

    apt-get install -y curl g++ cmake automake autoconf libprotobuf-dev libspatialite-dev  
    curl -OL https://github.com/google/protobuf/releases/download/v3.5.0/protobuf-cpp-3.5.0.tar.gz
    tar zxvf protobuf-cpp-3.5.0.tar.gz
    cd protobuf-3.5.0
    ./configure
    make
    make install
    ldconfig

Once this done, go inside the folder where you cloned or uncompressed this project in order to start to type the build commands.

Optionally, you can regenerate the C++ sources from the protocol definitions in order to make sure these files are compatible with your protobuf version:

    cd generated && ./regenerate.sh && cd ..

Finally, you can just run `build.sh` or perform the same steps as the script as follows:

First you have to generate build files with CMake, specifying the directory
containing our main `CMakeLists.txt` file. We suggest running the build in a dedicated directory,
the provided script uses directory `build`. Thus from the project root you can run

    mkdir build
    cd build
    cmake ..

CMake should have successfully generated a `Makefile`, so you can just execute

    make

Assuming the stars aligned luckily for your environment and you followed the suggested directory structure, you find the executable file `src/iop-locnetd` created under your `build` directory.
If you also want to install the software to your system directories, you have to run

    make install

docker push libertaria/iop-loc:amd64-v2.0.0

## Manifest

 manifest-tool push from-spec docker/iop-loc.yaml

### Installing manifest tool
wget https://github.com/estesp/manifest-tool/releases/download/v0.7.0/manifest-tool-linux-arm64
mv manifest-tool-linux-arm64 manifest-tool
chmod +x manifest-tool
./manifest-tool --help # Just to check that it's working
mv manifest-tool /usr/local/bin/



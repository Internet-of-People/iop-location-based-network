#How to Build

## ARM 

### Build the container
```
docker build  -f Dockerfile.arm7 -t libertaria/iop-loc:arm7-v2.0.0 .
```

### Run the container

```
docker run   \
-e NODEID=058ab41184872391449f17d092f4c240ed87e3e9  \
-e PUBLIC_IP=109.41.3.136  \
-e LATITUDE=52.4713  \
-e LONGITUDE=13.5168  \
-e CONFIGFILE=/data/locnet/iop-locnet.cfg  \
-e DBPATH=/data/locnet/iop-locnet.sqlite  \
-e LOCALPORT=16982  \
-e CLIENTPORT=16981  \
-e IOPLOC_PATH=/data/locnet/  \
-e NODEPORT=16980  \
-e LOGPATH=/data/locnet/debug.log  \
-e LOCALDEVICE= 172.17.0.1 \
-p 16980:16980  \
-p 16981:16981  \
-p 16982:16982  \
libertaria/iop-loc:arm7-v2.0.0

```


### Push the container

This is only needed if the container is needed by other people

```
docker push libertaria/iop-loc:amd64-v2.0.0
```

## x64

### Build the container
```
docker build  -f Dockerfile.yml  -t libertaria/iop-loc:amd64-v2.0.0 .
```

### Run the container

```
docker run   \
-e NODEID=058ab41184872391449f17d092f4c240ed87e3e9  \
-e PUBLIC_IP=109.41.3.136  \
-e LATITUDE=52.4713  \
-e LONGITUDE=13.5168  \
-e CONFIGFILE=/data/locnet/iop-locnet.cfg  \
-e DBPATH=/data/locnet/iop-locnet.sqlite  \
-e LOCALPORT=16982  \
-e CLIENTPORT=16981  \
-e IOPLOC_PATH=/data/locnet/  \
-e NODEPORT=16980  \
-e LOGPATH=/data/locnet/debug.log  \
-e LOCALDEVICE= 172.17.0.1 \
-p 16980:16980  \
-p 16981:16981  \
-p 16982:16982  \
libertaria/iop-loc:amd64-v2.0.0
```

### Push the container

This is only needed if the container is needed by other people



```
docker push libertaria/iop-loc:amd64-v2.0.0
```

## x86

### Build the container
```
docker build  -f Dockerfile.x86  -t libertaria/iop-loc:x86-v2.0.0 .
```

### Run the container

```
docker run   \
-e NODEID=058ab41184872391449f17d092f4c240ed87e3e9  \
-e PUBLIC_IP=109.41.3.136  \
-e LATITUDE=52.4713  \
-e LONGITUDE=13.5168  \
-e CONFIGFILE=/data/locnet/iop-locnet.cfg  \
-e DBPATH=/data/locnet/iop-locnet.sqlite  \
-e LOCALPORT=16982  \
-e CLIENTPORT=16981  \
-e IOPLOC_PATH=/data/locnet/  \
-e NODEPORT=16980  \
-e LOGPATH=/data/locnet/debug.log  \
-e LOCALDEVICE= 172.17.0.1 \
-p 16980:16980  \
-p 16981:16981  \
-p 16982:16982  \
libertaria/iop-loc:x86-v2.0.0
```

### Push the container

This is only needed if the container is needed by other people

```
docker push libertaria/iop-loc:x86-v2.0.0
```


## Manifest

 manifest-tool push from-spec docker/iop-loc.yaml

### Installing manifest tool
wget https://github.com/estesp/manifest-tool/releases/download/v0.7.0/manifest-tool-linux-arm64
mv manifest-tool-linux-arm64 manifest-tool
chmod +x manifest-tool
./manifest-tool --help # Just to check that it's working
mv manifest-tool /usr/local/bin/



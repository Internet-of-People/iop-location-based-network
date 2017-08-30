rm -rf IopLocNet*

echo Generating C++ sources from protocol definition
protoc -I=../src --cpp_out=. ../src/IopLocNet.proto


### Or you can also access the proto file from the repository

#mkdir /tmp/IopLocNet
#echo Downloading Protobuf protocol definitions
#wget --quiet --output-document /tmp/IopLocNet/IopLocNet.proto \
#    https://raw.githubusercontent.com/Internet-of-People/message-protocol/60a1bf7cdca63ca3814bcfa1465118e5af87b66d/IopLocNet.proto

#protoc -I=/tmp/IopLocNet --cpp_out=. /tmp/IopLocNet/IopLocNet.proto
#rm -rf /tmp/IopLocNet


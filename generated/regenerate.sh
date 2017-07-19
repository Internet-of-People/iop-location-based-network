rm -rf IopLocNet*

echo Generating C++ sources from protocol definitions
protoc -I=../src --cpp_out=. ../src/IopLocNet.proto


### NOTE older version downloading the proto file from the repository

#mkdir /tmp/IopLocNet
#echo Downloading Protobuf protocol definitions
#wget --quiet --output-document /tmp/IopLocNet/IopLocNet.proto \
#    https://raw.githubusercontent.com/Internet-of-People/message-protocol/master/IopLocNet.proto

#protoc -I=/tmp/IopLocNet --cpp_out=. /tmp/IopLocNet/IopLocNet.proto
#rm -rf /tmp/IopLocNet


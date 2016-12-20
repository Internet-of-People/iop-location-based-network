rm -rf IopLocNet*
mkdir /tmp/IopLocNet
echo Downloading Protobuf protocol definitions
wget --quiet --output-document /tmp/IopLocNet/IopLocNet.proto \
    https://raw.githubusercontent.com/Internet-of-People/message-protocol/master/IopLocNet.proto
echo Generating C++ sources from protocol definitions
protoc -I=/tmp/IopLocNet --cpp_out=. /tmp/IopLocNet/IopLocNet.proto
rm -rf /tmp/IopLocNet

# echo Generating empty SpatiaLite database
# rm locnet.sqlite
# ../build/test/gendb locnet.sqlite


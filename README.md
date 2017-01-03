![alt text](https://raw.githubusercontent.com/Fermat-ORG/media-kit/00135845a9d1fbe3696c98454834efbd7b4329fb/MediaKit/Logotype/fermat_logo_3D/Fermat_logo_v2_readme_1024x466.png "Fermat Logo")

# iop-location-based-network
[![Build Status](https://travis-ci.org/Fermat-ORG/iop-location-based-network.svg?branch=master)](https://travis-ci.org/Fermat-ORG/iop-location-based-network)

Header-only dependencies already included in directory extlib:
- easylogging++ 9.89, used for logging in the whole source,
  [download here](https://github.com/easylogging/easyloggingpp)
- asio 1.11.0, used for networking,
  [download here](http://think-async.com/Asio/Download)
- ezOptionParser 0.2.2, used for handling options from command line and config files,
  [download here](http://ezoptionparser.sourceforge.net/)
- catch 1.5.9, used only for testing,
  [download here](https://github.com/philsquared/Catch)

Dependencies to be installed:
- libprotobuf-dev package. Included sources are generated with 3.x, but 2.x should also work
  after minimal changes on the protocol definition. Available as an Ubuntu package or
  [download and compile cpp version manually](https://github.com/google/protobuf)
- libspatialite-dev package, any recent version is expected to work. Available as an Ubuntu package or
  [download and compile manually](https://www.gaia-gis.it/fossil/libspatialite/index)

Generated sourcesa already included in directory generated:
- ProtoBuf classes used for messaging. Provided script regenerate.sh also needs a protobuf compiler.
  The protocol definition can be [downloaded from here](https://raw.githubusercontent.com/Internet-of-People/message-protocol/master/IopLocNet.proto)

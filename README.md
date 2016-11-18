![alt text](https://raw.githubusercontent.com/Fermat-ORG/media-kit/00135845a9d1fbe3696c98454834efbd7b4329fb/MediaKit/Logotype/fermat_logo_3D/Fermat_logo_v2_readme_1024x466.png "Fermat Logo")

# location-based-network
[![Build Status](https://travis-ci.org/Fermat-ORG/iop-location-based-network.svg?branch=master)](https://travis-ci.org/Fermat-ORG/iop-location-based-network)

Header-only dependencies already included in extlib:
- easylogging++ 9.83, logging in the whole source,
  [download here](https://github.com/easylogging/easyloggingpp)
- asio 1.11.0, used for networking,
  [download here](http://think-async.com/Asio/Download)
- catch 1.5.0, used only for testing,
  [download here](https://github.com/philsquared/Catch)

Dependencies to be installed:
- protoc-compiler package, preferably 3.x, but also should work with 2.x
  after minimal changes on the protocol definition. Available as an Ubuntu package or
  [download and manually compile cpp version](https://github.com/google/protobuf)


# TODO - libssl-dev

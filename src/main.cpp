#include <iostream>

#include "easylogging++.h"
#include "locnet.hpp"
#include "network.hpp"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace LocNet;



int main()
{
    NetworkInterface listenOn(AddressType::Ipv4, "127.0.0.1", 4567);
    TcpNetwork network(listenOn);
    string line;
    getline(cin, line);
    LOG(INFO) << "Located works";
    return 0;
}

#include <iostream>

#include "easylogging++.h"
#include "locnet.hpp"

using namespace std;

INITIALIZE_EASYLOGGINGPP


int main()
{
    LOG(INFO) << "Located works";
    return 0;
}

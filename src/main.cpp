#include <iostream>

#include "node.h"

using namespace std;


int main()
{
    Node node("Located", 0, 0);
    cout << node.id() << " works" << endl;
    return 0;
}

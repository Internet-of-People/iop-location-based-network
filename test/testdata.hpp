#ifndef __LOCNET_TEST_DATA_H__
#define __LOCNET_TEST_DATA_H__

#include "locnet.hpp"


namespace LocNet
{
    

struct TestData
{
    static GpsLocation Budapest;
    static GpsLocation Kecskemet;
    static GpsLocation Wien;
    static GpsLocation London;
    static GpsLocation NewYork;
    static GpsLocation CapeTown;
    
    static NodeInfo NodeBudapest;
    static NodeInfo NodeKecskemet;
    static NodeInfo NodeWien;
    static NodeInfo NodeLondon;
    static NodeInfo NodeNewYork;
    static NodeInfo NodeCapeTown;
    
    static NodeDbEntry EntryKecskemet;
    static NodeDbEntry EntryWien;
    static NodeDbEntry EntryLondon;
    static NodeDbEntry EntryNewYork;
    static NodeDbEntry EntryCapeTown;
};


} // namespace LocNet


#endif // __LOCNET_TEST_IMPLEMENTATIONS_H__
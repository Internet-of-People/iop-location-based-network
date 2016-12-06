#ifndef __LOCNET_CONFIG_H__
#define __LOCNET_CONFIG_H__

#include "basic.hpp"


namespace LocNet
{



class IConfig
{
    virtual NodeInfo myNodeInfo() const = 0;
};



class Config : public IConfig
{
    static Config _instance;
    
    NodeId          _id;
    Address         _ipAddr;
    TcpPort         _port;
    GpsCoordinate   _latitude;
    GpsCoordinate   _longitude;
    
    Config();
    
public:

    static bool Initialize(int argc, const char *argv[]);
    static const Config& Instance();
    
    NodeInfo myNodeInfo() const override;
};



} // namespace LocNet


#endif // __LOCNET_BUSINESS_LOGIC_H__

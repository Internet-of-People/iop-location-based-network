#ifndef __LOCNET_CONFIG_H__
#define __LOCNET_CONFIG_H__

#include <memory>

#include "basic.hpp"


namespace LocNet
{



class Config
{
    static std::unique_ptr<Config> _instance;
    
protected:
    
    virtual bool Initialize(int argc, const char *argv[]) = 0;
    
public:
    
    static bool Init(int argc, const char *argv[]);
    static const Config& Instance();
    
    virtual NodeInfo myNodeInfo() const = 0;
};



class EzParserConfig : public Config
{
    NodeId          _id;
    Address         _ipAddr;
    TcpPort         _port;
    GpsCoordinate   _latitude;
    GpsCoordinate   _longitude;
    
public:

    bool Initialize(int argc, const char *argv[]) override;
    
    NodeInfo myNodeInfo() const override;
};



} // namespace LocNet


#endif // __LOCNET_BUSINESS_LOGIC_H__

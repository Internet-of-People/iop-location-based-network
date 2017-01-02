#ifndef __LOCNET_CONFIG_H__
#define __LOCNET_CONFIG_H__

#include <chrono>
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

    virtual ~Config() {}
    
    virtual const std::string& version() const;
    virtual bool versionRequested() const = 0;
    
    virtual const NodeInfo& myNodeInfo() const = 0;
    virtual const std::vector<Address>& seedNodes() const = 0;
    virtual TcpPort defaultPort() const = 0;
    
    virtual const std::string& dbPath() const = 0;
    virtual std::chrono::duration<uint32_t> dbMaintenancePeriod() const = 0;
    virtual std::chrono::duration<uint32_t> dbExpirationPeriod() const = 0;
    virtual std::chrono::duration<uint32_t> discoveryPeriod() const = 0;
};



class EzParserConfig : public Config
{
    static const std::chrono::duration<uint32_t>    _dbMaintenancePeriod;
    static const std::chrono::duration<uint32_t>    _dbExpirationPeriod;
    static const std::chrono::duration<uint32_t>    _discoveryPeriod;
    
    bool            _versionRequested;
    NodeId          _id;
    Address         _ipAddr;
    TcpPort         _port;
    GpsCoordinate   _latitude;
    GpsCoordinate   _longitude;
    std::string     _dbPath;
    std::vector<std::string> _seedNodes;
    
    std::unique_ptr<NodeInfo> _myNodeInfo;
    
public:

    bool Initialize(int argc, const char *argv[]) override;
    
    bool versionRequested() const override;
    
    const NodeInfo& myNodeInfo() const override;
    const std::vector<Address>& seedNodes() const override;
    TcpPort defaultPort() const override;
    
    const std::string& dbPath() const override;
    std::chrono::duration<uint32_t> dbMaintenancePeriod() const override;
    std::chrono::duration<uint32_t> dbExpirationPeriod() const override;
    std::chrono::duration<uint32_t> discoveryPeriod() const override;
};



} // namespace LocNet


#endif // __LOCNET_BUSINESS_LOGIC_H__

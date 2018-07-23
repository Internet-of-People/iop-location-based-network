#ifndef __LOCNET_CONFIG_H__
#define __LOCNET_CONFIG_H__

#include <chrono>
#include <memory>
#include <vector>

#include "basic.hpp"


namespace LocNet
{



// Abstract base class for project configuration.
// Built with the singleton pattern.
class Config
{
protected:

    Config();
    Config(const Config &other) = delete;
    Config& operator=(const Config &other) = delete;
    
public:

    virtual ~Config() {}
    
    virtual const std::string& version() const;
    
    virtual const NodeInfo& myNodeInfo() const = 0;
    virtual const NetworkEndpoint& localServiceEndpoint() const = 0;
    
    virtual const std::string& logPath() const = 0;
    virtual const std::string& dbPath() const = 0;
    
    virtual bool isTestMode() const = 0;
    virtual const std::vector<NetworkEndpoint>& seedNodes() const = 0;
    virtual size_t neighbourhoodTargetSize() const = 0;
    
    virtual std::chrono::duration<uint32_t> requestExpirationPeriod() const = 0;
    virtual std::chrono::duration<uint32_t> dbMaintenancePeriod() const = 0;
    virtual std::chrono::duration<uint32_t> dbExpirationPeriod() const = 0;
    virtual std::chrono::duration<uint32_t> discoveryPeriod() const = 0;
};



// The currently preferred Config implementation using the ezOptionParser library
// to parse options from the command line and/or config files.
class EzParserConfig : public Config
{
    static const std::chrono::duration<uint32_t> _requestExpirationPeriod;
    static const std::chrono::duration<uint32_t> _dbMaintenancePeriod;
    static const std::chrono::duration<uint32_t> _dbExpirationPeriod;
    static const std::chrono::duration<uint32_t> _discoveryPeriod;
    
    bool            _testMode = false;
    bool            _versionRequested = false;
    NodeId          _nodeId;
    Address         _ipAddr;
    TcpPort         _nodePort = 0;
    TcpPort         _clientPort = 0;
    NetworkEndpoint _localEndpoint = NetworkEndpoint("",0);
    GpsCoordinate   _latitude = 0;
    GpsCoordinate   _longitude = 0;
    std::string     _logPath;
    std::string     _dbPath;
    std::vector<NetworkEndpoint> _seedNodes;
    
    std::unique_ptr<NodeInfo> _myNodeInfo;
    
public:

    bool Initialize(int argc, const char *argv[]);
    bool versionRequested() const;

    const NodeInfo& myNodeInfo() const override;
    const NetworkEndpoint& localServiceEndpoint() const override;
    
    const std::string& logPath() const override;
    const std::string& dbPath() const override;
    
    bool isTestMode() const override;
    const std::vector<NetworkEndpoint>& seedNodes() const override;
    size_t neighbourhoodTargetSize() const override;
    
    std::chrono::duration<uint32_t> requestExpirationPeriod() const override;
    std::chrono::duration<uint32_t> dbMaintenancePeriod() const override;
    std::chrono::duration<uint32_t> dbExpirationPeriod() const override;
    std::chrono::duration<uint32_t> discoveryPeriod() const override;
};



} // namespace LocNet


#endif // __LOCNET_BUSINESS_LOGIC_H__

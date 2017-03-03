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
    static std::unique_ptr<Config> _instance;
    
protected:

    bool _testMode = false;
    
    virtual bool Initialize(int argc, const char *argv[]) = 0;
    
    Config();
    Config(const Config &other) = delete;
    Config& operator=(const Config &other) = delete;
    
public:

    // Used to initialize a config instance with command line parameters, call only once
    static bool Init(int argc, const char *argv[]);
    static void InitForTest();
    
    // Used to access the singleton object after it's properly initialized
    static const Config& Instance();
    

    virtual ~Config() {}
    
    virtual bool isTestMode() const;
    virtual const std::string& version() const;
    virtual size_t neighbourhoodTargetSize() const;
    
    virtual bool versionRequested() const = 0;
    
    virtual const NodeInfo& myNodeInfo() const = 0;
    virtual TcpPort localServicePort() const = 0;
    virtual const std::vector<NetworkEndpoint>& seedNodes() const = 0;
    
    virtual const std::string& logPath() const = 0;
    virtual const std::string& dbPath() const = 0;
    virtual std::chrono::duration<uint32_t> dbMaintenancePeriod() const = 0;
    virtual std::chrono::duration<uint32_t> dbExpirationPeriod() const = 0;
    virtual std::chrono::duration<uint32_t> discoveryPeriod() const = 0;
};



// The currently preferred Config implementation using the ezOptionParser library
// to parse options from the command line and/or config files.
class EzParserConfig : public Config
{
    static const std::chrono::duration<uint32_t> _dbMaintenancePeriod;
    static const std::chrono::duration<uint32_t> _dbExpirationPeriod;
    static const std::chrono::duration<uint32_t> _discoveryPeriod;
    
    bool            _versionRequested;
    NodeId          _nodeId;
    Address         _ipAddr;
    TcpPort         _nodePort;
    TcpPort         _clientPort;
    TcpPort         _localPort;
    GpsCoordinate   _latitude;
    GpsCoordinate   _longitude;
    std::string     _logPath;
    std::string     _dbPath;
    std::vector<NetworkEndpoint> _seedNodes;
    
    std::unique_ptr<NodeInfo> _myNodeInfo;
    
public:

    bool Initialize(int argc, const char *argv[]) override;
    
    bool versionRequested() const override;
    
    const NodeInfo& myNodeInfo() const override;
    TcpPort localServicePort() const override;
    const std::vector<NetworkEndpoint>& seedNodes() const override;
    
    const std::string& logPath() const override;
    const std::string& dbPath() const override;
    std::chrono::duration<uint32_t> dbMaintenancePeriod() const override;
    std::chrono::duration<uint32_t> dbExpirationPeriod() const override;
    std::chrono::duration<uint32_t> discoveryPeriod() const override;
};



} // namespace LocNet


#endif // __LOCNET_BUSINESS_LOGIC_H__

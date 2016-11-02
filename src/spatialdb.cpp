#include "spatialdb.hpp"



LocNetNodeDbEntry::LocNetNodeDbEntry(const LocNetNodeDbEntry& other) :
    LocNetNodeInfo(other), _relationType(other._relationType), _roleType(other._roleType) {}


LocNetNodeDbEntry::LocNetNodeDbEntry( const NodeProfile& profile, const GpsLocation& location,
                                      LocNetRelationType relationType, PeerContactRoleType roleType ) :
    LocNetNodeInfo(profile, location), _relationType(relationType), _roleType(roleType) {}

LocNetNodeDbEntry::LocNetNodeDbEntry( const LocNetNodeInfo& info,
                                      LocNetRelationType relationType, PeerContactRoleType roleType) :
    LocNetNodeInfo(info), _relationType(relationType), _roleType(roleType) {}

    
LocNetRelationType LocNetNodeDbEntry::relationType() const { return _relationType; }
PeerContactRoleType LocNetNodeDbEntry::roleType() const { return _roleType; }


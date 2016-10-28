#include "spatialdb.hpp"



LocNetNodeDbEntry::LocNetNodeDbEntry( const NodeProfile& profile, const GpsLocation& location,
                                      LocNetRelationType relationType, PeerRoleType roleType ) :
    LocNetNodeInfo(profile, location), _relationType(relationType), _roleType(roleType) {}

LocNetNodeDbEntry::LocNetNodeDbEntry( const LocNetNodeInfo& info,
                                      LocNetRelationType relationType, PeerRoleType roleType) :
    LocNetNodeInfo(info), _relationType(relationType), _roleType(roleType) {}

    
LocNetRelationType LocNetNodeDbEntry::relationType() const { return _relationType; }
PeerRoleType LocNetNodeDbEntry::roleType() const { return _roleType; }


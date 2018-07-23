#!/bin/sh
exec 2>&1
exec /usr/local/bin/iop-locnetd --nodeid $NODEID --latitude $LATITUDE --longitude $LONGITUDE --localdevice $LOCALDEVICE

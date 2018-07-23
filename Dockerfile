FROM multiarch/alpine:amd64-edge
MAINTAINER wessel<jimfromsa@gmail.com>

ENV IOP_VERSION  v1.2-Beta
ENV IOP_PLATFORM linux-64


# Ports for node2node, client2node, interprocess communication between local services
EXPOSE 16980
EXPOSE 16981
EXPOSE 16982


# Public node id, should be an SHA256 hash of the public key of this node
#ENV NODEID

# TCP port to serve client (i.e. end user) queries. Optional,default value: 16981
ENV CLIENTPORT 16981

# TCP port to serve other IoP services running on this node. Optional, default value: 16982
ENV LOCALPORT 16982

#  TCP port to serve node to node communication. Optional, default value: 16980
#Should only be locally available
ENV NODEPORT 16980

ENV IOPLOC_PATH "/data/locnet/"

# Path to config file to load options from. Optional, default value: ~/.iop-locnet/iop-locnet.cfg
ENV CONFIGFILE "/data/locnet/iop-locnet.cfg"

#Path to db file. Optional, default value: ~/.iop-locnet/locnet.sqlite
ENV DBPATH  "/data/locnet/iop-locnet.sqlite"

#  Path to log file. Optional, default value: ~/.iop-locnet/debug.log
ENV LOGPATH  "/data/locnet/debug.log"

#   Externally accessible IP address (ipv4 or v6) to be advertised for other nodes or clients. Required for seeds only, autodetected otherwise.
#ENV HOST
#   Internally accessible IP address that is required for mercury services on the same node.
#ENV LOCALHOST

# Host name of seed node to be used instead of default seeds. You can repeat this option to define multiple custom seed nodes.
ENV NODEID  "YourNodeIdHere"

# GPS latitude of this server as real number from range ((-90,90)
ENV LATITUDE 48.2081743

#  GPS longitude of this server as real number from range (-180,180)
ENV LONGITUDE 16.3738189

# Host name of seed node to be used instead of default seeds. You can repeat this option to define multiple custom seed nodes.
#ENV SEEDNODE


#add glibc
RUN ALPINE_GLIBC_BASE_URL="https://github.com/sgerrand/alpine-pkg-glibc/releases/download" && \
    ALPINE_GLIBC_PACKAGE_VERSION="2.26-r0" && \
    ALPINE_GLIBC_BASE_PACKAGE_FILENAME="glibc-$ALPINE_GLIBC_PACKAGE_VERSION.apk" && \
    ALPINE_GLIBC_BIN_PACKAGE_FILENAME="glibc-bin-$ALPINE_GLIBC_PACKAGE_VERSION.apk" && \
    ALPINE_GLIBC_I18N_PACKAGE_FILENAME="glibc-i18n-$ALPINE_GLIBC_PACKAGE_VERSION.apk" && \
    apk add --no-cache --virtual=.build-dependencies wget ca-certificates && \
    wget \
        "https://raw.githubusercontent.com/andyshinn/alpine-pkg-glibc/master/sgerrand.rsa.pub" \
        -O "/etc/apk/keys/sgerrand.rsa.pub" && \
    wget \
        "$ALPINE_GLIBC_BASE_URL/$ALPINE_GLIBC_PACKAGE_VERSION/$ALPINE_GLIBC_BASE_PACKAGE_FILENAME" \
        "$ALPINE_GLIBC_BASE_URL/$ALPINE_GLIBC_PACKAGE_VERSION/$ALPINE_GLIBC_BIN_PACKAGE_FILENAME" \
        "$ALPINE_GLIBC_BASE_URL/$ALPINE_GLIBC_PACKAGE_VERSION/$ALPINE_GLIBC_I18N_PACKAGE_FILENAME" && \
    apk add --no-cache \
        "$ALPINE_GLIBC_BASE_PACKAGE_FILENAME" \
        "$ALPINE_GLIBC_BIN_PACKAGE_FILENAME" \
        "$ALPINE_GLIBC_I18N_PACKAGE_FILENAME" && \
    \
    rm "/etc/apk/keys/sgerrand.rsa.pub" && \
    /usr/glibc-compat/bin/localedef --force --inputfile POSIX --charmap UTF-8 C.UTF-8 || true && \
    echo "export LANG=C.UTF-8" > /etc/profile.d/locale.sh && \
    \
    apk del glibc-i18n && \
    \
    rm "/root/.wget-hsts" && \
    apk del .build-dependencies && \
    rm \
        "$ALPINE_GLIBC_BASE_PACKAGE_FILENAME" \
        "$ALPINE_GLIBC_BIN_PACKAGE_FILENAME" \
        "$ALPINE_GLIBC_I18N_PACKAGE_FILENAME"

# Protobuf 3.5
RUN apk add --update protobuf protobuf-dev   \
&& /usr/glibc-compat/sbin/ldconfig


# Get the loc sourcecode
COPY . iop-locnet
ADD docker/run.sh run.sh

RUN apk add --update --repository http://dl-3.alpinelinux.org/alpine/edge/testing/  g++ make automake autoconf libspatialite-dev cmake sqlite-dev linux-headers \
&& cd /iop-locnet/generated && ./regenerate.sh && cd .. \
&&  rm -rf build && mkdir build && cd build && cmake .. && make install \
&& cd / rm -r iop-locnet &&   apk del  g++ make automake autoconf cmake protobuf-dev linux-headers  && rm -rf /var/cache/apk/*  \
&& chmod +x run.sh && mkdir -p $IOPLOC_PATH \
&& adduser -D -h $IOPLOC_PATH -u 1000 iop  \
&& chown iop:iop $IOPLOC_PATH && chmod 755 $IOPLOC_PATH && chown iop:iop /usr/local/bin/iop-locnetd

# Call uid 1000 "loc"
USER iop

# Expose the fs-repo as a volume.
# We're doing this down here (and not at the top),
# so that the overlay directory is owned by the loc user.
# start_loc initializes an ephemeral fs-repo if none is mounted,
# which is why uid=1000 needs write permissions there.
VOLUME $IOPLOC_PATH
ENTRYPOINT "/run.sh"
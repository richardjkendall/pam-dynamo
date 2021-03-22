FROM ubuntu:focal

# install base requirements
RUN apt-get update -y
RUN apt-get install -y libpam0g-dev libcurl4-openssl-dev

# install sqlite3 lib
COPY libsqlite3.so.0 /usr/local/lib

# copy in deb files
COPY *.deb /opt/

# install deb files
RUN apt install /opt/*.deb

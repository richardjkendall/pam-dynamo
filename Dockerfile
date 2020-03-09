FROM ubuntu:bionic

# install base requirements
RUN apt-get update -y
RUN apt-get install -y libpam0g-dev libcurl4-openssl-dev

# copy in deb files
COPY *.deb /opt

# install deb files
RUN apt install /opt/*.deb



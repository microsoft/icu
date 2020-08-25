# This uses the DotNet CentOS 7 docker image to build.
# https://github.com/dotnet/dotnet-buildtools-prereqs-docker

FROM mcr.microsoft.com/dotnet-buildtools/prereqs:centos-7-359e48e-20200313130914
MAINTAINER Jeff Genovy <29107334+jefgen@users.noreply.github.com>
LABEL com.github.microsoft.icu="centos-7"

# Remove icu-dev, so it doesn't conflict.
RUN yum -y remove libicu-devel

# When we run the docker container we will mount the source repo here
VOLUME /src

# The output from make install will go here
VOLUME /dist

# Do the actual build in tmp
RUN mkdir /tmp/build
WORKDIR /tmp/build

# This uses the .NET Core Ubuntu 16.04 (xenial) Cross-ARM64 docker image to build.
# https://github.com/dotnet/dotnet-buildtools-prereqs-docker

FROM mcr.microsoft.com/dotnet-buildtools/prereqs:ubuntu-16.04-cross-arm64-20210106160011-b2c2436

LABEL maintainer="Jeff Genovy <29107334+jefgen@users.noreply.github.com>"
LABEL com.github.microsoft.icu="ubuntu-16.04-cross-arm64"

# Remove icu-dev, so it doesn't conflict.
RUN apt-get -y remove libicu-dev icu-devtools

# When we run the docker container we will mount the source repo here
VOLUME /src

# The output bits will go here
VOLUME /dist

# Do the actual building in /tmp
RUN mkdir /tmp/build
WORKDIR /tmp/build

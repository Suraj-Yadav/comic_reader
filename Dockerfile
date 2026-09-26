FROM ubuntu:22.04 AS build
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y build-essential wget cmake git curl zip pkg-config

# Install vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git
ENV VCPKG_INSTALLATION_ROOT=/vcpkg

# Install stuff needed by vcpkg
RUN apt-get install -y x11-xserver-utils libxi-dev libltdl-dev 
RUN apt-get install -y libxtst-dev python3-jinja2 libxrandr-dev 
RUN apt-get install -y autoconf-archive bison python3.10-venv
RUN apt-get install -y libdbus-1-3

WORKDIR /src
# Build vcpkg stuff before
RUN cd $VCPKG_INSTALLATION_ROOT && git pull && ./bootstrap-vcpkg.sh -disableMetrics
COPY vcpkg.json ./
RUN /vcpkg/vcpkg install \
  && rm -rf /opt/vcpkg/buildtrees /opt/vcpkg/downloads

COPY . .

RUN bash ./resource/builder.sh

FROM scratch AS artifacts
COPY --from=build /src/build/Comic_Reader-x86_64.AppImage /

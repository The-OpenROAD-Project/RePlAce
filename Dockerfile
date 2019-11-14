FROM centos:centos6 AS builder

# install gcc 6
RUN yum -y install centos-release-scl && \
    yum -y install devtoolset-6 devtoolset-6-libatomic-devel
ENV CC=/opt/rh/devtoolset-6/root/usr/bin/gcc \
    CPP=/opt/rh/devtoolset-6/root/usr/bin/cpp \
    CXX=/opt/rh/devtoolset-6/root/usr/bin/g++ \
    PATH=/opt/rh/devtoolset-6/root/usr/bin:$PATH \
    LD_LIBRARY_PATH=/opt/rh/devtoolset-6/root/usr/lib64:/opt/rh/devtoolset-6/root/usr/lib:/opt/rh/devtoolset-6/root/usr/lib64/dyninst:/opt/rh/devtoolset-6/root/usr/lib/dyninst:/opt/rh/devtoolset-6/root/usr/lib64:/opt/rh/devtoolset-6/root/usr/lib:$LD_LIBRARY_PATH

# install dependencies
RUN yum install -y wget libstdc++-devel libstdc++-static libX11-devel \
    boost-devel zlib-devel tcl-devel tk-devel swig flex \
    gmp-devel mpfr-devel libmpc-devel bison \
    ImageMagick ImageMagick-devel git glibc-static zlib-static libjpeg-turbo-static

# Installing cmake for build dependency
RUN wget https://cmake.org/files/v3.9/cmake-3.9.0-Linux-x86_64.sh && \
    chmod +x cmake-3.9.0-Linux-x86_64.sh  && \
    ./cmake-3.9.0-Linux-x86_64.sh --skip-license --prefix=/usr/local

COPY . /RePlAce
RUN mkdir -p /RePlAce/build
WORKDIR /RePlAce/build
RUN cmake -DCMAKE_INSTALL_PREFIX=/build ..
RUN make

FROM centos:centos6 AS runner
RUN yum update -y && yum install -y tcl-devel libSM libX11-devel libXext libjpeg libgomp
COPY --from=builder /RePlAce/build/replace /build/replace
COPY --from=builder /RePlAce/module/OpenSTA/app/sta /build/sta
COPY --from=builder /RePlAce/test/PORT9.dat /build/share/PORT9.dat
COPY --from=builder /RePlAce/test/POST9.dat /build/share/POST9.dat
COPY --from=builder /RePlAce/test/POWV9.dat /build/share/POWV9.dat
RUN useradd -ms /bin/bash openroad
USER openroad
WORKDIR /home/openroad

FROM centos:7
LABEL maintainer="Abdelrahman Hosny <abdelrahman@brown.edu>" 

# install gcc 7
RUN yum -y install centos-release-scl && \
    yum -y install devtoolset-7 devtoolset-7-libatomic-devel
ENV CC=/opt/rh/devtoolset-7/root/usr/bin/gcc \
    CPP=/opt/rh/devtoolset-7/root/usr/bin/cpp \
    CXX=/opt/rh/devtoolset-7/root/usr/bin/g++ \
    PATH=/opt/rh/devtoolset-7/root/usr/bin:$PATH

# install dependencies 
RUN yum update -y && \
    yum install -y wget libstdc++-devel libstdc++-static libX11-devel \
    boost-devel zlib-devel tcl-devel autoconf automake swig flex libtool \
    libtool-ltdl gmp-devel mpfr-devel libmpc-devel bison byacc ctags \ 
    ImageMagick ImageMagick-devel

# install Intel MKL and IPP
RUN yum-config-manager --add-repo https://yum.repos.intel.com/setup/intelproducts.repo && \
    rpm --import https://yum.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS-2019.PUB && \
    yum install -y intel-mkl-2018.2-046 intel-ipp-2018.4-057

# add source code
COPY . RePlAce 

# install RePlAce 
RUN cd RePlAce && \
    make clean && \
    make && \
    make install

# test installation
RUN cd RePlAce/test && \
    ./run.sh

# ccncaching
cache simulation in ns3 for ccn
only test in ns-3.18.1

Installation

1、download ns-3.18.1:
wget https://www.nsnam.org/release/ns-allinone-3.18.1.tar.bz2
tar -xvf ns-allinone-3.18.1.tar.bz2
cd ns-allinone-3.18.1
./build.py

2.download ccncaching
cd ns-3.18.1/src/
git clone https://github.com/melon-li/ccncaching.git

3.configure and make ns
cd ../../
CXXFLAGS="-std=c++11" LINKFLAGS="-lbf -lpthread" ./waf configure --enable-test
./waf

if there is not libbf library, please install it as libbf github wifi
https://github.com/mavam/libbf



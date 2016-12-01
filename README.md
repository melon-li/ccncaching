This is the ns-3 module, called "ccncaching-HCaching".
On the basis of Yannis's "ccncaching", ccncaching-HCaching implements OPC [1] HCaching [2] in three differenct cache placement policies: edge caching, univercal caching and betweenness caching[3].
This module was tested only in ns-3-allinone.3.18, therefore follow the ns-3 instractions to set up ns-3 and the module.

#Installation ns-3.18.1

##Download ns-3.18.1 

```shell
$ wget https://www.nsnam.org/release/ns-allinone-3.18.1.tar.bz2

$ tar -xvf ns-allinone-3.18.1.tar.bz

$ cd ns-allinone-3.18.1
```

## Modify point-to-point module
```shell
$ vi ns-3.18.1/src/point-to-point/model/point-to-point-net-device.cc +603
```

You should add ccn protocol support code as follows
```cpp
uint16_t
PointToPointNetDevice::PppToEther (uint16_t proto)
{
  switch(proto)
    {
    case 0x0021: return 0x0800;   //IPv4
    case 0x0057: return 0x86DD;   //IPv6
    case 0x0061: return 0x88DD;   //ccn
    default: NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
    }
  return 0;
}

uint16_t
PointToPointNetDevice::EtherToPpp (uint16_t proto)
{
  switch(proto)
    {
    case 0x0800: return 0x0021;   //IPv4
    case 0x86DD: return 0x0057;   //IPv6
    case 0x88DD: return 0x0061;   //IPv6
    default: NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
    }
  return 0;
}
```

```shell
vi ns-3.18.1/src/point-to-point/model/ppp-header.cc +70
```
Add ccn support in PppHeader::Print function!
```cpp
void
PppHeader::Print (std::ostream &os) const
{
  std::string proto;

  switch(m_protocol)
    {
    case 0x0021: /* IPv4 */
      proto = "IP (0x0021)";
      break;
    case 0x0057: /* IPv6 */
      proto = "IPv6 (0x0057)";
      break;
    case 0x0061: /* ccn */
      proto = "IPv6 (0x0061)";
      break;
    default:
      NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
    }
  os << "Point-to-Point Protocol: " << proto;
}
```
##Build ns-3
Run the command as follows:
```shell
$ ./build.py
```


#Install ccncaching module

##Download ccncaching source code
```shell
$ cd ns-3.18.1/src/
```

```shell
$ git clone https://github.com/melon-li/ccncaching.git
```




##configure and make it

```shell
$ cd ../../
$ CXXFLAGS="-std=c++11" LINKFLAGS="-lbf -lpthread" ./waf configure --enable-test
$ ./waf
```

#Note
If there is not libbf library, please install it as [libbf github wiki](https://github.com/melon-li/libbf)

# CONTACT
Yannis Thomas
thomasi@aueb.gr

Melon Li
melon.haifeng@gmail.com

# REFERENCES
[1] Thomas, Yannis, et al. "Object-oriented Packet Caching for ICN" Proceedings of the 2nd international conference on Information-centric networking. ACM, 2015.
[2] H. Li,et all. "High-speed Caching for Information Centric Routers".
[3] Chai, Wei Koong, et al. "Cache “less for more” in information-centric networks." NETWORKING 2012. Springer Berlin Heidelberg, 2012. 27-40.



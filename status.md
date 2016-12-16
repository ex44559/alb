#Open vSwitch 流量优化机制进展动态

##2016.9.14 完成加入接收数据的功能

features:

- 添加ioctl_ethtool.c和ioctl_etool.h源文件，使用ioctl接口从网卡驱动中读取接收数据；
- 拓展bond_entry数据结构，添加rx_bytes统计项；
- 添加ALB_rebalance()函数,实现新的rebalance算法；
- 在vswitchd/vswitch.xml文件中添加新算法的描述；
- 更新vswitchd/vswitch.ovsschema,并更新其校验和；
- 更新系统目录下的conf.db文件，更新Open vSwitch的database。

baseline:
Open vSwitch 2.5.1

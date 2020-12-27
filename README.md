# linux下安装libevent库

~~~shell
apt-get install perl g++ make automake libtool

编译安装zlib (安装目录usr/local/lib)
tar xvf zlib-1.2.11.tar.gz
./configure
make
sudo make install 

编译安装openssl(安装目录usr/local/lib)
tar xvf openssl-1.1.1h.tar.gz
./config
make
sudo make install

编译安装libevent(安装目录usr/local/lib)
uzip libevent-master.zip
./autogen.sh 生成 configure  (automake、libtool)
./configure
make
sudo make install 
测试：test/regress > log.txt
~~~




# 参考资料

+ [libevent_C++高并发网络编程 高级篇](https://www.bilibili.com/video/BV1kZ4y137NZ?from=search&seid=15907418001775431098)

+ [C++高并发网络编程高级篇资料](https://www.jimeng365.cn/5684.html)

+ [libevent_demo](https://github.com/s290717997/libevent_demo)
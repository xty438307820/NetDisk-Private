

# 环境安装配置

## 一、安装配置mysql

安装mysql

```bash
sudo apt-get install mysql-server mysql-client
sudo apt-get install libmysqlclient-dev
```

以root身份进入mysql，这里root默认密码为空

```bash
sudo mysql -u root
```

创建并初始化数据库

```mysql
create database FileServer
use FileServer
create table UserInfo(
UserName varchar(128) primary key,
Salt char(32) not null,
PassWord char(128) not null
)

```

# 运行项目

## 一、运行服务端

编译

```bash
cd server
mkdir home virtualFile  //创建用户工作目录
mkdir build
cd build
cmake ..
make
```

修改 conf/server.conf 服务端ip和端口

```
192.168.67.128                       //ip
2000                                 //端口号
/home/yu/AOS/FileServer/server/home  //用户工作目录
```

启动服务端

```bash
./FtpServer ../conf/server.conf
```

## 二、运行客户端

编译

```bash
cd client
mkdir build
cd build
cmake ..
make
```

启动客户端

```bash
./FtpClient [服务端ip] [服务端端口]
```


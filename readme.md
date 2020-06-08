

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
//创建普通用户
mysql> CREATE USER 'abc'@'localhost' IDENTIFIED BY '123';
//创建数据库
mysql> create database FileServer;
//分配权限
mysql> GRANT ALL PRIVILEGES ON FileServer.* TO 'abc'@'localhost';
mysql> exit
//以普通用户登陆
mysql -uabc -p123
```

创建并初始化数据库

```mysql
use FileServer;
create table UserInfo(
UserName varchar(128) primary key,
Salt char(32) not null,
PassWord char(128) not null
);
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

## 三、测试

先启动服务器，再启动客户端程序

### 3.1 连接成功，注册账号

```
服务端输出:
thread pool start success
client 192.168.67.128 36714 connect
...

客户端输出:
connect success
Please choose your action:
1.login
2.register
>> 2
Please input you UserName:user1
Input password:
register success
```

### 3.2 注册成功，进行登陆

```
Please choose your action:
1.login
2.register
>> 1
Please input your UserName:user1
Input password:
login success
```


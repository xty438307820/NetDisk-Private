

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

创建并初始化数据表

```mysql
use FileServer;
create table UserInfo(
UserName varchar(128) primary key,
Salt char(32) not null,
PassWord char(128) not null
);
```



# 功能需求

## 一、基本功能

编写服务器端，服务器端启动，然后启动客户端，通过客户端可以输入以下命令进行服务器上的文件查看：

```
1.cd [目录名]           #进入对应目录，cd .. 进入上级目录
2.ls                   #列出相应目录文件
3.puts [文件名]         #将本地文件上传至服务器
4.gets [文件名]         #下载服务器文件到本地
5.remove [文件、目录名]  #删除服务器上文件或目录
6.pwd                  #显示目前所在路径
7.mkdir [目录名]        #创建目录
8.clear                #清空屏幕
9.其他命令不响应
```

## 二、扩展功能

### 2.1 用户注册，密码验证

客户端进行用户密码验证后，才可进行操作，客户端只能看到自己的文件，不能看到其他用户的文件。

服务端通过数据库存储用户名和密码。

### 2.2 断点续传

文件传输过程中断，下次传输同一文件从中断处开始传输

### 2.3 日志记录

服务端记录客户端连接时间及操作时间，写入log文件

### 2.4 超时断开

针对连接上的客户端，超过30秒未发送任何请求，关闭其描述符

### 2.5 大文件加速传递

文件大小大于100M，将大文件映射进内存，进行网络传递



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

### 3.3 登陆成功，进行操作

创建3个目录，列出目录，进入目录，查看目前所在路径

```
user1> mkdir dir1
user1> mkdir dir2
user1> mkdir dir3
user1> ls
type                        name                 size
d                           dir1                4096B
d                           dir2                4096B
d                           dir3                4096B
user1> cd dir1
user1> pwd
/dir1
user1> cd ..
user1> pwd
/
```

将本地文件上传

```
//清空屏幕
user1> clear
user1> puts The_Holy_Bible.txt
100.0%
puts success
user1> ls
type                        name                 size
d                           dir1                4096B
d                           dir2                4096B
d                           dir3                4096B
-             The_Holy_Bible.txt             4351658B
```

将本地文件The_Holy_Bible.txt删除，测试下载功能

```
user1> gets The_Holy_Bible.txt
100.00%           
gets success
```

测试remove功能，删除一个文件和一个目录

```
user1> remove The_Holy_Bible.txt
user1> remove dir1
user1> ls
type                        name                 size
d                           dir2                4096B
d                           dir3                4096B
```

等待30秒不输入命令，服务端自动断开客户端连接



日志记录查看log文件夹下的log文件

# 设计图

## 时序图

![时序图](./images/sequence_diagram.png)

## 部署图

![部署图](./images/deployment_diagram.png)

## 应用层数据包格式

————————————————————————————

|&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;数据长度（n）&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;数&nbsp;据&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;|

|&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;（4&nbsp;B）&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;（n&nbsp;B）&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;|

————————————————————————————

## 超时断开算法

使用环形队列法，环上每个节点维护一个Set<fd>，监控线程设定每隔1秒触发的定时器，每次监控线程定时器超时，前进一个节点，将此节点的fd全部断开，然后清空Set；当客户端有请求到达时，从Set中删除此fd，然后将fd加进当前节点的前一个位置的Set中。



![超时断开](./images/timing_wheel.png)
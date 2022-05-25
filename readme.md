# 一个mini且toy的linux x86-64环境下的c++协程模块
## 适用环境
- 由于实现中使用了c++的内联汇编,所以只适用于linux x86-64架构
- 目前在Debian 4.19.132-1 (2020-07-24) x86_64 GNU/Linux + g++ 8.3.0的环境上测试通过
## 使用方法
引入co_env.h文件即可,使用方法可以参考文档(不过文档没写..)
## TODOs
- [ ] 解决在co_thread中使用cout会报错的问题
- [ ] 添加日志
- [ ] 完善文档
- [o] 完善例程
    - [x] tcp echo server
    - [ ] socks5 proxy server
    - [ ] http server
- [ ] 完善测试
- [ ] 添加压测工具并进行压测



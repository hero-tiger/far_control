 # far_control
  反弹shell，上传下载文件，采用sm4加密，epoll模型支持断线重连，多客户端连接，socket通信，tcp传输协议。 
  compile：
    mkdir bulid 
    cd ./bulid
    cmake ../
    make
  usage:
    ./far_cont listen_port
    ./far_cont target_ip listen_ip

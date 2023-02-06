# HoldingHands RAT

### 简介

这是一个免费的远程监控程序

采用IOCP 模型 (但是加了很多锁，感觉是发挥不出IOCP的性能)

参考ghost 远控开发，代码中可能有很多类似的地方，但主要框架还是有区别的.

### 支持的功能:

1. 远程桌面:
    1. 支持剪切板监视
    2. 截屏功能
    3. 采用x264编码器，大幅度提高帧率，减少带宽 (但是还是gdi抓屏…..)
    4. 录屏 (待实现)
    
2. Camera:
    1. 截屏功能
    2. 录屏 (待实现)

1. Microphone
    1. 发送远程语音到本地
    2. 发送本地语音到远程
    3. 录制语音 (待实现)
    
2. 文件管理:
    1. 文件下载
    2. 文件上传(从本地)
    3. 文件上传(从URL,http(s)/ftp)
    4. 文件搜索
    5. 文件复制,删除,移动 进度框 (待实现)
    
     
    
3. Cmd
    1. 远程的shell
    2. 字体大小缩放 (待实现)
    
4. ChatBox
    1. 一个聊天框

1. Proxy:
    1. SocksProxy
        1. 支持connect ,udp 
        2. 支持socks4,socks5
    2. HTTP(s) (未实现)
    3. 内网穿透(未实现)

1. 键盘监视:
    1. 通过注入全局 消息钩子截获 WM_KEYDOWN等消息实现
    2. 改进日志储存方式 (未实现)

## 如何二次开发?

1. 客户端:
    1. 继承CMsgHandler类
    2. 重写一下函数:
        1. OnOpen() 连接打开时会调用它
        2. OnReadMsg() 读到一条完整的消息时会调用它
        3. OnWriteMsg() 没必要使用
        4. OnClose() 关闭时调用,注意不论是主动关闭,还是被动关闭,最后都一定会调用这个函数
    
    1. 服务器
        1. 继承CMsgHandler 类，这个主要是处理逻辑
        2. OnOpen() 与 OnClose() 会在主线程中调用(UI) ，以便创建窗口(如果有需要)
        3. OnReadMsg,OnWriteMsg不会在主线程调用,无法确定在那个线程中调用它
        4. 在CManager中的OnConnect 为 CClientContext 分配一个 CMsgHandler，并且在OnDisconnect 中删除该MsgHandler.

详细的步骤请参考已经开发好的模块

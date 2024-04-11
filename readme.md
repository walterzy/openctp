# **openctp（Powered by TTS - Tick Trading System）**

[openctp](https://github.com/openctp/openctp)是一个以CTP生态为基础的平台，既提供了华鑫证券奇点、中泰证券XTP、东方财富EMT、东方证券OST等柜台的[CTPAPI](https://github.com/openctp/openctp)兼容接口，也提供了一套与上期技术SimNow模拟环境类似的模拟环境，也支持CTPAPI接口，不仅提供国内各期货交易所的期货与期权品种模拟交易，还提供了A股的股票、基金、债券以及股票期权模拟交易，也支持港股、美股等市场模拟交易。

openctp还提供了CTPAPI的Python接口，开发了CTP交易客户端[ViTrader](https://github.com/openctp/ViTrader)并开放了源码，还开发了图形界面的交易客户端[TickTrader](http://www.openctp.cn/download.html),都支持openctp、CTP、CTP股票期权、中泰XTP、华鑫奇点股票与股票期权等柜台。

openctp还做了一套websocket接口的CTP服务，[webctp](https://github.com/openctp/webctp)，将CTP的服务以websocket+json形式对外提供服务，也开放了源码。

openctp还提供了CTP、华鑫奇点、中泰XTP等柜台接口的开发咨询和培训以及柜台系统等的开发培训服务。

openctp还有更多的产品和服务在研发中。。。

![ctp开放平台全景图](https://user-images.githubusercontent.com/83346523/148639077-6c328032-b75a-4979-be8d-157de60cf3b4.jpg)

# 目录结构：
- ctp2TTS：openctp模拟环境CTPAPI兼容接口。
- ctpopt2TTS:openctp模拟环境CTP股票期权兼容接口。
- ctp2XTP：中泰证券XTP柜台CTPAPI兼容接口（含源码）。
- ctp2STP：华鑫证券奇点股票柜台CTPAPI兼容接口（含源码）。
- ctp2EMT：东方财富EMT柜台CTPAPI兼容接口（含源码）。
- ctp2STPOPT：华鑫证券奇点股票期权柜台CTPAPI兼容接口（含源码）。
- ctp2OST：东方证券OST柜台CTPAPI兼容接口。
- ctp2IB：盈透证券CTPAPI兼容接口。
- ctp2QQ：腾讯行情CTPAPI兼容接口（含源码）。
- ctp2Sina：新浪行情CTPAPI兼容接口（含源码）。
- demo：CTPAPI开发相关的demo及工具源码。
- tools：生产力工具。
- docs：开发文档及行业资料。
- ctpapi-python：CTPAPI的Python接口（openctp官方开发）。
- ctpapi-opt-python：CTP股票期权API的Python接口（openctp官方开发）。
- ctpapi-java：CTPAPI的Java接口。
- ctpapi-go：CTPAPI的Go接口。
- ctpapi-c：CTPAPI的C语言接口。
- ctpapi-rust：CTPAPI的Rust语言接口。
- ctpapi-C#：CTPAPI的C#语言接口。
- widgets：图形界面小应用。

# openctp模拟环境
openctp模拟环境与上期技术SimNow模拟环境类似，均为CTPAPI接口的测试与仿真平台，CTP是上期开发的，SimNow用的也是CTP柜台，所以SimNow是CTPAPI接口的官方测试平台，openctp是自己开发了兼容CTPAPI接口的柜台系统，由于CTP柜台业务非常多，我们openctp只是从一般投资者角度考虑，只实现了一般交易过程中需要使用的接口，完整版还需要到SimNow测试，其实SimNow也没多完整，毕竟是个模拟环境，很多业务也不支持，所以有些功能还是需要在实盘环境中测试的。

openctp模拟环境有三套，一套7x24环境，不间断循环播放最新交易日的一段行情，一套为仿真环境，交易时段与实盘一致，可以用来长期验证策略的运行效果，除期货外还支持A股的股票交易。第三套也是仿真环境，不过带宽较高，提供的品种也全，除期货、期权外还提供了A股的股票、基金、债券以及股票期权的仿真交易，收费也很便宜，只要300块一年，关注openctp公众号并回复注册vip即可。

## **支持品种：**
- 沪深交易所股票、债券、基金、股票期权等
- 上期所、中金所等国内期货交易所全品种期货、期权
- 港股、美股全部股票合约模拟交易
- CME等外盘期货品种（即将上线）

## 相对Simnow优点：
- 支持负价交易（负价合约的合约号为MINUS，仅在7x24环境提供）。
- 支持部分撮合、部分撤消。
- 提供各交易所全商品模拟交易。
- 关注“CTP开放平台”公众号即可自动得到一个7x24测试账号及仿真账号，回复“注册仿真”可再注册多个仿真账号，回复“注册24”可再注册多个7x24测试账号，且立即生效。
- QQ群127235179有问必答，解答CTP及各交易相关问题。
- 真正的7x24，1秒钟都不停。
- 除国内期货及期权外，还提供A股股票、债券、基金、股票期权、港美股以及外盘期货等全球市场模拟交易。
- 支持市价单。
- 支持CTP股票期权接口。

## 撮合方式（同时支持做市与撮合）：
- 撮合：完全由用户之间撮合，按价格优先、时间优先撮合成交。撮合模式的合约只有三个，合约代码分别为TEST、BTC、MINUS，其它合约均为做市模式。
- 做市：Simnow用的就是做市模式，以实盘行情盘口做市成交，即高于叫卖价的多单立即成交，低于叫买价的空单立即成交，否则挂在队列中等行情符合条件的时候成交。

## 部分成交、部分撤消：
- 仿真环境的做市模式不会部分成交，要测试部分成交可在7x24环境交易TEST、BTC、MINUS这三个合约。

## openctp模拟环境账号注册
关注openctp公众号，回复相应信息即可注册模拟账号，即时生效，初始资金1000万。
- 7x24环境账号注册，回复注册24，每回复一次就多注册一个7x24账号，一个微信最多注册3个号。
- 仿真环境账号注册，回复注册仿真，每回复一次就多注册一个仿真账号，一个微信最多注册3个号。
- vip环境账号注册，回复注册vip，每回复一次就多注册一个vip账号，一个微信最多注册10个号。

## BrokerID、AppID、AuthCode
openctp模拟环境不检查这几个字段，3项均可不填。

## 7x24模拟环境
- 交易前置 - tcp://121.37.80.177:20002，或者使用域名tcp://openctp.cn:20002
- 行情前置 - tcp://121.37.80.177:20004，或者使用域名tcp://openctp.cn:20004
## 仿真环境
- 交易前置 - tcp://121.37.90.193:20002
- 行情前置 - 无（期货实盘行情前置见[CTP柜台实盘环境监控](http://www.openctp.cn/env.html)）
## 仿真环境
- 交易前置 - tcp://42.192.226.242:20002
- 行情前置 - 无（期货实盘行情前置见[CTP柜台实盘环境监控](http://www.openctp.cn/env.html)）
# openctp监控平台
openctp提供了一个集中监控SimNow、华鑫N视界、中泰XTP、东财EMT等模拟环境的监控平台，当然也包括openctp自己的模拟环境，有几个环境，有没开着，一眼就知道了，点这里看看：[openctp模拟环境监控](http://www.openctp.cn)。

openctp还提供了对几十家主流期货公司CTP柜台实盘环境的监控，并且标出了提供上期所免费5档行情支持的期货公司，点这里一看就知道了：[CTP柜台实盘环境监控](http://www.openctp.cn/env.html)。

![613dc093f916d1bf0764e5365f202ff](https://user-images.githubusercontent.com/83346523/148802378-2c9b3d3f-1959-4aab-851a-cf55666806d8.png)


# **CTP程序接入股票柜台：**
除提供开放平台模拟交易外，还提供使用CTP接口接入证券柜台的能力，可以进行股票、债券、逆回购、新股申购、融资融券、ETF期权等交易，同样使用CTP接口将证券柜台接口封装成跟CTP完全兼容的动态库，使得CTP程序无需任何修改，只更换CTP动态库即可接入证券柜台，目前已完成华鑫证券（股票、债券、基金、股票期权）、中泰证券（股票、债券、基金）股票交易接入功能，同样发布了目前在用的四个CTPAPI版本，分别提供win32、win64、linux三套动态库。

股票接入方式采用**直连证券柜台**方式，不经过开放平台处理，因此需要向证券公司申请模拟账号。**如需实盘接入股票柜台（个人投资者也可接入）或实盘交易请在openctp公众号回复咨询两个字。**
- **华鑫证券**，模拟账号申请地址：http://www.n-sight.com.cn
- **中泰证券**，模拟账号申请地址：https://xtp.zts.com.cn

**股票柜台接口与接入问题请加QQ群 127235179 咨询。**

# **CTPAPI及各柜台CTPAPI兼容接口下载：**
[CTP、TTS、XTP、TORA等柜台接口下载](http://www.openctp.cn/download.html)

# **已官方支持TTS通道（CTP开放平台）的产品：**
- [TickTrader（openctp出品的交易客户端，支持全球市场交易，支持TTS、CTP、华鑫证券等柜台）](http://www.openctp.cn/download.html)
![image](https://github.com/openctp/openctp/assets/83346523/bc458496-172b-4cb3-bc70-dbde12c0bc17)

- [vn.py（知名Python量化交易客户端，支持全球市场交易）](https://www.vnpy.com/)
<img src="https://user-images.githubusercontent.com/83346523/136988918-1159fc88-073e-4b6f-a8d6-3f33991e8a72.png" alt="vnpy" width="700" height="400" />

- [MT5CTP（MT5软件，已支持国内A股、期货及期权交易，QQ群：967352413，备注openctp。）](https://www.zhihu.com/people/mt5ctp)
<img src="https://user-images.githubusercontent.com/83346523/136989596-b12d91e8-48a0-4b26-bcaf-fdfca52d962c.png" alt="mt5ctp" width="700" height="400" />

- [TextTrader（CTP开源命令行交易终端，支持A股、期货及期权交易）](https://github.com/openctp/TextTrader)
<img src="https://user-images.githubusercontent.com/83346523/136989754-1f0130e6-5d75-427f-bbf3-7ed084b6eae1.png" alt="texttrader" width="700" height="400" />

- [WonderTrader（一个基于C++核心模块的，适应全市场全品种交易的，高效率、高可用的量化交易开发框架，QQ群：610730738，备注openctp。）](https://www.zhihu.com/column/c_1338797723131740161)
<img src="https://user-images.githubusercontent.com/83346523/198839414-d72614d8-9752-497a-b9a9-19b38d3da326.png" alt="WonderTrader" width="700" height="400" />

- [ctpbee（一个轻量级Python量化交易框架，支持CTP柜台。）](https://github.com/ctpbee/ctpbee)
<img src="https://github.com/openctp/openctp/assets/83346523/c0448edf-a1fe-4e7a-92c9-5a7652f83f94" alt="WonderTrader" width="700" height="400" />

# **通过自己替换dll可接入TTS通道（CTP开放平台）的产品：**
- [快期V3（CTP期货交易客户端）](https://zhuanlan.zhihu.com/p/376482285)
<img src="https://user-images.githubusercontent.com/83346523/138928402-f7e12a28-50d3-457b-9c0a-32b356448913.png" alt="快期V3" width="700" height="400" />
- [快期V2（CTP期货交易客户端）](https://zhuanlan.zhihu.com/p/432252376)
<img src="https://user-images.githubusercontent.com/83346523/148678947-b2a7ed9c-f77f-43d6-8e85-74b2c9ca4c44.png" alt="快期V2" width="700" height="400" />
- [TBTerminalCTP（交易开拓者）](https://zhuanlan.zhihu.com/p/437818698)
<img src="https://user-images.githubusercontent.com/83346523/149624377-01c97148-9e34-4f2b-8bb3-c43b57340284.png" alt="TBTerminal" width="700" height="400" />

# **粉丝交流QQ群：127235179**

<img src="https://user-images.githubusercontent.com/83346523/225706658-5dde7246-d837-4d13-9207-aea48b688184.png" alt="QQ群二维码" width="300" height="500" />

# openctp咨询服务
基于openctp积累的深厚的技术，我们为CTP、华鑫奇点、中泰XTP等柜台接入与开发提供咨询服务，有接口及实盘交易问题均可咨询，只需1000元，永久服务。

# openctp培训服务
openctp提供证券期货交易开发方面的技术培训，也提供行业无关的基础技术培训，openctp的培训偏向于就业方向，比如想去私募或者科技公司从事量化或者柜台系统开发的比较适合，当然如果想自己学习一些技术帮助自己更好地做交易也是可以的。openctp的培训是迭代式的，会不断更新，补充更多的内容，同学可在相应课程的群内永久交流。所有课程的每节课在B站上都有试看视频，报培训只需要在openctp的公众号回复培训两个字即可获取联系方式。

openctp不定期组织同学进行技术交流，为大家创造一个好的学习氛围。

## 课程介绍
- 第一期：[C/C++高级编程](https://www.bilibili.com/video/BV1mV4y1V7HM)，3000元，以krenx开发的C语言跨平台开发框架[Think库](https://github.com/krenx1983/think)为基准进行讲解，含socket网络编程、IPC进程通讯等，有众多实用的工具，可立即应用到工作中。另外还有boost.asio异步网络通讯框架等开发技术的讲解，也提供相应的实例源码。
- 第二期：[CTP、XTP等柜台接口开发技术](https://www.bilibili.com/video/BV1JP411N78s)，5000元，以openctp相关技术为基准进行讲解，含CTPAPI底层逻辑、CTPAPI各种注意事项、开源CTP客户端TextTrader源码讲解等。送高质量轻量级Tick级多策略交易框架源码（约三五千行），保持原汁原味的CTP数据结构，实时计算持仓、资金。
- 第三期：[交易系统开发](https://www.bilibili.com/video/BV1F3411f7Q9)，5000元，以TTS交易系统为基准进行讲解，含交易系统结构、架构技术、业务表结构设计、关键业务处理等。送一套完整的交易撮合系统源码，含下单、仓位与资金计算、委托回报、成交回报、撮合成交、行情推送等完整功能，正在开发中，开发完成后也将免费发给前面已报名的同学。
- 第四期：[金融交易业务与产品设计](https://www.bilibili.com/video/BV1sd4y1a7Kk)，3000元，通讲全球股票、期货、期权交易发展历程、交易规则、计算公式、风险控制及产品设计，提供一份CTP全部常用字段的详细说明。
- 第五期：[内存数据库架构交易系统总线开发技术](https://www.bilibili.com/video/BV1Bx4y1K7t7)，8000元，通过TTS的总线架构技术讲解CTP那样的总线开发技术，包括重演、热备、负载均衡、最短路由、分布式计算等技术，内存计算架构在各行业的高性能通讯方面都可以应用，远不止金融交易领域。

## openctp公开课
openctp做了一些免费的0基础学习课程，帮助更多朋友进入到软件编程与证券期货交易行业。
- [C语言公开课](https://www.bilibili.com/video/BV1CK411o743)：以生动有趣的方式讲C语言基础性编程技术，重在兴趣培养和信心建立。
- C++语言公开课：以生动有趣的方式讲C++语言基础性编程技术，课程在准备中。
- [Linux环境编程公开课](https://www.bilibili.com/video/BV1Jw411E7sF)：介绍Unix&Linux的前世今世，讲Shell、VI编辑器等使用，讲netstat、traceroute、ifconfig、lsof等网络工具的使用，讲正则表达式等等，0基础，谁都能听得懂。

# openctp公众号
<img src="https://user-images.githubusercontent.com/83346523/225707613-59293970-0f04-4056-8ea4-dd4596a4efec.png" alt="微信公众号" width="300" height="350" />

# 精品文章：
- [如何使用CTP开放平台提供的各项能力](https://mp.weixin.qq.com/s?__biz=Mzk0ODI0NDE2Ng==&mid=2247484094&idx=1&sn=97bd791622333886260bf767bea40db1&chksm=c36bd917f41c50016b676b5f5b11f899aea889cd9b10e6724c7fee0ad443f31351f87ff5a4d2&token=1790747698&lang=zh_CN#rd)
- [CTP接口开发“葵花宝典”](https://zhuanlan.zhihu.com/p/397359483)
- [CTP接口支持pip install](https://zhuanlan.zhihu.com/p/622959788)
- [CTP接口量化交易资料汇总](https://zhuanlan.zhihu.com/p/607325008)
- [开放腾讯行情CTPAPI接口源码](https://mp.weixin.qq.com/s?__biz=Mzk0ODI0NDE2Ng==&mid=2247484606&idx=1&sn=270ba6034d9e45334642236dc315b16e&chksm=c36bdf17f41c56011a5a3bf974022c13f53b21b0cad35d43ea85130748152b4f0739e02f7a0e&token=55513683&lang=zh_CN#rd)
- [开放新浪行情CTPAPI接口源码](https://zhuanlan.zhihu.com/p/585724196)
- [CTP程序无缝接入华鑫证券奇点柜台（CTP2STP）](https://mp.weixin.qq.com/s?__biz=Mzk0ODI0NDE2Ng==&mid=2247483843&idx=1&sn=fdb033a68e9f803183d902dcf92f969b&chksm=c36bda6af41c537c6ed262923f2ee9a4cccb1b02821b918382e51a299cca81556bdb4302cdd3&scene=21#wechat_redirect)
- [发布华鑫证券奇点柜台股票期权CTPAPI](https://mp.weixin.qq.com/s?__biz=Mzk0ODI0NDE2Ng==&mid=2247484767&idx=1&sn=34fc5c6b270cf8c8bdc37981df4ae8e1&chksm=c36bdef6f41c57e075460fca8d670db17310e7d832a240aa37fa0f4666ba92672aa179d8f0dd&token=1435234803&lang=zh_CN#rd)
- [开放华鑫证券奇点柜台行情CTPAPI接口源码](https://mp.weixin.qq.com/s?__biz=Mzk0ODI0NDE2Ng==&mid=2247484647&idx=1&sn=03bef5f9f71ecd879c3520de2564f8dd&chksm=c36bdf4ef41c565895c10adaa558d6ac471ac5003791cbe301771c8884757cef13400e09ca5b&token=1847931716&lang=zh_CN#rd)
- [发布一批行情显示工具（命令行版）](https://mp.weixin.qq.com/s?__biz=Mzk0ODI0NDE2Ng==&mid=2247484039&idx=1&sn=794a13777cb358e01c175439e022d99b&chksm=c36bd92ef41c5038224f3b38740b001ef3b36bec89b0ccede51a446039fd1fa679ff7b4bc3b5&token=1790747698&lang=zh_CN#rd)
- [openctp培训与咨询服务](https://mp.weixin.qq.com/s?__biz=Mzk0ODI0NDE2Ng==&mid=2247484610&idx=1&sn=b7317eb127d22fd52958a41e40121b06&chksm=c36bdf6bf41c567d4f3c16454fc1f5ff2a22f567d893ea35a2135c082b78ce5d01d944b086e4&token=55513683&lang=zh_CN#rd)

<u>*注：openctp不对模拟交易及相关服务作任何保证，使用openctp产品进行实盘交易的后果完全由使用者自己承担。*</u>

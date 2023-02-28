# SE3356-LAB1-RDT
韩金铂 520021910807 chasingdreams@sjtu.edu.cn

### 提示
- 增加了rdt_util.h和rdt_util.cc文件，包含了一些常量
- **修改了Makefile、rdt_sender.cc, rdt_receiver.cc**
- 没有修改rdt_struct.h, rdt_sender.h, rdt_sim.cc, rdt_recevier.h

### 概述
- 采用Select-Repeat ARQ
- 使用ACK+Timeout而不是NAK
- 没有使用piggyback
- 使用自然溢出计算Checksum

### Packet设计
128字节packet，包含2个Byte的HEADER，若干Byte的Payload，2个Byte的TAIL

#### HEADER
第一个byte是Payload段的长度，第二个byte是sequence number
#### Payload
存数据
#### TAIL
存Checksum, 计算方法是对于HEADER+PAYLOAD，每一位乘以它的位权再加一个偏移量，用自然溢出的方式等价于对65536取模。

### 逻辑时钟
用一个物理时钟来模拟多个逻辑时钟；logical_clock是一个list，按照它们设置的时间顺序排列；在这里因为每个逻辑时钟是等长的(TIMEOUT),在实现时可以简单一些。

对时钟的修改有三个时机，分别是调用Wrapped_startTimer(), Wrapped_stopTimer(),和Sender_Timeout();
第三个是被动调用。

Wrapped_StartTimer(int seq)负责对seq number创建一个逻辑时钟；对于一个固定的序列号，在同一时刻只能有一个时钟。
因此使用clock_set来记录某个序列号的时钟是不是在设置状态。这里要注意一个特别状态，是一开始没有一个时钟的时候。

Wrapped_StopTimer(int seq)主动停下一个逻辑时钟，这一般发生在Sender收到ACK（或者从buffer_ack中读）时。
此函数遍历从前向后遍历所有逻辑时钟，找到匹配的直接删除。

这里有一个特殊情况是删掉第一个时钟，因为第一个逻辑时钟对应着物理时钟。当第一个逻辑时钟被删除的时候，物理时钟会被重置，重置为第二个逻辑时钟，设定的timeout时间需要根据逻辑时钟设定的偏移量算出。

这里还有一个情况是，由于Timeout由函数调用触发而不是中断；因此可能会出现未触发timeout的逻辑时钟，它们需要被添加到resend_list中触发resend.
这是必要的，因为对每个序列号只有一个时钟。一个case是0号packet， 1号packet还没发到receiver就loss了; 处理0号timeout的时候，程序语句执行的时间使得1号也timeout了，如果忽略掉1号的timeout，就会使得1号永远不能重发。

### Sender_FromUpperLayer
这个函数主要接受上层传递下来的message，将其切分成packet，并存到缓存中。
用缓存是因为因为上层传递下来的包的速度可能大大快于发送的速度； 这个缓存用了一个queue。

try_sendPacket()函数尝试从缓存中拿一个包并发送。这里有两个不能发送的条件：
- 当前window中的包数目大于window_size
- buffer中没有包；

### Receiver_FromLowerLayer
首先检查checksum是否合法，如果不合法直接丢。（Sender的Timeout会重发的）

1. 如果packet的seq等于期望的seq(cur_seq_expected)，把包传到上层。
2. 如果packet的seq不等于期望的seq, 并且是这一轮的包（往往是超前接收），使用buffered_packets将包缓存起来。不丢out-of-order的包是select-repeat的精髓。
3. 如果packet的seq不等于期望的seq，并且是上一轮的包（往往是sender 重复发送），不管。

以上3种可能情况，都要发ack表示收到了这个包，为了简单，发回的ack可以就是这个packet本身。

最后，尝试让buffer中的缓存包被接收，它只能发生一个1情况之后。

### Sender_FromLowerLayer
首先检查checksum是否合法，如果不合法直接丢弃。同样依靠Sender后续的Timeout来保证正确。

1. 如果packet的seq等于期望的seq(ack_expected)，窗口向前推进一位。
2. 如果packet的seq超前于期望的seq，同样把它缓存起来。
3. 如果packet的seq延迟于期望的seq，丢弃。

同样，以上3类情况都要停止逻辑时钟，并且尝试让buffer中的缓存包被接收。
最后try_sendPacket()尝试启动新的一轮。

### 一些问题
1. 最初没有注意int->char的强制类型转换，导致一些bit被错误覆盖
2. 最初想用go-back-n，但是发现重复发包太严重；
3. 逻辑时钟的具体实现出了几个bug













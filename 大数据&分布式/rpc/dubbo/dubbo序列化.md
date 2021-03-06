# 常见序列化方式对比

1. Java原生的序列化协议把字段类型信息用字符串格式写到了二进制流里面，这样反序列化方就可以根据字段信息来反序列化。但是Java原生的序列化协议最大的问题就是生成的字节流太大

2. Hessian, Kryo这些协议不需要借助中间文件，直接把分帧信息写入了二进制流，并且没有使用字符串来存放，而是定义了特定的格式来表示这些类型信息。Hessian, Kryo生成的字节流就优化了很多，尤其是Kryo，生成的字节流大小甚至可以优于Protobuf.

3. Protobuf和Thrift利用IDL来生成中间文件，这些中间文件包含了如何分帧的信息，比如Thrift给每个字段生成了元数据，包含了顺序信息（加了id信息），和类型信息，实际写的二进制流里面包含了每个字段id, 类型，长度等分帧信息。序列化方和反序列化方共享这些中间文件来进行序列化操作。

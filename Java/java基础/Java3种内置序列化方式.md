# Java 3种内置序列化方式

一.使用默认的序列化机制，即实现Serializable接口即可，不需要实现任何方法。

该接口没有任何方法，只是一个标记而已，告诉Java虚拟机该类可以被序列化了。然后利用ObjectOutputStream进行序列化和用ObjectInputStream进行反序列化。

注意：

该方式下序列化机制会自动保存该对象的成员变量，static成员变量和transient关键字修饰的成员变量不会被序列化保存。

二.实现Externalizable接口

Externalizable接口是继承自Serializable接口的，我们在实现Externalizable接口时，必须实现writeExternal(ObjectOutput)和readExternal(ObjectInput)方法，在这两个方法下我们可以手动的进行序列化和反序列化那些需要保存的成员变量。

三.实现Serializable接口，在该实现类中再增加writeObject方法和readObject方法。该方式要严格遵循以下两个方法的方法签名：

writeObject和readObject

在这两个方法里面需要使用stream.defaultWriteObject()序列化那些非static和非transient修饰的成员变量，static的和transient的变量则用stream.writeObject(object)显式序列化。

在序列化输出的时候，writeObject(object)会检查object参数，如果object拥有自己的writeObject()方法，那么就会使用它自己的writeObject()方法进行序列化。readObject()也采用了类似的步骤进行处理。如果object参数没有writeObject()方法，在readObject方法中就不能调用stream.readObject()，否则会报错。

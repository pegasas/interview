# new、delete

new / new[]：完成两件事，先底层调用 malloc 分了配内存，然后调用构造函数（创建对象）。
delete/delete[]：也完成两件事，先调用析构函数（清理资源），然后底层调用 free 释放空间。
new 在申请内存时会自动计算所需字节数，而 malloc 则需我们自己输入申请内存空间的字节数。

申请内存，确认是否申请成功

int main()
{
    T* t = new T();     // 先内存分配 ，再构造函数
    delete t;           // 先析构函数，再内存释放
    return 0;
}
定位 new
定位 new（placement new）允许我们向 new 传递额外的参数。

new (palce_address) type
new (palce_address) type (initializers)
new (palce_address) type [size]
new (palce_address) type [size] { braced initializer list }
palce_address 是个指针
initializers 提供一个（可能为空的）以逗号分隔的初始值列表


# malloc、free

用于分配、释放内存

申请内存，确认是否申请成功

char *str = (char*) malloc(100);
assert(str != nullptr);
释放内存后指针置空

free(p);
p = nullptr;

# new/delete与malloc/free的区别
new是运算符，malloc是C语言库函数
new可以重载，malloc不能重载
new的变量是数据类型，malloc的是字节大小
new可以调用构造函数，delete可以调用析构函数，malloc/free不能
new返回的是指定对象的指针，而malloc返回的是void*，因此malloc的返回值一般都需要进行类型转化
malloc分配的内存不够的时候可以使用realloc扩容，new没有这样的操作
new内存分配失败抛出bad_malloc，malloc内存分配失败返回NULL值

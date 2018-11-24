# using

using 声明

一条 using 声明 语句一次只引入命名空间的一个成员。它使得我们可以清楚知道程序中所引用的到底是哪个名字。如：

```
using namespace_name::name;
```

构造函数的 using 声明【C++11】

在 C++11 中，派生类能够重用其直接基类定义的构造函数。

```
class Derived : Base {
public:
    using Base::Base;
    /* ... */
};
```

如上 using 声明，对于基类的每个构造函数，编译器都生成一个与之对应（形参列表完全相同）的派生类构造函数。生成如下类型构造函数：

```
derived(parms) : base(args) { }
```

using 指示

using 指示 使得某个特定命名空间中所有名字都可见，这样我们就无需再为它们添加任何前缀限定符了。如：

using namespace_name name;

尽量少使用 using 指示 污染命名空间

一般说来，使用 using 命令比使用 using 编译命令更安全，这是由于它只导入了制定的名称。如果该名称与局部名称发生冲突，编译器将发出指示。using编译命令导入所有的名称，包括可能并不需要的名称。如果与局部名称发生冲突，则局部名称将覆盖名称空间版本，而编译器并不会发出警告。另外，名称空间的开放性意味着名称空间的名称可能分散在多个地方，这使得难以准确知道添加了哪些名称。

尽量少使用 using 指示

using namespace std;
应该多使用 using 声明

int x;
std::cin >> x ;
std::cout << x << std::endl;
或者

using std::cin;
using std::cout;
using std::endl;
int x;
cin >> x;
cout << x << endl;

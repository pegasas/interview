# 模板别名using

在进入这个主题之前，各位应该先弄清楚“模板”和“类型”本质上的不同。class template (类型模板，是模板)是用来产生 template class (模板类型，是类型)。
在标准 C++，typedef 可定义模板类型一个新的类型名称，但是不能够使用 typedef 来定义模板的别名。举例来说：

```
template< typename first, typename second, int third>
class SomeType;

template< typename second>
typedef SomeType<OtherType, second, 5> TypedefName; // 在C++是不合法的
```

这不能够通过编译。

为了定义模板的别名，C++11 将会增加以下的语法：

```
template< typename first, typename second, int third>
class SomeType;

template< typename second>
using TypedefName = SomeType<OtherType, second, 5>;
```

using 也能在 C++11 中定义一般类型的别名，等同 typedef：

```
typedef void (*PFD)(double);            // 傳統語法
using PFD = void (*)(double);           // 新增語法
```

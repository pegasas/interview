# auto

C++引入auto关键字主要有两种用途：

一是在变量声明时根据初始化表达式自动推断该变量的类型
二是在声明函数时作为函数返回值的占位符

## 自动类型推断

使用auto从初始化表达式中推断出变量的数据类型，可以大大简化我们的编程工作，特别是对于一些类型冗长复杂的变量。比如，在以前的做法中，对于vector类型的变量，如果我们需要获取它的迭代器，我们需要这样声明vector::iterator iter，而使用auto关键字后我们可以让编译器帮我们推断出迭代器的具体类型。另外，在模板函数定义时，如果变量的类型依赖于模板参数，我们也很难确定变量的类型，使用auto关键字则可以把这些“脏活累活”交给编译器完成。

```
#include <iostream>
#include <vector>
using namespace std;

template<class T, class U>
void add(T t, U u)
{
    auto s = t + u;
    cout << "type of t + u is " << typeid(s).name() << endl;
}

int main()
{
    // 简单自动类型推断
    auto a = 123;
    cout << "type of a is " << typeid(a).name() << endl;
    auto s("fred");
    cout << "type of s is " << typeid(s).name() << endl;

    // 冗长的类型说明（如迭代器）
    vector<int> vec;
    auto iter = vec.begin();
    cout << "type of iter is " << typeid(iter).name() << endl;

    // 使用模板技术时，如果某个变量的类型依赖于模板参数，使用auto确定变量类型
    add(101, 1.1);
}
```

## 函数返回值占位符

在这种情况下，auto主要与decltype关键字配合使用，作为返回值类型后置时的占位符。此时，关键字不表示自动类型检测，仅仅是表示后置返回值的语法的一部分。

示例如下：

```
template<class T, class U>
auto add(T t, U u) -> decltype(t + u)
{
    return t + u;
}
```

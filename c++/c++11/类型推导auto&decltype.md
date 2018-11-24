# 类型推导auto&decltype

# auto

auto 在很早以前就已经进入了 C++，但是他始终作为一个存储类型的指示符存在，与 register 并存。在传统 C++ 中，如果一个变量没有声明为 register 变量，将自动被视为一个 auto 变量。而随着 register 被弃用，对 auto 的语义变更也就非常自然了。

使用 auto 进行类型推导的一个最为常见而且显著的例子就是迭代器。

```
// 由于 cbegin() 将返回 vector<int>::const_iterator
// 所以 itr 也应该是 vector<int>::const_iterator 类型
for(auto itr = vec.cbegin(); itr != vec.cend(); ++itr);
```

```
auto i = 5;             // i 被推导为 int
auto arr = new auto(10) // arr 被推导为 int *
```

注意：auto 不能用于函数传参，因此下面的做法是无法通过编译的（考虑重载的问题，我们应该使用模板）：

```
int add(auto x, auto y);
```

此外，auto 还不能用于推导数组类型：

```
auto i = 5;

int arr[10] = {0};
auto auto_arr = arr;
auto auto_arr2[10] = arr;
```

# decltype

decltype 关键字是为了解决 auto 关键字只能对变量进行类型推导的缺陷而出现的。它的用法和 sizeof 很相似：

decltype(表达式)

在此过程中，编译器分析表达式并得到它的类型，却不实际计算表达式的值。
有时候，我们可能需要计算某个表达式的类型，例如：

auto x = 1;
auto y = 2;
decltype(x+y) z;

# 拖尾返回类型、auto 与 decltype 配合

你可能会思考，auto 能不能用于推导函数的返回类型。考虑这样一个例子加法函数的例子，在传统 C++ 中我们必须这么写：

template<typename R, typename T, typename U>
R add(T x, U y) {
    return x+y
}

这样的代码其实变得很丑陋，因为程序员在使用这个模板函数的时候，必须明确指出返回类型。但事实上我们并不知道 add() 这个函数会做什么样的操作，获得一个什么样的返回类型。

在 C++11 中这个问题得到解决。虽然你可能马上回反应出来使用 decltype 推导 x+y 的类型，写出这样的代码：

decltype(x+y) add(T x, U y);

但事实上这样的写法并不能通过编译。这是因为在编译器读到 decltype(x+y) 时，x 和 y 尚未被定义。为了解决这个问题，C++11 还引入了一个叫做拖尾返回类型（trailing return type），利用 auto 关键字将返回类型后置：

template<typename T, typename U>
auto add(T x, U y) -> decltype(x+y) {
    return x+y;
}

从 C++14 开始是可以直接让普通函数具备返回值推导，因此下面的写法变得合法：

template<typename T, typename U>
auto add(T x, U y) {
    return x+y;
}

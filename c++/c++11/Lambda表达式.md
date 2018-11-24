# Lambda 表达式

Lambda 表达式，实际上就是提供了一个类似匿名函数的特性，而匿名函数则是在需要一个函数，但是又不想费力去命名一个函数的情况下去使用的。

Lambda 表达式的基本语法如下：

[ caputrue ] ( params ) opt -> ret { body; };

1) capture是捕获列表；
2) params是参数表；(选填)
3) opt是函数选项；可以填mutable,exception,attribute（选填）
mutable说明lambda表达式体内的代码可以修改被捕获的变量，并且可以访问被捕获的对象的non-const方法。
exception说明lambda表达式是否抛出异常以及何种异常。
attribute用来声明属性。
4) ret是返回值类型（拖尾返回类型）。(选填)
5) body是函数体。

捕获列表：lambda表达式的捕获列表精细控制了lambda表达式能够访问的外部变量，以及如何访问这些变量。

1) []不捕获任何变量。
2) [&]捕获外部作用域中所有变量，并作为引用在函数体中使用（按引用捕获）。
3) [=]捕获外部作用域中所有变量，并作为副本在函数体中使用(按值捕获)。注意值捕获的前提是变量可以拷贝，且被捕获的变量在 lambda 表达式被创建时拷贝，而非调用时才拷贝。如果希望lambda表达式在调用时能即时访问外部变量，我们应当使用引用方式捕获。

```
int a = 0;
auto f = [=] { return a; };

a+=1;

cout << f() << endl;       //输出0

int a = 0;
auto f = [&a] { return a; };

a+=1;

cout << f() <<endl;       //输出1
```
4) [=,&foo]按值捕获外部作用域中所有变量，并按引用捕获foo变量。
5) [bar]按值捕获bar变量，同时不捕获其他变量。
6) [this]捕获当前类中的this指针，让lambda表达式拥有和当前类成员函数同样的访问权限。如果已经使用了&或者=，就默认添加此选项。捕获this的目的是可以在lamda中使用当前类的成员函数和成员变量。

```
class A
{
 public:
     int i_ = 0;

     void func(int x,int y){
         auto x1 = [] { return i_; };                   //error,没有捕获外部变量
         auto x2 = [=] { return i_ + x + y; };          //OK
         auto x3 = [&] { return i_ + x + y; };        //OK
         auto x4 = [this] { return i_; };               //OK
         auto x5 = [this] { return i_ + x + y; };       //error,没有捕获x,y
         auto x6 = [this, x, y] { return i_ + x + y; };     //OK
         auto x7 = [this] { return i_++; };             //OK
};

int a=0 , b=1;
auto f1 = [] { return a; };                         //error,没有捕获外部变量
auto f2 = [&] { return a++ };                      //OK
auto f3 = [=] { return a; };                        //OK
auto f4 = [=] {return a++; };                       //error,a是以复制方式捕获的，无法修改
auto f5 = [a] { return a+b; };                      //error,没有捕获变量b
auto f6 = [a, &b] { return a + (b++); };                //OK
auto f7 = [=, &b] { return a + (b++); };                //OK
```

注意f4，虽然按值捕获的变量值均复制一份存储在lambda表达式变量中，修改他们也并不会真正影响到外部，但我们却仍然无法修改它们。如果希望去修改按值捕获的外部变量，需要显示指明lambda表达式为mutable。被mutable修饰的lambda表达式就算没有参数也要写明参数列表。

原因：lambda表达式可以说是就地定义仿函数闭包的“语法糖”。它的捕获列表捕获住的任何外部变量，最终会变为闭包类型的成员变量。按照C++标准，lambda表达式的operator()默认是const的，一个const成员函数是无法修改成员变量的值的。而mutable的作用，就在于取消operator()的const。

```
int a = 0;
auto f1 = [=] { return a++; };                //error
auto f2 = [=] () mutable { return a++; };       //OK
```

lambda表达式的大致原理：每当你定义一个lambda表达式后，编译器会自动生成一个匿名类（这个类重载了()运算符），我们称为闭包类型（closure type）。那么在运行时，这个lambda表达式就会返回一个匿名的闭包实例，是一个右值。所以，我们上面的lambda表达式的结果就是一个个闭包。对于复制传值捕捉方式，类中会相应添加对应类型的非静态数据成员。在运行时，会用复制的值初始化这些成员变量，从而生成闭包。对于引用捕获方式，无论是否标记mutable，都可以在lambda表达式中修改捕获的值。至于闭包类中是否有对应成员，C++标准中给出的答案是：不清楚的，与具体实现有关。

lambda表达式是不能被赋值的：

```
auto a = [] { cout << "A" << endl; };
auto b = [] { cout << "B" << endl; };

a = b;   // 非法，lambda无法赋值
auto c = a;   // 合法，生成一个副本
```

闭包类型禁用了赋值操作符，但是没有禁用复制构造函数，所以你仍然可以用一个lambda表达式去初始化另外一个lambda表达式而产生副本。

在多种捕获方式中，最好不要使用[=]和[&]默认捕获所有变量。

默认引用捕获所有变量，你有很大可能会出现悬挂引用（Dangling references），因为引用捕获不会延长引用的变量的生命周期：

```
std::function<int(int)> add_x(int x)
{
    return [&](int a) { return x + a; };
}
```

上面函数返回了一个lambda表达式，参数x仅是一个临时变量，函数add_x调用后就被销毁了，但是返回的lambda表达式却引用了该变量，当调用这个表达式时，引用的是一个垃圾值，会产生没有意义的结果。上面这种情况，使用默认传值方式可以避免悬挂引用问题。

但是采用默认值捕获所有变量仍然有风险，看下面的例子：

```
class Filter
{
public:
    Filter(int divisorVal):
        divisor{divisorVal}
    {}

    std::function<bool(int)> getFilter()
    {
        return [=](int value) {return value % divisor == 0; };
    }

private:
    int divisor;
};
```

这个类中有一个成员方法，可以返回一个lambda表达式，这个表达式使用了类的数据成员divisor。而且采用默认值方式捕捉所有变量。你可能认为这个lambda表达式也捕捉了divisor的一份副本，但是实际上并没有。因为数据成员divisor对lambda表达式并不可见，你可以用下面的代码验证：

```
// 类的方法，下面无法编译，因为divisor并不在lambda捕捉的范围
std::function<bool(int)> getFilter()
{
    return [divisor](int value) {return value % divisor == 0; };
}
```

原代码中，lambda表达式实际上捕捉的是this指针的副本，所以原来的代码等价于：

```
std::function<bool(int)> getFilter()
{
    return [this](int value) {return value % this->divisor == 0; };
}
```

管还是以值方式捕获，但是捕获的是指针，其实相当于以引用的方式捕获了当前类对象，所以lambda表达式的闭包与一个类对象绑定在一起了，这很危险，因为你仍然有可能在类对象析构后使用这个lambda表达式，那么类似“悬挂引用”的问题也会产生。所以，采用默认值捕捉所有变量仍然是不安全的，主要是由于指针变量的复制，实际上还是按引用传值。

lambda表达式可以赋值给对应类型的函数指针。但是使用函数指针并不是那么方便。所以STL定义在< functional >头文件提供了一个多态的函数对象封装std::function，其类似于函数指针。它可以绑定任何类函数对象，只要参数与返回类型相同。如下面的返回一个bool且接收两个int的函数包装器：

```
std::function<bool(int, int)> wrapper = [](int x, int y) { return x < y; };
```

lambda表达式一个更重要的应用是其可以用于函数的参数，通过这种方式可以实现回调函数。

最常用的是在STL算法中，比如你要统计一个数组中满足特定条件的元素数量，通过lambda表达式给出条件，传递给count_if函数：

```
int value = 3;
vector<int> v {1, 3, 5, 2, 6, 10};
int count = std::count_if(v.beigin(), v.end(), [value](int x) { return x > value; });
```

再比如你想生成斐波那契数列，然后保存在数组中，此时你可以使用generate函数，并辅助lambda表达式：

```
vector<int> v(10);
int a = 0;
int b = 1;
std::generate(v.begin(), v.end(), [&a, &b] { int value = b; b = b + a; a = value; return value; });
// 此时v {1, 1, 2, 3, 5, 8, 13, 21, 34, 55}
```

当需要遍历容器并对每个元素进行操作时：

```
std::vector<int> v = { 1, 2, 3, 4, 5, 6 };
int even_count = 0;
for_each(v.begin(), v.end(), [&even_count](int val){
    if(!(val & 1)){
        ++ even_count;
    }
});
std::cout << "The number of even is " << even_count << std::endl;
```

大部分STL算法，可以非常灵活地搭配lambda表达式来实现想要的效果。

# C++11智能指针原理、使用、实现

shared_ptr
unique_ptr
weak_ptr
auto_ptr（被 C++11 弃用）

## shared_ptr

多个智能指针可以共享同一个对象，对象的最末一个拥有着有责任销毁对象，并清理与该对象相关的所有资源。

支持定制型删除器（custom deleter），可防范 Cross-DLL 问题（对象在动态链接库（DLL）中被 new 创建，却在另一个 DLL 内被 delete 销毁）、自动解除互斥锁

### shared_ptr如何避免循环引用

shared_ptr是强引用，会计数，weak_ptr是弱引用，不会计数，

为了避免shared_ptr对象相互只有导致循环引用，通常做法是：

owner持有指向child的shared_ptr，child持有指向owner的weak_ptr。

## weak_ptr

weak_ptr 允许你共享但不拥有某对象，一旦最末一个拥有该对象的智能指针失去了所有权，任何 weak_ptr 都会自动成空（empty）。因此，在 default 和 copy 构造函数之外，weak_ptr 只提供 “接受一个 shared_ptr” 的构造函数。

可打破环状引用（cycles of references，两个其实已经没有被使用的对象彼此互指，使之看似还在 “被使用” 的状态）的问题

## unique_ptr

unique_ptr 是 C++11 才开始提供的类型，是一种在异常时可以帮助避免资源泄漏的智能指针。采用独占式拥有，意味着可以确保一个对象和其相应的资源同一时间只被一个 pointer 拥有。一旦拥有着被销毁或编程 empty，或开始拥有另一个对象，先前拥有的那个对象就会被销毁，其任何相应资源亦会被释放。

unique_ptr 用于取代 auto_ptr

## auto_ptr
被 c++11 弃用，原因是缺乏语言特性如 “针对构造和赋值” 的 std::move 语义，以及其他瑕疵。

auto_ptr 与 unique_ptr 比较
auto_ptr 可以赋值拷贝，复制拷贝后所有权转移；unqiue_ptr 无拷贝赋值语义，但实现了move 语义；
auto_ptr 对象不能管理数组（析构调用 delete），unique_ptr 可以管理数组（析构调用 delete[] ）；

## 1.智能指针的作用

使用普通指针，容易造成堆内存泄露（忘记释放），二次释放，程序发生异常时内存泄露等问题等，使用智能指针能更好的管理堆内存。

理解智能指针需要从下面三个层次：

(1)智能指针是利用了一种叫做RAII（资源获取即初始化）的技术对普通的指针进行封装，这使得智能指针实质是一个对象，行为表现的却像一个指针。

(2)智能指针的作用是防止忘记调用delete释放内存和程序异常的进入catch块忘记释放内存。另外指针的释放时机也是非常有考究的，多次释放同一个指针会造成程序崩溃，这些都可以通过智能指针来解决。

(3)智能指针还有一个作用是把值语义转换成引用语义。C++和Java有一处最大的区别在于语义不同，
在Java里面下列代码：
Animal a = new Animal();
Animal b = a;
你当然知道，这里其实只生成了一个对象，a和b仅仅是把持对象的引用而已。但在C++中不是这样，
Animal a;
Animal b = a;
这里却是就是生成了两个对象。

## 2.智能指针的使用

智能指针在C++11版本之后提供，包含在头文件<memory>中，shared_ptr、unique_ptr、weak_ptr

## 2.1 shared_ptr的使用

shared_ptr多个指针指向相同的对象。

shared_ptr使用引用计数，每一个shared_ptr的拷贝都指向相同的内存。每使用他一次，内部的引用计数加1，每析构一次，内部的引用计数减1，减为0时，自动删除所指向的堆内存。
shared_ptr内部的引用计数是线程安全的，但是对象的读取需要加锁。

初始化：

智能指针是个模板类，可以指定类型
传入指针通过构造函数初始化，也可以使用make_shared函数初始化。
不能将指针直接赋值给一个智能指针，一个是类，一个是指针。例如std::shared_ptr<int> p4 = new int(1);的写法是错误的

拷贝和赋值：

拷贝使得对象的引用计数增加1，赋值使得原对象引用计数减1，当计数为0时，自动释放内存。后来指向的对象引用计数加1，指向后来的对象。

get函数获取原始指针

注意不要用一个原始指针初始化多个shared_ptr，否则会造成二次释放同一内存

注意避免循环引用，shared_ptr的一个最大的陷阱是循环引用，循环，循环引用会导致堆内存无法正确释放，导致内存泄漏。循环引用在weak_ptr中介绍。

```
int a = 10;
std::shared_ptr<int> ptra = std::make_shared<int>(a);
std::shared_ptr<int> ptra2(ptra); //copy
std::cout << ptra.use_count() << std::endl; //对象的引用计数增加1


int *pb = &a;
//std::shared_ptr<int> ptrb = pb;  // 不要用一个原始指针初始化多个shared_ptr，否则会造成二次释放同一内存

int b = 20;
std::shared_ptr<int> ptrb = std::make_shared<int>(b);
ptra2 = ptrb; //assign
pb = ptrb.get(); //获取原始指针
```

## 2.2 unique_ptr的使用

unique_ptr“唯一”拥有其所指对象，同一时刻只能有一个unique_ptr指向给定对象（通过禁止拷贝语义、只有移动语义来实现）。
相比与原始指针unique_ptr用于其RAII的特性，使得在出现异常的情况下，动态资源能得到释放。
unique_ptr指针本身的生命周期：从unique_ptr指针创建时开始，直到离开作用域。离开作用域时，若其指向对象，则将其所指对象销毁(默认使用delete操作符，用户可指定其他操作)。
unique_ptr指针与其所指对象的关系：在智能指针生命周期内，可以改变智能指针所指对象，如创建智能指针时通过构造函数指定、通过reset方法重新指定、通过release方法释放所有权、通过移动语义转移所有权。

```
int main() {
    {
        std::unique_ptr<int> uptr(new int(10));  //绑定动态对象
        //std::unique_ptr<int> uptr2 = uptr;  //不能賦值
        //std::unique_ptr<int> uptr2(uptr);  //不能拷貝
        std::unique_ptr<int> uptr2 = std::move(uptr); //轉換所有權
        uptr2.release(); //释放所有权
    }
    //超過uptr的作用域，內存釋放
}
```

## 2.3 weak_ptr的使用

weak_ptr是为了配合shared_ptr而引入的一种智能指针，因为它不具有普通指针的行为，没有重载operator*和->,它的最大作用在于协助shared_ptr工作，像旁观者那样观测资源的使用情况。
weak_ptr可以从一个shared_ptr或者另一个weak_ptr对象构造，获得资源的观测权。但weak_ptr没有共享资源，它的构造不会引起指针引用计数的增加。使用weak_ptr的成员函数use_count()可以观测资源的引用计数，另一个成员函数expired()的功能等价于use_count()==0,但更快，表示被观测的资源(也就是shared_ptr的管理的资源)已经不复存在。
weak_ptr可以使用一个非常重要的成员函数lock()从被观测的shared_ptr获得一个可用的shared_ptr对象， 从而操作资源。但当expired()==true的时候，lock()函数将返回一个存储空指针的shared_ptr。

```
int main() {
    {
        std::shared_ptr<int> sh_ptr = std::make_shared<int>(10);
        std::cout << sh_ptr.use_count() << std::endl;

        std::weak_ptr<int> wp(sh_ptr);
        std::cout << wp.use_count() << std::endl;

        if(!wp.expired()){
            std::shared_ptr<int> sh_ptr2 = wp.lock(); //get another shared_ptr
            *sh_ptr = 100;
            std::cout << wp.use_count() << std::endl;
        }
    }
    //delete memory
}
```

## 2.4 循环引用

考虑一个简单的对象建模——家长与子女：a Parent has a Child, a Child knows his/her Parent。
如果使用原始指针作为成员，Child和Parent由谁释放？那么如何保证指针的有效性？如何防止出现空悬指针？这些问题是C++面向对象编程麻烦的问题，现在可以借助smart pointer把对象语义（pointer）转变为值（value）语义，shared_ptr轻松解决生命周期的问题，不必担心空悬指针。但是这个模型存在循环引用的问题，注意其中一个指针应该为weak_ptr。

原始指针的做法，容易出错

```
class Child;
class Parent;

class Parent {
private:
    Child* myChild;
public:
    void setChild(Child* ch) {
        this->myChild = ch;
    }

    void doSomething() {
        if (this->myChild) {

        }
    }

    ~Parent() {
        delete myChild;
    }
};

class Child {
private:
    Parent* myParent;
public:
    void setPartent(Parent* p) {
        this->myParent = p;
    }
    void doSomething() {
        if (this->myParent) {

        }
    }
    ~Child() {
        delete myParent;
    }
};

int main() {
    {
        Parent* p = new Parent;
        Child* c =  new Child;
        p->setChild(c);
        c->setPartent(p);
        delete c;  //only delete one
    }
    return 0;
}
```

循环引用内存泄露的问题

```
class Child;
class Parent;

class Parent {
private:
    std::shared_ptr<Child> ChildPtr;
public:
    void setChild(std::shared_ptr<Child> child) {
        this->ChildPtr = child;
    }

    void doSomething() {
        if (this->ChildPtr.use_count()) {

        }
    }

    ~Parent() {
    }
};

class Child {
private:
    std::shared_ptr<Parent> ParentPtr;
public:
    void setPartent(std::shared_ptr<Parent> parent) {
        this->ParentPtr = parent;
    }
    void doSomething() {
        if (this->ParentPtr.use_count()) {

        }
    }
    ~Child() {
    }
};

int main() {
    std::weak_ptr<Parent> wpp;
    std::weak_ptr<Child> wpc;
    {
        std::shared_ptr<Parent> p(new Parent);
        std::shared_ptr<Child> c(new Child);
        p->setChild(c);
        c->setPartent(p);
        wpp = p;
        wpc = c;
        std::cout << p.use_count() << std::endl; // 2
        std::cout << c.use_count() << std::endl; // 2
    }
    std::cout << wpp.use_count() << std::endl;  // 1
    std::cout << wpc.use_count() << std::endl;  // 1
    return 0;
}
```

正确的做法(一个用weak_ptr,一个用shared_ptr)

```
#include <iostream>
#include <memory>

class Child;
class Parent;

class Parent {
private:
    //std::shared_ptr<Child> ChildPtr;
    std::weak_ptr<Child> ChildPtr;
public:
    void setChild(std::shared_ptr<Child> child) {
        this->ChildPtr = child;
    }

    void doSomething() {
        //new shared_ptr
        if (this->ChildPtr.lock()) {

        }
    }

    ~Parent() {
    }
};

class Child {
private:
    std::shared_ptr<Parent> ParentPtr;
public:
    void setPartent(std::shared_ptr<Parent> parent) {
        this->ParentPtr = parent;
    }
    void doSomething() {
        if (this->ParentPtr.use_count()) {

        }
    }
    ~Child() {
    }
};

int main() {
    std::weak_ptr<Parent> wpp;
    std::weak_ptr<Child> wpc;
    {
        std::shared_ptr<Parent> p(new Parent);
        std::shared_ptr<Child> c(new Child);
        p->setChild(c);
        c->setPartent(p);
        wpp = p;
        wpc = c;
        std::cout << p.use_count() << std::endl; // 2
        std::cout << c.use_count() << std::endl; // 1
    }
    std::cout << wpp.use_count() << std::endl;  // 0
    std::cout << wpc.use_count() << std::endl;  // 0
    return 0;
}
```

# const关键字

(1)常变量：
const 类型说明符 变量名
const int x=20;
说明该变量不可以被改变

(2)常引用：
const 类型说明符 &引用名
const int &p=&x;
不能通过这个const引用来改变绑定对象的值

(3)常对象：
类名 const 对象名
const对象只能访问const成员函数
const对象的成员是不能修改的

(4)常成员函数：
类名::fun(形参) const
void f() const
{
    ......
}
1)const修饰的成员函数不能修改任何的成员变量(mutable修饰的变量除外)
2)const成员函数不能调用非const成员函数，因为非const成员函数可以会修改成员变量

(5)常指针：
const 类型说明符* 指针名 ，类型说明符* const 指针名
如果const位于*的左侧，则const就是用来修饰指针所指向的变量，即指针指向为常量；
如果const位于*的右侧，const就是修饰指针本身，即指针本身是常量。
const int *p=&x;//定义指向常量的指针p,可以改变指针的指向，但是不能通过指针来改变 x中的值
int * const p=&x;//定义常指针p，不可改变指针的指向（常指针），但可以通过指针来修改x中的值

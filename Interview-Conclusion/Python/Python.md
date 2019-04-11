# Python

## 1 GIL

GIL, 即全局解释器锁（global interpreter lock），每个线程在执行时候都需要先获取GIL，保证同一时刻只有一个线程可以执行代码，即同一时刻只有一个线程使用CPU，也就是说多线程并不是真正意义上的同时执行。

py在同一时刻只能跑一个线程，这样在跑多线程的情况下，只有当线程获取到全局解释器锁后才能运行，而全局解释器锁只有一个，因此即使在多核的情况下也只能发挥出单核的功能。

## 2 面向对象

### 2.1 __init__

__init__ 方法通常用在初始化一个类实例的时候

```
# -*- coding: utf-8 -*-

class Person(object):
    """Silly Person"""

    def __init__(self, name, age):
        self.name = name
        self.age = age

    def __str__(self):
        return '<Person: %s(%s)>' % (self.name, self.age)

if __name__ == '__main__':
    piglei = Person('piglei', 24)
    print piglei
```

### 2.2 __new__

__new__方法接受的参数虽然也是和__init__一样，但__init__是在类实例创建之后调用，而 __new__方法正是创建这个类实例的方法。

因为类每一次实例化后产生的过程都是通过__new__来控制的，所以通过重载__new__方法，我们 可以很简单的实现单例模式。

```
class Singleton(object):
    def __new__(cls):
        # 关键在于这，每一次实例化的时候，我们都只会返回这同一个instance对象
        if not hasattr(cls, 'instance'):
            cls.instance = super(Singleton, cls).__new__(cls)
        return cls.instance

obj1 = Singleton()
obj2 = Singleton()

obj1.attr1 = 'value1'
print obj1.attr1, obj2.attr1
print obj1 is obj2
```

### 2.3 metaclass

Python中的类也是对象。元类就是用来创建这些类（对象）的，元类就是类的类

```
MyClass = MetaClass()    #元类创建
MyObject = MyClass()     #类创建实例
实际上MyClass就是通过type()来创创建出MyClass类，它是type()类的一个实例；同时MyClass本身也是类，也可以创建出自己的实例，这里就是MyObject
```

Python中所有的东西，注意，我是指所有的东西——都是对象。这包括整数、字符串、函数以及类。它们全部都是对象，而且它们都是从一个类创建而来。

元类就是创建类这种对象的东西， type就是Python的内建元类，当然了，你也可以创建自己的元类。

#### 2.3.1 __metaclass__属性

写一个类的时候为其添加__metaclass__属性, 定义了__metaclass__就定义了这个类的元类。

在该类并定义的时候，它还没有在内存中生成，知道它被调用。Python做了如下的操作：

1）Foo中有__metaclass__这个属性吗？如果是，Python会在内存中通过__metaclass__创建一个名字为Foo的类对象（我说的是类对象，请紧跟我的思路）。
2）如果Python没有找到__metaclass__，它会继续在父类中寻找__metaclass__属性，并尝试做和前面同样的操作。
3）如果Python在任何父类中都找不到__metaclass__，它就会在模块层次中去寻找__metaclass__，并尝试做同样的操作。
4）如果还是找不到__metaclass__,Python就会用内置的type来创建这个类对象。

现在的问题就是，你可以在__metaclass__中放置些什么代码呢？

答案就是：可以创建一个类的东西。那么什么可以用来创建一个类呢？type，或者任何使用到type或者子类化type的东西都可以。

#### 2.3.2 自定义元类

元类的主要目的就是为了当创建类时能够自动地改变类。通常，你会为API做这样的事情，你希望可以创建符合当前上下文的类。

1、使用函数当做元类

```
# 元类会自动将你通常传给‘type’的参数作为自己的参数传入
def upper_attr(future_class_name, future_class_parents, future_class_attr):
    '''返回一个类对象，将属性都转为大写形式'''
    #选择所有不以'__'开头的属性
    attrs = ((name, value) for name, value in future_class_attr.items() if not name.startswith('__'))
    # 将它们转为大写形式
    uppercase_attr = dict((name.upper(), value) for name, value in attrs)
    #通过'type'来做类对象的创建
    return type(future_class_name, future_class_parents, uppercase_attr)#返回一个类

class Foo(object):
    __metaclass__ = upper_attr
    bar = 'bip'
```

2、使用class来当做元类

```
# 请记住，'type'实际上是一个类，就像'str'和'int'一样。所以，你可以从type继承
# __new__ 是在__init__之前被调用的特殊方法，__new__是用来创建对象并返回之的方法，__new_()是一个类方法
# 而__init__只是用来将传入的参数初始化给对象，它是在对象创建之后执行的方法。
# 你很少用到__new__，除非你希望能够控制对象的创建。这里，创建的对象是类，我们希望能够自定义它，所以我们这里改写__new__
# 如果你希望的话，你也可以在__init__中做些事情。还有一些高级的用法会涉及到改写__call__特殊方法，但是我们这里不用，下面我们可以单独的讨论这个使用

class UpperAttrMetaClass(type):
    def __new__(upperattr_metaclass, future_class_name, future_class_parents, future_class_attr):
        attrs = ((name, value) for name, value in future_class_attr.items() if not name.startswith('__'))
        uppercase_attr = dict((name.upper(), value) for name, value in attrs)
        return type(future_class_name, future_class_parents, uppercase_attr)#返回一个对象，但同时这个对象是一个类

```

### 2.4 __call__

__call__方法其实和类的创建过程和实例化没有多大关系了，定义了__call__方法才能被使用函数的方式执行。

# this关键字

this是一个引用，它指向自身的这个对象。

1.使用this调用本类中的成员变量（属性），成员方法

this.name = xxx;
this.print();

2.使用this调用构造方法

this(name);

3.使用this引用当前对象

Student s1 = this ;//当前的对象，就表示stu1

Student s2 = stu ;

if (s1 == s2)

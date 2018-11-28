# struct和typedef struct

C

```
// c
typedef struct Student {
    int age;
} S;
```

C++

```
// c
struct Student {
    int age;
};

typedef struct Student S;
```

此时 S 等价于 struct Student，但两个标识符名称空间不相同。

另外还可以定义与 struct Student 不冲突的 void Student() {}。

C++ 中

由于编译器定位符号的规则（搜索规则）改变，导致不同于C语言。

一、如果在类标识符空间定义了 struct Student {...};，使用 Student me; 时，编译器将搜索全局标识符表，Student 未找到，则在类标识符内搜索。

即表现为可以使用 Student 也可以使用 struct Student，如下：

```
// cpp
struct Student {
    int age;
};

void f( Student me );       // 正确，"struct" 关键字可省略
```

二、若定义了与 Student 同名函数之后，则 Student 只代表函数，不代表结构体，如下：

```
typedef struct Student {
    int age;
} S;

void Student() {}           // 正确，定义后 "Student" 只代表此函数

//void S() {}               // 错误，符号 "S" 已经被定义为一个 "struct Student" 的别名

int main() {
    Student();
    struct Student me;      // 或者 "S me";
    return 0;
}
```

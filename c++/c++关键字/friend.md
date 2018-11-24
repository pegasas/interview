# friend 友元类和友元函数

能访问私有成员
破坏封装性
友元关系不可传递
友元关系的单向性
友元声明的形式及数量不受限制

```
#include <iostream>
#include <cmath>
using namespace std;
//使用友元函数计算两点之间的距离
class Point{
public:
    Point(int xx = 0, int yy = 0) { X = xx; Y = yy;}
    int GetX() {return X;}
    int GetY() {return Y;}
    friend float fDist( Point &a, Point &b );
private:
    int X, Y;
};

float fDist(Point &p1, Point &p2){
    double x = double(p1.X - p2.X);//通过对象访问私有数据成员，而不是必须使用Getx()函数
    double y = double(p1.Y - p2.Y);
    return float(sqrt(x*x + y*y));
}
int main(){
    Point p1(1, 1), p2(4, 5);
    cout << "the distance is:";
    cout << fDist(p1, p2) << endl;//计算两点之间的距离
    return 0;
}  
```

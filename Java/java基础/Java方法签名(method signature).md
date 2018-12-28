# Java方法签名(method signature)

方法签名由方法名称和一个参数列表（方法的参数的顺序和类型）组成。

注意，方法签名不包括方法的返回类型。不包括返回值和访问修饰符。

常见的问题应用：重载和重写。

```
public class A{
	 protected int method (int a, int b) {
		 return 0;
	 }
 }
class B extends A{
	private int method(int a,long b){
		return 0;
	}
}
```

Here is an example of a typical method declaration:(这里是一个典型的方法声明)

public double calculateAnswer(double wingSpan, int numberOfEngines,
                              double length, double grossTons) {
    //do the calculation here
}
The signature of the method declared above is:（上面方法的签名是）

calculateAnswer(double, int, double, double)

# Java为什么匿名内部类参数引用需要用final进行修饰

事实上，除了匿名内部类内部，
方法和作用域内的内部类内部使用的外部变量也必须是 final 的。
原因大致总结一下：

简单解释就是：方法中的局部变量的生命周期很短，方法结束后变量就要被销毁，
加上final是为了延长变量的生命周期。
进一步解释：    内部类通常都含有回调，引用那个匿名内部类的函数执行完了就没了，
所以内部类中引用外面的局部变量需要是final的，这样在回调的时候才能找到那个变量，
而如果是外围类的成员变量就不需要是final的，因为内部类本身都会含有一个外围了的引用
（外围类.this），所以回调的时候一定可以访问到。

```
private Animator createAnimatorView(final View view, final int position) {
       MyAnimator animator = new MyAnimator();
       animator.addListener(new AnimatorListener() {
           @Override
           public void onAnimationEnd(Animator arg0) {
               Log.d(TAG, "position=" + position);
           }
       });
       return animator;
   }
```
匿名内部类回调里访问position的时候createAnimatorView()早就执行完了，
position如果不是final的，
回调的时候肯定就无法获取它的值了，因为局部变量在函数执行完了以后就被回收了。
所以java干脆把这样的变量设计成final，一旦初始化，就必须不能改变！
这样保证任何时候进行回调都能得到所需的值。

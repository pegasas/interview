# Spring mvc HandlerMapping 实现机制

概述
当DispatcherServlet接受到客户端的请求后，SpringMVC 通过 HandlerMapping 找到请求的Controller。
HandlerMapping 在这里起到路由的作用，负责找到请求的Controller。

Spring MVC 默认提供了4种 HandlerMapping的实现
org.springframework.web.servlet.handler.SimpleUrlHandlerMapping
通过配置请求路径和Controller映射建立关系，找到相应的Controller
org.springframework.web.servlet.mvc.support.ControllerClassNameHandlerMapping
通过 Controller 的类名找到请求的Controller。
org.springframework.web.servlet.handler.BeanNameUrlHandlerMapping
通过定义的 beanName 进行查找要请求的Controller
org.springframework.web.servlet.mvc.annotation.DefaultAnnotationHandlerMapping
通过注解 @RequestMapping("/userlist") 来查找对应的Controller。
HandlerMapping 的4种配置
<bean class="org.springframework.web.servlet.handler.SimpleUrlHandlerMapping">
    <property name="mappings">
        <props>
            <prop key="/userlist.htm">userController</prop>
        </props>
    </property>
</bean>

<bean class="org.springframework.web.servlet.mvc.support.ControllerClassNameHandlerMapping"/>

<bean class="org.springframework.web.servlet.handler.BeanNameUrlHandlerMapping"/>

<bean class="org.springframework.web.servlet.mvc.annotation.DefaultAnnotationHandlerMapping"/>

<bean id="userController" name="/users" class="cn.com.infcn.web.controller.UserController"></bean>
UserController
@Controller
public class UserController extends AbstractController {

    @Override
    @RequestMapping("/userlist")
    protected ModelAndView handleRequestInternal(HttpServletRequest request, HttpServletResponse response)
            throws Exception {

        List<User> userList = new ArrayList<User>();

        userList.add(new User("zhangsan", 18));
        userList.add(new User("list", 16));

        return new ModelAndView("userList", "users", userList);
    }
}
HandlerMapping 4种访问路径
SimpleUrlHandlerMapping
访问方式: http://ip:port/project/userlist.htm
ControllerClassNameHandlerMapping
访问方式: http://ip:port/project/user
注：类的首字母要小写
BeanNameUrlHandlerMapping
访问方式: http://ip:port/project/users
注：bean name属性必须要以“/”开头。
DefaultAnnotationHandlerMapping
访问方式: http://ip:port/project/userlist
注：@RequestMapping("/userlist")定义的路径
HandlerMapping 初始化原理
继续上一篇Spring mvc DispatchServlet 实现机制 初始化DispatchServlet的时候，执行了初始化HandlerMapping操作。


initHandlerMapping() 方法

判断detectAllHandlerMappings是否为true，如果为true，则加载当前系统中所有实现了HandlerMapping接口的bean。
如果为false，则加载bean名称为“handlermapping”的HandlerMapping实现类。
如果还没有找到HandlerMapping，则加载SpvingMVC 配置文件中，默认配置的HandlerMapping。
detectAllHandlerMappings 设置
detectAllHandlerMappings 默认为true，如果只想加载自己指定的HandlerMapping，请使用下面的方式指定




如果这样指定，则Spring MVC 只会加载这个HandlerMapping，而不会加载配置的其它的HandlerMapping。

SimpleUrlHandlerMapping
以SimpleUrlHandlerMapping 为例，简单分析下HandlerMapping



从SimpleUrlHandlerMapping 类结构中我们可以发现urlMap属性。这个urlMap中保存了xml中配置的映射关系，通过setMappings方法填充到urlMap中。


这个urlMap就充当了SpringMVC的路由功能。

每个HandlerMapping都会有一个这样的Map。

DispatcherServlet.doDispatch()
当用户请求时，真正的请求会执行到DispatcherServlet的doDispatch()方法。



通过getHandler() 方法去查找HandlerMapping中查找对应的Controller，并封装成HandlerExecutionChain。
如果找不到，则执行noHandlerFound() 方法。
getHandler() 方法

迭代查找所有的HandlerMapping，如果找到则直接返回。

noHandlerFound() 方法

如果找不到Controller 则后台抛出异常或响应给前台 404。

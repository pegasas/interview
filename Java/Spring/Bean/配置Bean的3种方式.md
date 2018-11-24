# 配置Bean的3种方式

Spring IoC容器完全由实际编写的配置元数据的格式解耦。有下面三个重要的方法把配置元数据提供给Spring容器：

(1)基于XML的配置文件

<bean id=“loginUserDao” class=“com.chinalife.dao.impl.LoginUserDaoImpl”
        lazy-init=“true” init-method=“myInit” destroy-method=“myDestroy”
        scope=“prototype”>
        ……
</bean>

在XML配置中，通过<bean> </bean>来定义Bean，通过id或name属性定义Bean的名称，如果未指定id和name属性，Spring则自动将全限定类名作为Bean的名称。通过<property>子元素或者p命名空间的动态属性为Bean注入值。还可以通过<bean>的init-method和destory-method属性指定Bean实现类的方法名来设置生命过程方法（最多指定一个初始化方法和销毁方法）。通过<bean>的scope指定Bean的作用范围。听过<bean>的lazy-init属性指定是否延迟初始化。

当Bean的实现类来源于第三方类库，比如DataSource、HibernateTemplate等，无法在类中标注注解信息，只能通过XML进行配置；而且命名空间的配置，比如aop、context等，也只能采用基于XML的配置。

(2)基于注解的配置

@Scope(“prototype”)
@Lazy(true)
@Component(“loginUserDao”)
public class LoginUserDao {
    ……
    // 用于设置初始化方法
    @PostConstruct
    public void myInit() {

    }

    // 用于设置销毁方法
    @PreDestroy
    public void myDestroy() {
    }
}

在Bean实现类中通过一些Annotation来标注Bean类：

·@Component：标注一个普通的Spring Bean类（可以指定Bean名称，未指定时默认为小写字母开头的类名）

·@Controller：标注一个控制器类

·@Service：标注一个业务逻辑类

·@Repository：标注一个DAO类

通过在成员变量或者方法入参处标注@Autowired按类型匹配注入，也可以使用@Qualifier按名称配置注入。通过在方法上标注@PostConstrut和PreDestroy注解指定的初始化方法和销毁方法（可以定义任意多个）。通过@Scope(“prototype”)指定Bean的作用范围。通过在类定义处标注@Lazy(true)指定Bean的延迟加载。

当Bean的实现类是当前项目开发的，可以直接在Java类中使用基于注解的配置，配置比较简单。


(3)基于Java的配置

@Configuration
public class Conf {
    @Scope(“prototype”)
    @Bean(“loginUserDao”)
    public LoginUserDao loginUserDao() {
        return new LoginUserDao();
    }
}

在标注了@Configuration的java类中，通过在类方法标注@Bean定义一个Bean。方法必须提供Bean的实例化逻辑。通过@Bean的name属性可以定义Bean的名称，未指定时默认名称为方法名。在方法处通过@Autowired使方法入参绑定Bean，然后在方法中通过代码进行注入；也可以调用配置类的@Bean方法进行注入。通过@Bean的initMethod或destroyMethod指定一个初始化或者销毁方法。通过Bean方法定义处标注@Scope指定Bean的作用范围。通过在Bean方法定义处标注@Lazy指定Bean的延迟初始化。

当实例化Bean的逻辑比较复杂时，则比较适合基于Java类配置的方式。

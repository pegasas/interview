# jdbc

JDBC	org.springframework.jdbc.core.JdbcTemplate

```
// JDBC模板依赖于连接池来获得数据的连接，所以必须先要构造连接池
DriverManagerDataSource dataSource = new DriverManagerDataSource();
dataSource.setDriverClassName("com.mysql.jdbc.Driver");
dataSource.setUrl("jdbc:mysql://localhost:3306/spring");
dataSource.setUsername("root");
dataSource.setPassword("123456");

// 创建JDBC模板
JdbcTemplate jdbcTemplate = new JdbcTemplate();
// 这里也可以使用构造方法
jdbcTemplate.setDataSource(dataSource);

// sql语句
String sql = "select count(*)  from user";
Long num = (long) jdbcTemplate.queryForObject(sql, Long.class);

System.out.println(num);
```

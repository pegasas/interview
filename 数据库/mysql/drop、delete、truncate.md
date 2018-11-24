# drop、delete、truncate

delete和truncate只删除表的数据不删除表的结构
速度,一般来说: drop> truncate >delete
delete语句是dml,这个操作会放到rollback segement中,事务提交之后才生效;
如果有相应的trigger,执行的时候将被触发. truncate,drop是ddl, 操作立即生效,原数据不放到rollback segment中,不能回滚. 操作不触发trigger.

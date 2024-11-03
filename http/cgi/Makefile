# 定义伪目标 all
.PHONY: all
all: test_cgi mysql_cgi  # 默认目标，构建 test_cgi 和 mysql_cgi

# 规则: 构建 test_cgi
test_cgi: test_cgi.cc  # test_cgi 依赖于 test_cgi.cc
	g++ -o $@ $^ -std=c++11  # 使用 g++ 编译 test_cgi

# 规则: 构建 mysql_cgi
mysql_cgi: mysql_cgi.cc  # mysql_cgi 依赖于 mysql_cgi.cc
	g++ -o $@ $^ -std=c++11 -I include -L lib -lmysqlclient -lpthread -ldl -static
	# 使用 g++ 编译 mysql_cgi, 包含 MySQL 客户端库和其他链接选项

# 定义伪目标 clean
.PHONY: clean
clean:  # 清理目标
	rm -f test_cgi mysql_cgi # 删除生成的可执行文件

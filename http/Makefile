# Makefile for HTTP Server

# 定义可执行文件的名称
bin = httpserver

# 指定使用的编译器
cc = g++

# 编译选项，包括 C++11 标准和 pthread 库的链接
LD_FLAGS = -std=c++11 -lpthread

# 获取当前工作目录的路径
curr = $(shell pwd)

# 源文件列表，包含所有必要的源文件
src = main.cc                 # 主程序

# 定义伪目标 ALL
.PHONY: ALL
ALL: $(bin) CGI                # 默认目标，构建 httpserver 和 CGI

# 构建可执行文件的规则
$(bin): $(src)                 # 当源文件更新时重新构建可执行文件
	$(cc) -o $@ $^ $(LD_FLAGS) # 编译命令，生成可执行文件

# 处理 CGI 的规则
CGI:
	cd $(curr)/cgi; \          # 进入 cgi 子目录
	make; \                    # 执行 make，构建 CGI 程序
	cd -;                      # 返回上层目录

# 定义伪目标 clean
.PHONY: clean
clean:                         # 清理构建生成的文件
	rm -f $(bin)               # 删除可执行文件 httpserver
	rm -rf output              # 删除输出目录
	cd $(curr)/cgi; \          # 进入 cgi 子目录
	make clean; \              # 执行 make clean，清理 CGI 相关文件
	cd -;                      # 返回上层目录

# 定义伪目标 output
.PHONY: output
output:                       # 生成输出文件
	mkdir -p output           # 创建 output 目录（如果不存在）
	cp $(bin) output          # 复制可执行文件到 output 目录
	cp -rf wwwroot output      # 复制 wwwroot 目录到 output
	# 复制 CGI 相关的文件到 output/wwwroot
	cp ./cgi/test_cgi output/wwwroot
	# cp ./cgi/shell_cgi.sh output/wwwroot
	# cp ./cgi/python_cgi.py output/wwwroot
	# cp ./cgi/mysql_cgi output/wwwroot

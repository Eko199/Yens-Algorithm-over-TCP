targets := bin/server.out bin/client.out
CXXFlags = -std=c++2a -Wall -Werror -Wunused

all: mkdir_bin $(targets)

mkdir_bin:
	mkdir -p bin

bin/%.out: src/%.c
	gcc $< -o $@
	chmod +x $@

bin/server.out: src/server.cpp src/yen.cpp
	g++ $(CXXFlags) $^ -o $@ -pthread -ltbb
	chmod +x $@

bin/%.out: src/%.cpp
	g++ $(CXXFlags) $< -o $@ -pthread -ltbb
	chmod +x $@

clean:
	rm -rf bin
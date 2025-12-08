targets := server.out client.out
CXXFlags = -std=c++2a -Wall -Werror -Wunused

all: $(targets)

%.out: %.c
	gcc $< -o $@
	chmod +x $@

%.o: %.cpp
	g++ $(CXXFlags) -c $< -o $@ -pthread -ltbb

server.out: server.o yen.o
	g++ $(CXXFlags) $^ -o $@ -pthread -ltbb
	chmod +x $@

%.out: %.o
	g++ $(CXXFlags) $< -o $@ -pthread -ltbb
	chmod +x $@

clean:
	rm -f *.o
	rm -f *.out
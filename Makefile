targets := yen.out server.out client.out
CXXFlags = -std=c++2a -Wall -Werror -Wunused

all: $(targets)

%.out: %.c
	gcc $< -o $@
	chmod +x $@

%.out: %.cpp
	g++ $(CXXFlags) $< -o $@ -pthread -ltbb
	chmod +x $@

clean:
	rm -f yen.out
	rm -f server.out
	rm -f client.out
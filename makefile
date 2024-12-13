all: manager feed

manager: manager.c
	gcc -o manager manager.c

feed: feed.c
	gcc -o feed feed.c

clean:
	rm -f manager feed
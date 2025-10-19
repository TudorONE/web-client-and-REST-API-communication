SRC = client.c helpers.c requests.c parson.c buffer.c
OUT = client

all: build

build:
	gcc -Wall -Wextra -g -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)

all: up

up:
	docker compose up -d

build:
	docker compose build

start:
	docker compose start

stop:
	docker compose stop

down:
	docker compose down

clean:
	docker compose rm

local_build:
	bash local_build.sh

local_clean:
	make -C back clean
	make -C front clean
	find . -type f -name '*.o' -delete

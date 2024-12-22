all: up clean

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
	docker compose rm -f -v

local_build:
	./scripts/build.sh

local_clean:
	./scripts/clean.sh

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

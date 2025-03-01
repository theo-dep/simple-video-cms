all: local-all docker-all

local-all: back front

back front:
	@$(MAKE) -C $@ all

analyze-all: analyze-back analyze-front

analyze-back:
	@$(MAKE) -C back analyze

analyze-front:
	@$(MAKE) -C front analyze

docker-all: up clean

docker-up:
	docker compose up -d

docker-build:
	docker compose build

docker-start:
	docker compose start

docker-stop:
	docker compose stop

docker-down:
	docker compose down

docker-clean:
	docker compose rm

clean::
	-@$(MAKE) -C back clean
	-@$(MAKE) -C front clean

distclean:: clean
	-@$(MAKE) -C back distclean
	-git submodule foreach 'git clean -ffdx'
	-git submodule foreach 'git reset --hard HEAD'

.PHONY: all local-all back front analyze-all analyze-back analyze-front \
	docker-all docker-up docker-build docker-start docker-stop docker-down docker-clean \
	clean distclean

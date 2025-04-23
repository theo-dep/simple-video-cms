all: local-all docker-all

local-all: back front

back front:
	@$(MAKE) -C $@ all

local-start-back:
	@$(MAKE) -C back start

local-start-front:
	@$(MAKE) -C front start

analyze-all: analyze-common analyze-back analyze-front

analyze-common:
	@$(MAKE) -C common analyze

analyze-back:
	@$(MAKE) -C back analyze

analyze-front:
	@$(MAKE) -C front analyze

docker-all: docker-up docker-clean

docker-up:
	docker compose up -d ;

docker-build:
	docker compose build ;

docker-start:
	docker compose start ;

docker-stop:
	docker compose stop ;

docker-down:
	docker compose down ;

docker-clean:
	docker compose rm ;

clean::
	-@$(MAKE) -C back clean
	-@$(MAKE) -C front clean

distclean:: clean
	-@$(MAKE) -C back distclean
	-git submodule foreach 'git clean -ffdx'
	-git submodule foreach 'git reset --hard HEAD'

.PHONY: all local-all back front analyze-all analyze-common analyze-back analyze-front local-start-back local-start-front \
	docker-all docker-up docker-build docker-start docker-stop docker-down docker-clean \
	clean distclean

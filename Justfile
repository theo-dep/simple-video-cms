start-prod: build-back build-front
	@./dist/back

start-dev: build-debug
	@./build/vcpkg/Debug/back

build-debug: configure
	@cmake --build --preset vcpkg-debug

build-back: configure
	@cmake --build --preset vcpkg-release --target install

configure:
	@cmake --preset vcpkg

cpplint: cpplint-configure
	@cmake --build build/lint --parallel 1

cpplint-configure:
	@cmake --preset lint

build-front:
	@npx rollup -c rollup.config.js

eslint:
	@npx eslint "front/**/*.js"

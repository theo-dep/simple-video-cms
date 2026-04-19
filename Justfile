start-back: build-back
	@./build/vcpkg/back/Debug/back

start-front: build-front
	@./build/vcpkg/front/Debug/front

build-back: configure
	@cmake --build --preset vcpkg-debug --target back

build-front: configure
	@cmake --build --preset vcpkg-debug --target front

build-debug: configure
	@cmake --build --preset vcpkg-debug

build: configure
	@cmake --build --preset vcpkg

configure:
	@cmake --preset vcpkg

cpplint: cpplint-configure
	@cmake --build build/lint --parallel 1

cpplint-configure:
	@cmake --preset lint

eslint:
	@npx eslint "front/**/*.js"

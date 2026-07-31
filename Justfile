start-prod: build-back build-front
	@./dist/back

start-dev: build-debug
	@./build/vcpkg/Debug/back

build-debug: configure
	@cmake --build --preset vcpkg-debug --target back

build-back: configure
	@cmake --build --preset vcpkg-release --target install

configure:
	@cmake --preset vcpkg

cpplint: cpplint-configure
	@cmake --build build/lint --parallel 1

cpplint-configure:
	@cmake --preset lint

build-front:
	@npx rollup -c front/rollup.config.js

eslint:
	@npx eslint "front/**/*.js"

format:
	@npx prettier --write .
	@find . -type d -name build -prune -iname '*.h' -o -iname '*.cpp' | xargs clang-format -i --verbose

launch-browser:
	@brave-browser --app=http://0.0.0.0:8080

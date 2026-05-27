import { nodeResolve } from '@rollup/plugin-node-resolve';
import commonjs from '@rollup/plugin-commonjs';

// Fixes incompatibility between Babel's inheritsLoose helper (uses .call())
// and video.js native ES6 classes in ESM builds.
// Applies to all subclasses in the file, not just YtStyle.
//
// Babel pattern:   _Parent.call(this, arg1, arg2, ...) || this;
// Fixed pattern:   Reflect.construct(_Parent, [arg1, arg2, ...], new.target);
//
// new.target is the constructor actually invoked with `new`, which is exactly
// what Reflect.construct needs as its third argument to wire up the prototype chain.
const fixNativeClassInheritance = {
  name: 'fix-native-class-inheritance',
  transform(code, id) {
    if (!id.includes('videojs-yt-style') || id.includes('?')) return null;

    return {
      code: code.replace(
        /(_\w+)\.call\(this(?:,\s*([\s\S]*?))?\)\s*\|\|\s*this;/g,
        (_, parent, args) => `Reflect.construct(${parent}, [${args ?? ''}], new.target);`
      ),
      map: null,
    };
  },
};

// a lot of video.js dependencies does not have a default export
// build them with rollup to fix import
export default [
  {
    input: 'video.js',
    output: {
      dir: 'build/',
      format: 'es',
    },
    plugins: [
      nodeResolve({
        browser: true,
      }),
      commonjs({
        requireReturnsDefault: 'preferred',
      }),
    ],
  },
  {
    input: 'videojs-yt-style',
    output: {
      dir: 'build/',
      format: 'es',
    },
    plugins: [
      nodeResolve({
        browser: true,
      }),
      commonjs(),
      fixNativeClassInheritance,
    ],
    external: ['video.js'],
  },
  {
    input: 'videojs-mobile-ui',
    output: {
      dir: 'build/',
      format: 'es',
    },
    plugins: [
      nodeResolve({
        browser: true,
      }),
      commonjs(),
    ],
    external: ['video.js'],
  },
];

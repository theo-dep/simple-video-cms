import { nodeResolve } from '@rollup/plugin-node-resolve';
import commonjs from '@rollup/plugin-commonjs';
import babel from '@rollup/plugin-babel';

// Transpile for macOS 10.13 (Safari 11+): keeps native classes, so the
// native-class inheritance fix below stays valid
export const babelTargets = { targets: 'safari 11' };

// core-js is already ES5-compatible: polyfill injection into its own modules
// creates import cycles that break the commonjs plugin's lazy wrappers
export const excludeCoreJs = [/node_modules[\\/]core-js[\\/]/];

export const legacyBabel = () =>
  babel({
    babelHelpers: 'bundled',
    compact: true, // évite la note "deoptimised the styling" sur les fichiers > 500 KB
    exclude: excludeCoreJs,
    presets: [['@babel/preset-env', { ...babelTargets, useBuiltIns: 'usage', corejs: 3 }]],
  });

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

// String.prototype.at is missed by useBuiltIns: 'usage' (Safari 15.4+).
// Rollup externalise silencieusement les imports non résolus : failOnUnresolvedImports
// les transforme en erreur (un renommage de module core-js ne peut plus passer inaperçu).
export const onwarn = (warning, warn) => {
  if (warning.code === 'UNRESOLVED_IMPORT') {
    throw new Error(`Import non résolu : ${warning.message}`);
  }
  warn(warning);
};

const explicitPolyfills = {
  name: 'explicit-core-js-polyfills',
  transform(code, id) {
    if (!id.includes('videojs-yt-style') || id.includes('?')) return null;

    return {
      code: `import 'core-js/modules/es.string.at-alternative';\n${code}`,
      map: null,
    };
  },
};

const videojsPlugins = [nodeResolve({ browser: true }), commonjs({ requireReturnsDefault: 'preferred' }), legacyBabel()];

// a lot of video.js dependencies does not have a default export
// build them with rollup to fix import
export const videojsEntries = [
  {
    input: 'video.js',
    plugins: videojsPlugins,
  },
  {
    input: 'videojs-yt-style',
    plugins: [...videojsPlugins, explicitPolyfills, fixNativeClassInheritance],
    external: ['video.js'],
  },
  {
    input: 'videojs-mobile-ui',
    plugins: videojsPlugins,
    external: ['video.js'],
  },
];

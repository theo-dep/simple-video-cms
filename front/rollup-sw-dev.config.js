import { nodeResolve } from '@rollup/plugin-node-resolve';
import { injectManifest } from 'rollup-plugin-workbox';
import { onwarn } from './rollup.shared.js';

export default [
  {
    input: 'front/sw.js',
    onwarn,
    output: { dir: 'build/', format: 'es' },
    plugins: [
      nodeResolve({
        browser: true,
        extensions: ['.js', '.mjs'],
      }),
      injectManifest(
        {
          swSrc: 'front/sw.js',
          swDest: 'build/sw.js',
          globDirectory: 'mandatory/did/not/exists',
        },
        { esbuild: { minify: false, target: 'es2017' } }
      ),
    ],
  },
];

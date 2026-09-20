import { nodeResolve } from '@rollup/plugin-node-resolve';
import { injectManifest } from 'rollup-plugin-workbox';

export default [
  {
    input: 'front/sw.js',
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
        { esbuild: { minify: false } }
      ),
    ],
  },
];

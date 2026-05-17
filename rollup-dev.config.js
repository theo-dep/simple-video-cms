import { nodeResolve } from '@rollup/plugin-node-resolve';
import commonjs from '@rollup/plugin-commonjs';

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
      commonjs({
        requireReturnsDefault: 'preferred',
      }),
    ],
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
      commonjs({
        requireReturnsDefault: 'preferred',
      }),
    ],
  },
];

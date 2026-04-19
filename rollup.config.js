import { nodeResolve } from '@rollup/plugin-node-resolve';
import commonjs from '@rollup/plugin-commonjs';
import terser from '@rollup/plugin-terser';
import { rollupPluginHTML as html } from '@web/rollup-plugin-html';
import minifyTemplateLiterals from 'rollup-plugin-minify-template-literals';
import css from 'rollup-plugin-import-css';

const terserOptions = {
  ecma: 2020,
  warnings: true,
  compress: { passes: 2 },
};

export default [
  // Main Bundle
  {
    input: 'front/index.html',
    output: {
      dir: 'dist',
      format: 'es',
      sourcemap: true,
    },
    preserveEntrySignatures: 'strict',
    plugins: [
      minifyTemplateLiterals({
        exclude: ['**/node_modules/**'],
      }),
      html({
        minify: true,
        transformHtml: (html) => html.replace(/<script type="importmap">[\s\S]*?<\/script>/, ''),
      }),
      nodeResolve({
        browser: true,
        extensions: ['.js', '.mjs'],
      }),
      commonjs({
        requireReturnsDefault: 'preferred',
      }),
      css({
        transform: true,
      }),
      terser({
        ...terserOptions,
        module: true,
      }),
    ],
  },

  // Service worker
  {
    input: 'front/videoserviceworker.js',
    output: {
      file: 'dist/videoserviceworker.js',
      format: 'iife',
    },
    plugins: [
      terser({
        ...terserOptions,
        module: false,
      }),
    ],
  },
];

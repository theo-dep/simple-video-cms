import { readFileSync, writeFileSync, unlinkSync, renameSync } from 'fs';
import { nodeResolve } from '@rollup/plugin-node-resolve';
import commonjs from '@rollup/plugin-commonjs';
import terser from '@rollup/plugin-terser';
import { rollupPluginHTML as html } from '@web/rollup-plugin-html';
import css from 'rollup-plugin-import-css';
import babel from '@rollup/plugin-babel';

const SRC_HTML_FILE = 'index.html';
const SRC_HTML = `front/${SRC_HTML_FILE}`;
const PROD_HTML_FILE = 'index-prod.html';
const PROD_HTML = `front/${PROD_HTML_FILE}`;
const DIST_DIR = 'dist';

// prepare index.html without absolute path for html plugin
{
  const src = readFileSync(SRC_HTML, 'utf-8');
  const prod = src.replace(/href="\/assets\//g, 'href="./assets/').replace(/src="\/index\.js"/g, 'src="./index.js"');
  writeFileSync(PROD_HTML, prod);
}

function deleteProdHtmlFile() {
  return {
    name: 'delete-prod-html-file',
    closeBundle() {
      unlinkSync(PROD_HTML);
      renameSync(`${DIST_DIR}/${PROD_HTML_FILE}`, `${DIST_DIR}/${SRC_HTML_FILE}`);
    },
  };
}

const terserOptions = {
  ecma: 2020,
  warnings: true,
  compress: { passes: 2 },
};

export default [
  // Main Bundle
  {
    input: PROD_HTML,
    output: {
      dir: DIST_DIR,
      format: 'es',
      sourcemap: true,
    },
    plugins: [
      html({
        minify: true,
        minifyCss: true,
        transformHtml: (html) => {
          return (
            html
              // remove importmap
              .replace(/<script type="importmap">[\s\S]*?<\/script>/, '')
              // restore absolute path from html plugin
              .replace(/href="assets\//g, 'href="/assets/')
              .replace(/src="\.\/index\.js"/g, 'src="/index.js"')
          );
        },
      }),
      deleteProdHtmlFile(),
      babel({
        babelHelpers: 'bundled',
        exclude: ['**/node_modules/**'],
        plugins: [
          [
            'babel-plugin-htm',
            {
              tag: 'html',
              import: 'htm/preact',
            },
          ],
        ],
      }),
      nodeResolve({
        browser: true,
        extensions: ['.js', '.mjs'],
      }),
      commonjs({
        requireReturnsDefault: 'preferred',
      }),
      css({
        minify: true,
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
      file: `${DIST_DIR}/videoserviceworker.js`,
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

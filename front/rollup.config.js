import { readFileSync, writeFileSync, unlinkSync, renameSync, readdirSync, existsSync, copyFileSync, mkdirSync } from 'fs';
import { nodeResolve } from '@rollup/plugin-node-resolve';
import commonjs from '@rollup/plugin-commonjs';
import terser from '@rollup/plugin-terser';
import { rollupPluginHTML as html } from '@web/rollup-plugin-html';
import babel from '@rollup/plugin-babel';
import replace from '@rollup/plugin-replace';
import { injectManifest } from 'rollup-plugin-workbox';
import { videojsEntries } from './rollup.shared.js';

const SRC_HTML_FILE = 'index.html';
const SRC_HTML = `front/${SRC_HTML_FILE}`;
const PROD_HTML_FILE = 'index-prod.html';
const PROD_HTML = `front/${PROD_HTML_FILE}`;
const DIST_DIR = 'dist';

const BOOTSTRAP_ICONS_FILE = 'node_modules/bootstrap-icons/font/bootstrap-icons.css';
const FIXED_BOOTSTRAP_ICONS_FILE = 'node_modules/bootstrap-icons/font/bootstrap-icons-fixed.css';

// remove bootstrap-icons font url query string
{
  const bootstrapIconsContent = readFileSync(BOOTSTRAP_ICONS_FILE, 'utf-8');
  const fixedBootstrapIconsContent = bootstrapIconsContent.replace(/(\.\/fonts\/bootstrap-icons\.(woff2|woff))\?[^"']*/g, '$1');
  writeFileSync(FIXED_BOOTSTRAP_ICONS_FILE, fixedBootstrapIconsContent);
}

// prepare index.html without absolute path for html plugin
// and with bootstrap-icons fixed
{
  const src = readFileSync(SRC_HTML, 'utf-8');
  const prod = src
    .replace(/href="\/assets\//g, 'href="assets/')
    .replace(/href="\/manifest\.json"/g, 'href="manifest.json"')
    .replace(/src="\/index\.js"/g, 'src="index.js"')
    .replace(BOOTSTRAP_ICONS_FILE, FIXED_BOOTSTRAP_ICONS_FILE);
  writeFileSync(PROD_HTML, prod);
}

// copy manifest files to dist
{
  mkdirSync(`${DIST_DIR}/assets/icons`, { recursive: true });
  copyFileSync('front/manifest.json', `${DIST_DIR}/manifest.json`);
  const icons = readdirSync('front/assets/icons').filter((f) => !f.startsWith('apple-touch'));
  for (const icon of icons) {
    copyFileSync(`front/assets/icons/${icon}`, `${DIST_DIR}/assets/icons/${icon}`);
  }
}

function deleteProdHtmlFile() {
  return {
    name: 'delete-prod-html-file',
    closeBundle() {
      unlinkSync(FIXED_BOOTSTRAP_ICONS_FILE);
      unlinkSync(PROD_HTML);

      const distProd = `${DIST_DIR}/${PROD_HTML_FILE}`;
      if (existsSync(distProd)) {
        // build ends successfully
        renameSync(distProd, `${DIST_DIR}/${SRC_HTML_FILE}`);
      }
    },
  };
}

const terserOptions = {
  ecma: 2020,
  warnings: true,
  compress: { passes: 2 },
};

// external video.js and its plugins are hashed
function hashPathsPlugin() {
  return {
    name: 'hash-paths',
    outputOptions(options) {
      if (!existsSync(DIST_DIR)) {
        return options;
      }

      options.paths = options.paths || {};
      const fileMappings = {
        'video.js': 'video.es-',
        'videojs-yt-style': 'videojs-yt-style-',
        'videojs-mobile-ui': 'videojs-mobile-ui-',
      };
      const files = readdirSync(DIST_DIR).filter((f) => f.endsWith('.js'));
      for (const [key, prefix] of Object.entries(fileMappings)) {
        const matchingFile = files.find((file) => file.startsWith(prefix));
        if (matchingFile) {
          options.paths[key] = `./${matchingFile}`;
        }
      }
      return options;
    },
  };
}

export default [
  ...videojsEntries.map((entry) => ({
    ...entry,
    plugins: [...entry.plugins, hashPathsPlugin(), terser({ ...terserOptions, module: true })],
    output: {
      dir: `${DIST_DIR}`,
      format: 'es',
      entryFileNames: '[name]-[hash].js',
    },
  })),

  // Main Bundle
  {
    input: PROD_HTML,
    output: {
      dir: DIST_DIR,
      format: 'es',
      sourcemap: true,
      assetFileNames: '[name]-[hash][extname]',
      entryFileNames: '[name]-[hash].js',
      chunkFileNames: '[name]-[hash].js',
    },
    external: ['video.js', 'videojs-yt-style', 'videojs-mobile-ui'],
    plugins: [
      hashPathsPlugin(),
      html({
        minify: true,
        minifyCss: true,
        externalAssets: ['manifest.json'],
        transformHtml: (html) => {
          return (
            html
              // remove importmap
              .replace(/<script type="importmap">[\s\S]*?<\/script>/, '')
              // restore absolute path from html plugin
              .replace(/href="(?:\.\/)?(\w)/g, 'href="/$1')
              .replace(/src="(?:\.\/)?(\w)/g, 'src="/$1')
          );
        },
      }),
      deleteProdHtmlFile(),
      replace({
        values: { __BUILD_ENV__: JSON.stringify('production') },
        preventAssignment: true,
        objectGuards: true,
      }),
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
      commonjs(),
      terser({
        ...terserOptions,
        module: true,
      }),
      // Workbox plugin: must be last, after asset-emitting plugins
      injectManifest({
        swSrc: 'front/sw.js',
        swDest: `${DIST_DIR}/sw.js`,
        globDirectory: DIST_DIR,
        globPatterns: ['**/*.{js,css,svg,png,woff,woff2}'],
        globIgnores: ['sw.js', 'assets/**'],
        dontCacheBustURLsMatching: /-[a-zA-Z0-9_-]{8,}\./,
      }),
    ],
  },
];

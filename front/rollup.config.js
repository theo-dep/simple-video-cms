import { readFileSync, writeFileSync, unlinkSync, renameSync, readdirSync, existsSync } from 'fs';
import { createHash } from 'crypto';
import { nodeResolve } from '@rollup/plugin-node-resolve';
import commonjs from '@rollup/plugin-commonjs';
import terser from '@rollup/plugin-terser';
import { rollupPluginHTML as html } from '@web/rollup-plugin-html';
import css from 'rollup-plugin-import-css';
import babel from '@rollup/plugin-babel';
import { videojsEntries } from './rollup.shared.js';

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

const withTerser = (entry) => ({
  ...entry,
  plugins: [...entry.plugins, terser({ ...terserOptions, module: true })],
});

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

const withHashPaths = (entry) => ({
  ...entry,
  plugins: [...entry.plugins, hashPathsPlugin()],
});

const withCollectAssetUrls = (entry) => ({
  ...entry,
  plugins: [...entry.plugins, collectAssetUrlsPlugin()],
});

const collectedAssetUrls = new Set();

function collectAssetUrlsPlugin() {
  return {
    name: 'collect-asset-urls',
    generateBundle(_outputOptions, bundle) {
      for (const item of Object.values(bundle)) {
        if (item.type !== 'asset' && item.type !== 'chunk') {
          continue;
        }

        const fileName = item.fileName;
        if (
          fileName === 'sw.js' ||
          fileName === PROD_HTML_FILE ||
          fileName === SRC_HTML_FILE ||
          fileName.endsWith('.map') ||
          fileName.endsWith('.html')
        ) {
          continue;
        }

        collectedAssetUrls.add(`/${fileName}`);
      }
    },
  };
}

function buildServiceWorkerPlugin() {
  return {
    name: 'build-service-worker',
    transform(code, _id, _options) {
      const assetUrls = [...collectedAssetUrls].sort();
      const assetsVersion = createHash('sha256').update(assetUrls.join('\n')).digest('hex').slice(0, 12);
      const swTemplate = code;

      if (!swTemplate.includes('__ASSETS_MANIFEST__')) {
        throw new Error('Missing service worker placeholder: __ASSETS_MANIFEST__');
      }

      if (!swTemplate.includes("'__ASSETS_CACHE_VERSION__'")) {
        throw new Error("Missing service worker placeholder: '__ASSETS_CACHE_VERSION__'");
      }

      const swBuilt = swTemplate
        .replaceAll('__ASSETS_MANIFEST__', JSON.stringify(assetUrls))
        .replace("'__ASSETS_CACHE_VERSION__'", `'${assetsVersion}'`); // keep replace first for check dev/prod

      return {
        code: swBuilt,
        map: null,
      };
    },
  };
}

export default [
  ...videojsEntries.map((entry) => ({
    ...withCollectAssetUrls(withHashPaths(withTerser(entry))),
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
        transformHtml: (html) => {
          return (
            html
              // remove importmap
              .replace(/<script type="importmap">[\s\S]*?<\/script>/, '')
              // restore absolute path from html plugin
              .replace(/href="(\w)/g, 'href="/$1')
              .replace(/src="(\w)/g, 'src="/$1')
              .replace(/src="\.\/index-(.*)\.js"/g, 'src="/index-$1.js"')
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
      commonjs(),
      css({
        minify: true,
      }),
      terser({
        ...terserOptions,
        module: true,
      }),
      collectAssetUrlsPlugin(),
    ],
  },

  // Service worker
  {
    input: 'front/sw.js',
    output: {
      file: `${DIST_DIR}/sw.js`,
      format: 'iife',
    },
    plugins: [
      buildServiceWorkerPlugin(),
      terser({
        ...terserOptions,
        module: false,
      }),
    ],
  },
];

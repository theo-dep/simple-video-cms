import js from '@eslint/js';
import globals from 'globals';
import css from '@eslint/css';
import { defineConfig } from 'eslint/config';
import eslintConfigPrettier from 'eslint-config-prettier/flat';
import { configs as litConfigs } from 'eslint-plugin-lit';
import { configs as wcConfigs } from 'eslint-plugin-wc';

export default defineConfig([
  js.configs.recommended,

  litConfigs['flat/recommended'],
  wcConfigs['flat/recommended'],

  {
    files: ['front/**/*.{js,mjs,cjs}'],
    languageOptions: {
      globals: globals.browser,
    },
  },

  {
    files: ['front/**/*.css'],
    plugins: { css },
    language: 'css/css',
    ...css.configs.recommended,
  },

  eslintConfigPrettier,
]);

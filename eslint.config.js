import js from '@eslint/js';
import globals from 'globals';
import css from '@eslint/css';
import { defineConfig } from 'eslint/config';
import eslintConfigPrettier from 'eslint-config-prettier/flat';

export default defineConfig([
  js.configs.recommended,

  {
    files: ['front/**/*.{js,mjs,cjs}'],
    languageOptions: {
      globals: globals.browser,
    },
    rules: {
      'no-unused-vars': [
        'error',
        {
          // ignore parameter begins by _
          argsIgnorePattern: '^_',
          caughtErrorsIgnorePattern: '^_',
        },
      ],
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

import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    environment: 'jsdom',
    include: ['front/**/*.test.js'],
    setupFiles: ['front/tests/setup.js'],
  },
});

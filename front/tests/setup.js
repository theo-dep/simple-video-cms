import { afterEach } from 'vitest';
import { cleanup } from '@testing-library/preact';
import '@testing-library/jest-dom/vitest';

if (!window.__ENV__) {
  window.__ENV__ = {
    websiteName: 'Simple Video CMS',
  };
}

afterEach(() => {
  cleanup();
});

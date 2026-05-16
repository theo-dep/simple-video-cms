import { signal } from '@preact/signals';

export const websiteName = signal(window.__ENV__.websiteName ?? 'Simple Video CMS');

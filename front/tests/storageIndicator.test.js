import { render, screen, waitFor } from '@testing-library/preact';
import { h } from 'preact';
import { describe, expect, it, vi } from 'vitest';

import { StorageIndicator } from '../component/StorageIndicator.js';
import { cache } from '../store/cache.js';
import { swReady } from '../store/wb.js';

describe('StorageIndicator', () => {
  it('re-renders when storageInfo changes', async () => {
    vi.spyOn(console, 'error').mockImplementation(() => {});
    vi.spyOn(console, 'warn').mockImplementation(() => {});

    swReady.value = false;
    cache.storageInfo.value = { quota: 1000, usage: 100, available: 900, percentageUsed: 10 };

    render(h(StorageIndicator));
    expect(screen.getByText('100 bytes')).toBeInTheDocument();

    cache.storageInfo.value = { quota: 1000, usage: 500, available: 500, percentageUsed: 50 };

    await waitFor(() => expect(screen.getByText('500 bytes')).toBeInTheDocument());
  });
});

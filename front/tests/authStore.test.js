import { waitFor } from '@testing-library/preact';
import { beforeEach, describe, expect, it, vi } from 'vitest';

const refreshMock = vi.fn();

vi.mock('../api.js', () => ({
  api: {
    refresh: (...args) => refreshMock(...args),
  },
}));

import { refreshRequested, user } from '../store/auth.js';
import { videoIdRedirected } from '../store/redirect.js';

describe('auth store', () => {
  beforeEach(() => {
    refreshMock.mockReset();
    user.id.value = null;
    user.name.value = '';
    user.isLogged.value = false;
    user.isAdmin.value = false;
    user.videos.value = [];
    videoIdRedirected.value = '';
  });

  it('updates user state after refresh success', async () => {
    refreshMock.mockResolvedValue({
      json: {
        id: 7,
        name: 'alice',
        isAdmin: true,
        videos: [{ id: 1, title: 'Demo' }],
      },
    });

    refreshRequested.value = true;

    await waitFor(() => {
      expect(user.id.value).toBe(7);
      expect(user.name.value).toBe('alice');
      expect(user.isLogged.value).toBe(true);
      expect(user.isAdmin.value).toBe(true);
      expect(user.videos.value).toEqual([{ id: 1, title: 'Demo' }]);
    });
  });

  it('resets state when refresh fails', async () => {
    user.id.value = 7;
    user.name.value = 'alice';
    user.isLogged.value = true;
    user.isAdmin.value = true;
    user.videos.value = [{ id: 1, title: 'Demo' }];

    refreshMock.mockRejectedValue(new Error('network'));

    refreshRequested.value = true;

    await waitFor(() => {
      expect(user.id.value).toBeNull();
      expect(user.name.value).toBe('');
      expect(user.isLogged.value).toBe(false);
      expect(user.isAdmin.value).toBe(false);
      expect(user.videos.value).toEqual([]);
    });
  });

  it('keeps redirect signal writable for post-login flow', () => {
    videoIdRedirected.value = '15';
    expect(videoIdRedirected.value).toBe('15');

    videoIdRedirected.value = '';
    expect(videoIdRedirected.value).toBe('');
  });
});

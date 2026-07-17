import { afterEach, describe, expect, it, vi } from 'vitest';

import { api } from '../api.js';

afterEach(() => {
  vi.restoreAllMocks();
});

describe('api client', () => {
  it('refresh returns parsed json payload', async () => {
    const fetchMock = vi.spyOn(globalThis, 'fetch').mockResolvedValue({
      ok: true,
      status: 200,
      text: async () => '{"id":1,"name":"alice"}',
    });

    const result = await api.refresh();

    expect(fetchMock).toHaveBeenCalledWith('/api/refresh', {
      method: 'GET',
      credentials: 'same-origin',
    });
    expect(result.status).toBe(200);
    expect(result.json).toEqual({ id: 1, name: 'alice' });
  });

  it('throws with status and message when request fails', async () => {
    vi.spyOn(globalThis, 'fetch').mockResolvedValue({
      ok: false,
      status: 401,
      statusText: 'Unauthorized',
      text: async () => 'Invalid password',
    });

    await expect(api.login('alice', 'wrong')).rejects.toMatchObject({
      status: 401,
      message: 'Invalid password',
    });
  });

  it('returns null json when response body empty', async () => {
    vi.spyOn(globalThis, 'fetch').mockResolvedValue({
      ok: true,
      status: 204,
      text: async () => '',
    });

    const result = await api.logout();

    expect(result.status).toBe(204);
    expect(result.json).toBeNull();
  });

  it('uses status text when body reading fails', async () => {
    vi.spyOn(globalThis, 'fetch').mockResolvedValue({
      ok: false,
      status: 500,
      statusText: 'Server down',
      text: async () => {
        throw new Error('read failure');
      },
    });

    await expect(api.refresh()).rejects.toMatchObject({
      status: 500,
      message: 'Server down',
    });
  });
});

import { fireEvent, render, screen, waitFor } from '@testing-library/preact';
import { h } from 'preact';
import { describe, expect, it, vi, beforeEach } from 'vitest';

const routeMock = vi.fn();
const loginMock = vi.fn();

vi.mock('preact-iso', () => ({
  useLocation: () => ({ route: routeMock }),
}));

vi.mock('../hook/useTitle.js', () => ({
  useTitle: () => {},
}));

vi.mock('../api.js', () => ({
  api: {
    login: (...args) => loginMock(...args),
  },
}));

vi.mock('../store/auth.js', () => ({
  user: {
    id: { value: null },
    name: { value: '' },
    isLogged: { value: false },
    isAdmin: { value: false },
    videos: { value: [] },
  },
  refreshRequested: { value: false },
}));

vi.mock('../store/redirect.js', () => ({
  previousRoute: { value: '' },
}));

import Login from '../pages/Login.js';
import { previousRoute } from '../store/redirect.js';
import { refreshRequested } from '../store/auth.js';

describe('Login page', () => {
  beforeEach(() => {
    routeMock.mockReset();
    loginMock.mockReset();
    refreshRequested.value = false;
    previousRoute.value = '';
  });

  it('shows API error message on failed login', async () => {
    loginMock.mockRejectedValue(new Error('Invalid password'));

    render(h(Login, {}));

    fireEvent.input(screen.getByPlaceholderText('username'), { target: { value: 'alice' } });
    fireEvent.input(screen.getByPlaceholderText('password'), { target: { value: 'wrong' } });
    fireEvent.click(screen.getByRole('button', { name: 'Login' }));

    await screen.findByText('Invalid password');
    expect(routeMock).not.toHaveBeenCalled();
  });

  it('redirects first connection users to reset password page', async () => {
    loginMock.mockResolvedValue({ status: 204 });

    render(h(Login, {}));

    fireEvent.input(screen.getByPlaceholderText('username'), { target: { value: 'alice' } });
    fireEvent.input(screen.getByPlaceholderText('password'), { target: { value: 'secret' } });
    fireEvent.click(screen.getByRole('button', { name: 'Login' }));

    await waitFor(() => {
      expect(routeMock).toHaveBeenCalledWith('/reset-password/alice');
    });
  });

  it('redirects to video and clears redirect store on successful login', async () => {
    loginMock.mockResolvedValue({ status: 200 });
    previousRoute.value = '/video/99';

    render(h(Login, {}));

    fireEvent.input(screen.getByPlaceholderText('username'), { target: { value: 'alice' } });
    fireEvent.input(screen.getByPlaceholderText('password'), { target: { value: 'secret' } });
    fireEvent.click(screen.getByRole('button', { name: 'Login' }));

    await waitFor(() => {
      expect(refreshRequested.value).toBe(true);
    });
    refreshRequested.value = false; // need a refresh before to route
    await waitFor(() => {
      expect(routeMock).toHaveBeenCalledWith('/video/99');
    });
    expect(previousRoute.value).toBe('');
  });
});

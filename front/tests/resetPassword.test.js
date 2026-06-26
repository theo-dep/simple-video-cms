import { fireEvent, render, screen, waitFor } from '@testing-library/preact';
import { h } from 'preact';
import { beforeEach, describe, expect, it, vi } from 'vitest';

const routeMock = vi.fn();
const addPasswordMock = vi.fn();

vi.mock('preact-iso', () => ({
  useLocation: () => ({ route: routeMock }),
}));

vi.mock('../hook/useTitle.js', () => ({
  useTitle: () => {},
}));

vi.mock('../api.js', () => ({
  api: {
    addPassword: (...args) => addPasswordMock(...args),
  },
}));

import ResetPassword from '../pages/ResetPassword.js';

describe('ResetPassword page', () => {
  beforeEach(() => {
    routeMock.mockReset();
    addPasswordMock.mockReset();
  });

  it('shows API error on failed password creation', async () => {
    addPasswordMock.mockRejectedValue(new Error('Passwords do not match'));

    render(h(ResetPassword, { username: 'alice' }));

    fireEvent.input(screen.getByPlaceholderText('password'), { target: { value: 'x' } });
    fireEvent.input(screen.getByPlaceholderText('confirm password'), { target: { value: 'y' } });
    fireEvent.click(screen.getByRole('button', { name: 'Create' }));

    await screen.findByText('Passwords do not match');
    expect(routeMock).not.toHaveBeenCalled();
  });

  it('redirects to login when password is set', async () => {
    addPasswordMock.mockResolvedValue({ status: 200 });

    render(h(ResetPassword, { username: 'alice' }));

    fireEvent.input(screen.getByPlaceholderText('password'), { target: { value: 'secret' } });
    fireEvent.input(screen.getByPlaceholderText('confirm password'), { target: { value: 'secret' } });
    fireEvent.click(screen.getByRole('button', { name: 'Create' }));

    await waitFor(() => {
      expect(addPasswordMock).toHaveBeenCalledWith('alice', 'secret', 'secret');
      expect(routeMock).toHaveBeenCalledWith('/login');
    });
  });
});

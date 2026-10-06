import { html } from 'htm/preact';
import { useEffect } from 'preact/hooks';
import { lazy, useLocation } from 'preact-iso';
import { user } from '../store/auth.js';

function userGuarded(Inner, props) {
  const { route } = useLocation();

  useEffect(() => {
    if (!user.isLogged.value) route('/login');
  }, []);

  if (!user.isLogged.value) return null;

  return html`<${Inner} ...${props} />`;
}

export function userLazy(importFn) {
  return lazy(async () => {
    const { default: Inner } = await importFn();
    return {
      default: (props) => userGuarded(Inner, props),
    };
  });
}

import { html } from 'htm/preact';
import { useEffect } from 'preact/hooks';
import { lazy, useLocation } from 'preact-iso';
import { user } from '../store/auth.js';

function adminGuarded(Inner, props) {
  const { route } = useLocation();

  useEffect(() => {
    if (!user.isAdmin.value) route('/403');
  }, []);

  if (!user.isAdmin.value) return null;

  return html`<${Inner} ...${props} />`;
}

export function adminLazy(importFn) {
  return lazy(async () => {
    const { default: Inner } = await importFn();
    return {
      default: (props) => adminGuarded(Inner, props),
    };
  });
}

export function adminLazyNamed(importFn, name) {
  return lazy(async () => {
    const mod = await importFn();
    const Inner = mod[name];
    return {
      default: (props) => adminGuarded(Inner, props),
    };
  });
}

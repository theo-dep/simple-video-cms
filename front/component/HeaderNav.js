import { html } from 'htm/preact';
import { useEffect, useState } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { user } from '../store/auth.js';
import { websiteName } from '../store/env.js';

function NavLinks({ pages }) {
  return pages.map(
    (p) => html`
      <li class="menu-item" key=${p.href}>
        <a
          ...${p.href ? { href: p.href } : { onClick: () => p.onClick(), style: 'cursor: pointer;' }}
          class=${'menu-link' + (document.location.pathname === p.href ? ' menu-selected' : '')}
        >
          ${p.label}
        </a>
      </li>
    `
  );
}

function useBurgerMenu() {
  const [isOpen, setIsOpen] = useState(false);

  useEffect(() => {
    if (!isOpen) return;

    const close = () => setIsOpen(false);
    window.addEventListener('resize', close);
    window.addEventListener('orientationchange', close);

    return () => {
      window.removeEventListener('resize', close);
      window.removeEventListener('orientationchange', close);
    };
  }, [isOpen]);

  return [isOpen, setIsOpen];
}

function BurgerToggle({ isOpen, onToggle }) {
  return html`<a
    href="#"
    class=${'menu-burger-toggle' + (isOpen ? ' is-active' : '')}
    aria-label="Toggle menu"
    aria-expanded=${isOpen}
    onClick=${(e) => {
      e.preventDefault();
      onToggle();
    }}
  >
    <s class="bar"></s><s class="bar"></s><s class="bar"></s>
  </a> `;
}

export function UserNav() {
  const { route } = useLocation();
  const [isOpen, setIsOpen] = useBurgerMenu();

  function onLoginClicked() {
    route('/login');
  }

  const pages = user.isLogged.value
    ? [
        ...(user.isAdmin.value ? [{ href: '/admin', label: 'Admin' }] : []),
        { href: '/bookmarks', label: 'Bookmarks' },
        { href: '/user-account', label: 'Account' },
        { href: '/logout', label: 'Logout' },
      ]
    : [{ onClick: onLoginClicked, label: 'Login' }];

  return html`
    <div class="header">
      <div class="header-brand menu">
        <a href="/" class="menu-link menu-heading">${websiteName}</a>
        <${BurgerToggle} isOpen=${isOpen} onToggle=${() => setIsOpen((v) => !v)} />
      </div>
      <div class=${'header-menu-wrapper menu-burger-wrapper' + (isOpen ? ' menu-burger-open' : '')} onClick=${() => setIsOpen(false)}>
        <ul class="menu">
          <${NavLinks} pages=${pages} />
        </ul>
      </div>
    </div>
  `;
}

export function AdminNav() {
  const [isOpen, setIsOpen] = useBurgerMenu();

  const pages = [
    { href: '/admin', label: 'Admin' },
    { href: '/admin/video-list', label: 'Video List' },
    { href: '/admin/admin-list', label: 'Admin List' },
    { href: '/admin/user-list', label: 'User List' },
    { href: '/admin/group-list', label: 'Group List' },
    { href: '/logout', label: 'Logout' },
  ];

  return html`
    <div class="header">
      <div class="header-brand menu">
        <a href="/" class="menu-link menu-heading">${websiteName}</a>
        <${BurgerToggle} isOpen=${isOpen} onToggle=${() => setIsOpen((v) => !v)} />
      </div>
      <div class=${'header-menu-wrapper menu-burger-wrapper' + (isOpen ? ' menu-burger-open' : '')} onClick=${() => setIsOpen(false)}>
        <ul class="menu">
          <${NavLinks} pages=${pages} />
        </ul>
      </div>
    </div>
  `;
}

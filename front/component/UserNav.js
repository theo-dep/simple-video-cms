import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { user } from '../store/auth.js';
import { videoIdRedirected } from '../store/redirect.js';
import { websiteName } from '../store/env.js';

function NavLinks({ pages }) {
  return pages.map(
    (p) => html`
      <li class="pure-menu-item" key=${p.href}>
        <a
          ...${p.href ? { href: p.href } : { onClick: () => p.onClick(), style: 'cursor:pointer' }}
          class=${'menu-link pure-menu-link' + (document.location.pathname === p.href ? ' menu-selected pure-menu-selected' : '')}
        >
          ${p.label}
        </a>
      </li>
    `
  );
}

export function UserNav({ children, videoId = null }) {
  const { route } = useLocation();
  const hasChildren = !!children;

  function onLoginClicked() {
    if (videoId) {
      videoIdRedirected.value = videoId;
    }
    route('/login');
  }

  const pages = user.isLogged.value
    ? [
        ...(user.isAdmin.value ? [{ href: '/admin', label: 'Admin' }] : []),
        { href: '/user-account', label: 'Account' },
        { href: '/logout', label: 'Logout' },
      ]
    : [{ onClick: onLoginClicked, label: 'Login' }];

  return html`
    <div class=${'header' + (hasChildren ? ' pure-g' : '')}>
      <div class=${(hasChildren ? 'pure-u-1 pure-u-md-3-4 ' : '') + 'pure-menu pure-menu-horizontal'}>
        <a href="/" class="menu-link pure-menu-heading pure-menu-link">${websiteName}</a>
        <ul class="menu-list pure-menu-list">
          <${NavLinks} pages=${pages} />
        </ul>
      </div>
      ${hasChildren && html`<div class="pure-u-1 pure-u-md-1-4">${children}</div>`}
    </div>
  `;
}

export function AdminNav() {
  const pages = [
    { href: '/admin', label: 'Admin' },
    { href: '/admin/video-list', label: 'Video List' },
    { href: '/admin/admin-list', label: 'Admin List' },
    { href: '/admin/user-list', label: 'User List' },
    { href: '/admin/group-list', label: 'Group List' },
    { href: '/logout', label: 'Logout' },
  ];

  return html`
    <div class="header pure-g">
      <div class="pure-u-1 pure-u-md-1-5 pure-menu pure-menu-horizontal">
        <a href="/" class="menu-link pure-menu-heading pure-menu-link">${websiteName}</a>
      </div>
      <div class="pure-u-1 pure-u-md-4-5 menu-scrollable pure-menu pure-menu-horizontal pure-menu-scrollable">
        <ul class="menu-scrollable-list pure-menu-list">
          <${NavLinks} pages=${pages} />
        </ul>
      </div>
    </div>
  `;
}

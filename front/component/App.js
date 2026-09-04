import { html } from 'htm/preact';
import { Fragment } from 'preact';
import { useEffect } from 'preact/hooks';
import { LocationProvider, Router, lazy, useLocation } from 'preact-iso';
import { firstRefreshed, user } from '../store/auth.js';
import { previousRoute } from '../store/redirect.js';
import { swReady, postToServiceWorker } from '../store/sw.js';
import { ConfirmDialog } from './ConfirmDialog.js';
import { adminLazy, adminLazyNamed } from '../utils/lazy.js';
import { Redirect } from './Redirect.js';
import { OfflineWatcher } from './OfflineWatcher.js';

// useLocation only work inside LocationProvider component
function RedirectUpdater() {
  const { url } = useLocation();

  const EXCLUDED_ROUTES = ['/login', '/logout', '/reset-password', '/403'];

  function isExcluded(url) {
    const path = url.split('?')[0] ?? '';
    return EXCLUDED_ROUTES.some((r) => path === r || path.startsWith(r + '/'));
  }

  useEffect(() => {
    if (!isExcluded(url)) previousRoute.value = url;
  }, [url]);
}

export function App() {
  const isLoading = !firstRefreshed.value || !swReady.value;

  useEffect(() => {
    if (!isLoading) document.getElementById('boot-loader')?.remove();
  }, [isLoading]);

  useEffect(() => {
    if (!swReady.value) return;
    if (user.isLogged.value) {
      postToServiceWorker('enableVideoCaching');
    } else {
      postToServiceWorker('disableVideoCaching');
    }
  }, [swReady.value, user.isLogged.value]);

  useEffect(() => {
    /* global __BUILD_ENV__ */
    if (typeof __BUILD_ENV__ !== 'undefined' && __BUILD_ENV__ === 'production') {
      if ('serviceWorker' in navigator) {
        // Remove old service worker (v1)
        navigator.serviceWorker.getRegistrations().then((registrations) => {
          registrations.forEach((reg) => {
            if (reg.active?.scriptURL.includes('videoserviceworker.js')) {
              reg.unregister();
            }
          });
        });

        // Scope must be "/" for clients.claim() to work without a reload:
        // clients.claim() matches the registration scope against the client's
        // CREATION URL (the initial navigation that loaded the document), not
        // the current SPA route (pushState). Since this app is always loaded
        // from "/", only scope="/" can match and grant immediate control.
        navigator.serviceWorker.register('/sw.js', { scope: '/' }).then((_registration) => {
          swReady.value = true;
        });
      }
    } else {
      swReady.value = true;
    }
  }, []);

  return isLoading
    ? null
    : html`
        <${Fragment}>
          <${OfflineWatcher} />
          <${ConfirmDialog} />
          <${LocationProvider}>
            <${RedirectUpdater} />
            <${Router}>
              <${lazy(() => import('../pages/Home.js'))} path="/" />
              <${lazy(() => import('../pages/Bookmarks.js'))} path="/bookmarks" />
              <${lazy(() => import('../pages/Login.js'))} path="/login" />
              <${lazy(() => import('../pages/Logout.js'))} path="/logout" />
              <${lazy(() => import('../pages/ResetPassword.js'))} path="/reset-password" />
              <${lazy(() => import('../pages/ResetPassword.js'))} path="/reset-password/:username" />
              <${lazy(() => import('../pages/UserAccount.js'))} path="/user-account" />
              <${Redirect} path="/watch-video/:videoId" to="/video/:videoId" />
              <${lazy(() => import('../pages/WatchVideo.js'))} path="/video/:videoId" />
              <${lazy(() => import('../pages/Forbidden.js'))} path="/403" />
              <${lazy(() => import('../pages/NotFound.js'))} default />
              <${adminLazy(() => import('../pages/AdminDashboard.js'))} path="/admin" />
              <${adminLazy(() => import('../pages/AdminVideoList.js'))} path="/admin/video-list" />
              <${adminLazy(() => import('../pages/AdminNewVideo.js'))} path="/admin/new-video" />
              <${adminLazy(() => import('../pages/AdminVideoSettings.js'))} path="/admin/video-settings/:videoId" />
              <${adminLazy(() => import('../pages/AdminUserList.js'))} path="/admin/user-list" />
              <${adminLazyNamed(() => import('../pages/AdminNewUser.js'), 'AdminNewUser')} path="/admin/new-user" />
              <${adminLazyNamed(() => import('../pages/AdminNewUser.js'), 'AdminNewAdmin')} path="/admin/new-admin" />
              <${adminLazyNamed(() => import('../pages/AdminUserSettings.js'), 'AdminUserSettings')} path="/admin/user-settings/:userId" />
              <${adminLazyNamed(() => import('../pages/AdminUserSettings.js'), 'AdminAdminSettings')} path="/admin/admin-settings/:adminId" />
              <${adminLazy(() => import('../pages/AdminAdminList.js'))} path="/admin/admin-list" />
              <${adminLazy(() => import('../pages/AdminGroupList.js'))} path="/admin/group-list" />
              <${adminLazy(() => import('../pages/AdminNewGroup.js'))} path="/admin/new-group" />
              <${adminLazy(() => import('../pages/AdminGroupSettings.js'))} path="/admin/group-settings/:groupId" />
            <//>
          <//>
        <//>
      `;
}

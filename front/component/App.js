import { html } from 'htm/preact';
import { Fragment } from 'preact';
import { useEffect } from 'preact/hooks';
import { LocationProvider, Router, useLocation, lazy } from 'preact-iso';
import { refreshed } from '../store/auth.js';
import { navigate } from '../store/router.js';
import { swReady } from '../store/sw.js';
import { ConfirmDialog } from './ConfirmDialog.js';

export function App() {
  const { route } = useLocation();
  const isLoading = !refreshed.value || !swReady.value;

  useEffect(() => {
    if (!isLoading) document.getElementById('boot-loader')?.remove();
  }, [isLoading]);

  useEffect(() => {
    navigate.value = route;

    if ('serviceWorker' in navigator) {
      // Scope must be "/" for clients.claim() to work without a reload:
      // clients.claim() matches the registration scope against the client's
      // CREATION URL (the initial navigation that loaded the document), not
      // the current SPA route (pushState). Since this app is always loaded
      // from "/", only scope="/" can match and grant immediate control.
      navigator.serviceWorker.register('/sw.js', { scope: '/' }).then((_registration) => {
        swReady.value = true;
      });
    }
  }, []);

  return isLoading
    ? null
    : html`
        <${Fragment}>
          <${ConfirmDialog} />
          <${LocationProvider}>
            <${Router}>
              <${lazy(() => import('../pages/Home.js'))} path="/" />
              <${lazy(() => import('../pages/Login.js'))} path="/login" />
              <${lazy(() => import('../pages/Logout.js'))} path="/logout" />
              <${lazy(() => import('../pages/ResetPassword.js'))} path="/reset-password" />
              <${lazy(() => import('../pages/ResetPassword.js'))} path="/reset-password/:username" />
              <${lazy(() => import('../pages/UserAccount.js'))} path="/user-account" />
              <${lazy(() => import('../pages/WatchVideo.js'))} path="/watch-video/:videoId" />
              <${lazy(() => import('../pages/AdminDashboard.js'))} path="/admin" />
              <${lazy(() => import('../pages/AdminVideoList.js'))} path="/admin/video-list" />
              <${lazy(() => import('../pages/AdminNewVideo.js'))} path="/admin/new-video" />
              <${lazy(() => import('../pages/AdminVideoSettings.js'))} path="/admin/video-settings/:videoId" />
              <${lazy(() => import('../pages/AdminUserList.js'))} path="/admin/user-list" />
              <${lazy(() => import('../pages/AdminNewUser.js').then((m) => m.AdminNewUser))} path="/admin/new-user" />
              <${lazy(() => import('../pages/AdminNewUser.js').then((m) => m.AdminNewAdmin))} path="/admin/new-admin" />
              <${lazy(() => import('../pages/AdminUserSettings.js').then((m) => m.AdminUserSettings))} path="/admin/user-settings/:userId" />
              <${lazy(() => import('../pages/AdminUserSettings.js').then((m) => m.AdminAdminSettings))} path="/admin/admin-settings/:adminId" />
              <${lazy(() => import('../pages/AdminAdminList.js'))} path="/admin/admin-list" />
              <${lazy(() => import('../pages/AdminGroupList.js'))} path="/admin/group-list" />
              <${lazy(() => import('../pages/AdminNewGroup.js'))} path="/admin/new-group" />
              <${lazy(() => import('../pages/AdminGroupSettings.js'))} path="/admin/group-settings/:groupId" />
              <${lazy(() => import('../pages/Forbidden.js'))} path="/403" />
              <${lazy(() => import('../pages/NotFound.js'))} default />
            <//>
          <//>
        <//>
      `;
}

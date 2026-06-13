import { html } from 'htm/preact';
import { useEffect } from 'preact/hooks';
import { LocationProvider, Router, lazy } from 'preact-iso';
import { refreshAuth } from '../store/auth.js';

export function App() {
  useEffect(() => {
    refreshAuth();
  }, []);

  return html`
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
  `;
}

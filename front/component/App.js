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
        <${lazy(() => import('../pages/AddPassword.js'))} path="/add-password" />
        <${lazy(() => import('../pages/AddPassword.js'))} path="/add-password/:username" />
        <${lazy(() => import('../pages/UpdateUser.js'))} path="/update-user" />
        <${lazy(() => import('../pages/WatchVideo.js'))} path="/watch-video/:videoId" />
        <${lazy(() => import('../pages/AdminDashboard.js'))} path="/admin" />
        <${lazy(() => import('../pages/AdminVideoList.js'))} path="/admin/video-list" />
        <${lazy(() => import('../pages/AdminAddVideo.js'))} path="/admin/add-video" />
        <${lazy(() => import('../pages/AdminUpdateVideo.js'))} path="/admin/update-video/:videoId" />
        <${lazy(() => import('../pages/AdminUserList.js'))} path="/admin/user-list" />
        <${lazy(() => import('../pages/AdminAddUser.js'))} path="/admin/add-user" />
        <${lazy(() => import('../pages/AdminUpdateUser.js'))} path="/admin/update-user/:userId" />
        <${lazy(() => import('../pages/AdminAdminList.js'))} path="/admin/admin-list" />
        <${lazy(() => import('../pages/AdminGroupList.js'))} path="/admin/group-list" />
        <${lazy(() => import('../pages/AdminAddGroup.js'))} path="/admin/add-group" />
        <${lazy(() => import('../pages/AdminUpdateGroup.js'))} path="/admin/update-group/:groupId" />
        <${lazy(() => import('../pages/Forbidden.js'))} path="/403" />
        <${lazy(() => import('../pages/NotFound.js'))} default />
      <//>
    <//>
  `;
}

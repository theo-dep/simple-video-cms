import { html } from 'htm/preact';
import { useState } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { websiteName } from '../store/env.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';
import { Loader } from '../component/Loader.js';

export default function AdminDashboard() {
  const { route } = useLocation();
  const [stats, setStats] = useState(null);
  const { isLoading } = useLoader(load, stats !== null);

  useTitle('Admin Dashboard');

  async function load() {
    try {
      const r = await api.adminStats();
      setStats(r.json ?? r);
    } catch {
      route('/403');
    }
  }

  return html`
    <${AdminNav} />

    <${Content}>
      ${isLoading
        ? html`<${Loader} />`
        : html`
            <h2>${websiteName} Statistics</h2>
            <table id="table" class="table pure-table pure-table-horizontal">
              <tbody>
                <tr>
                  <td>Total number of users registered</td>
                  <td><a href="/admin/user-list">${stats.userCount}</a></td>
                </tr>
                <tr>
                  <td>Total number of groups registered</td>
                  <td><a href="/admin/group-list">${stats.groupCount}</a></td>
                </tr>
                <tr>
                  <td>Total number of videos uploaded</td>
                  <td><a href="/admin/video-list">${stats.videoCount}</a></td>
                </tr>
              </tbody>
            </table>
          `}
    <//>
  `;
}

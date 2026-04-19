import { html } from 'htm/preact';
import { useState, useEffect } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { websiteName } from '../store/auth.js';
import { api } from '../api.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';

export default function AdminDashboard() {
  const { route } = useLocation();
  const [stats, setStats] = useState(null);

  useEffect(() => {
    api
      .adminStats()
      .then(({ json }) => setStats(json))
      .catch(() => route('/403'));
  }, []);

  return html`
    <${AdminNav} />

    <${Content}>
      <h2>${websiteName} Statistics</h2>
      ${stats &&
      html`
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

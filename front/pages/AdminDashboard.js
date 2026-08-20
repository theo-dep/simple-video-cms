import { html } from 'htm/preact';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { stats, loadStats } from '../store/admin.js';
import { websiteName } from '../store/env.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/HeaderNav.js';
import { Loader } from '../component/Loader.js';

export default function AdminDashboard() {
  const { isLoading } = useLoader(loadStats, stats.value !== null);

  useTitle('Admin Dashboard');

  return html`
    <${AdminNav} />

    <${Content}>
      ${
        isLoading
          ? html`<${Loader} />`
          : html`
              <h2>${websiteName} Statistics</h2>
              <table class="table">
                <tbody>
                  <tr>
                    <td>Total number of users registered</td>
                    <td><a href="/admin/user-list">${stats.value.userCount}</a></td>
                  </tr>
                  <tr>
                    <td>Total number of groups registered</td>
                    <td><a href="/admin/group-list">${stats.value.groupCount}</a></td>
                  </tr>
                  <tr>
                    <td>Total number of videos uploaded</td>
                    <td><a href="/admin/video-list">${stats.value.videoCount}</a></td>
                  </tr>
                </tbody>
              </table>
            `
      }
    <//>
  `;
}

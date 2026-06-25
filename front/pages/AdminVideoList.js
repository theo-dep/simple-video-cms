import { html } from 'htm/preact';
import { useState } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { selectedItem } from '../store/selection.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';
import { Drawer } from '../component/Drawer.js';
import { ListTable } from '../component/ListTable.js';
import { Loader } from '../component/Loader.js';
import { CloudPlusIcon } from '../svg/CloudPlusIcon.js';
import { CloudArrowDownIcon } from '../svg/CloudArrowDownIcon.js';

export default function AdminVideoList() {
  const { route } = useLocation();
  const [videos, setVideos] = useState(null);
  const { isLoading } = useLoader(load, Array.isArray(videos));

  useTitle('Video List');

  async function load() {
    try {
      const r = await api.adminVideoList();
      setVideos(r.json ?? r);
    } catch {
      route('/403');
    }
  }

  function updateVideo(video) {
    selectedItem.value = video;
    route('/admin/video-settings/' + video.id);
  }

  async function deleteVideo(id) {
    if (!confirm('Delete this video?')) return;
    await api.adminDeleteVideo(id);
    await load();
  }

  return html`
    <${AdminNav} />

    <${Content}>
      ${isLoading
        ? html`<${Loader} />`
        : html`<${ListTable}
            title="List of videos"
            icon="${html`<title>Add video</title> <${CloudPlusIcon} />`}"
            addLink="/admin/new-video"
            columns="${['', 'Video Name']}"
            items="${videos}"
            renderRow="${(v) => html`
              <tr key=${v.id}>
                <td>
                  <a href=${api.adminDownloadVideoPath(v.id)} download=${v.title + '.mp4'}>
                    <svg class="svg-button">
                      <title>Download</title>
                      <${CloudArrowDownIcon} />
                    </svg>
                  </a>
                </td>
                <td><a href=${'/watch-video/' + v.id}>${v.title}</a></td>
                <td>
                  <div class="pure-g">
                    <div class="pure-u-1 pure-u-lg-1-3">
                      ${(!!v.groups?.length || !!v.users?.length) &&
                      html`
                        <${Drawer}
                          label="Rights"
                          items=${[
                            { label: 'Group Rights', elements: v.groups?.map((g) => g.name) ?? [] },
                            { label: 'User Rights', elements: v.users?.map((u) => u.name) ?? [] },
                          ]}
                        />
                      `}
                    </div>
                    <div class="pure-u-1 pure-u-lg-1-3">
                      <a onClick=${() => updateVideo(v)} style="cursor:pointer">Update</a>
                    </div>
                    <div class="pure-u-1 pure-u-lg-1-3">
                      <a onClick=${() => deleteVideo(v.id)} style="cursor:pointer">Delete</a>
                    </div>
                  </div>
                </td>
              </tr>
            `}"
          /> `}
    <//>
  `;
}

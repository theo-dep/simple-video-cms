import { html } from 'htm/preact';
import { useState, useEffect } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { selectedItem } from '../store/selection.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';
import { Drawer } from '../component/Drawer.js';
import { Loader } from '../component/Loader.js';
import { CloudPlusIcon } from '../svg/CloudPlusIcon.js';
import { CloudArrowDownIcon } from '../svg/CloudArrowDownIcon.js';

export default function AdminVideoList() {
  const { route } = useLocation();
  const [videos, setVideos] = useState(null);
  const [isLoading, setIsLoading] = useState(true);

  useTitle('Video List');

  function load() {
    api
      .adminVideoList()
      .then((r) => {
        setVideos(r.json ?? r);
        setIsLoading(false);
      })
      .catch(() => route('/403'));
  }

  useEffect(() => {
    if (Array.isArray(videos)) {
      setIsLoading(false);
      return;
    }
    load();
  }, []);

  function updateVideo(video) {
    selectedItem.value = video;
    route('/admin/video-settings/' + video.id);
  }

  async function deleteVideo(id) {
    if (!confirm('Delete this video?')) return;
    await api.adminDeleteVideo(id);
    load();
  }

  return html`
    <${AdminNav} />

    <${Content}>
      ${isLoading
        ? html`<${Loader} />`
        : videos.value.length === 0
          ? html`
              <h1>Nothing uploaded yet</h1>
              <h3><a href="/admin/new-video">Upload a new video</a></h3>
            `
          : html`
              <h1>Uploaded videos</h1>
              <h3><a href="/admin/new-video">Upload a new video</a></h3>
              <table id="table" class="table pure-table pure-table-horizontal">
                <thead>
                  <tr>
                    <th></th>
                    <th>Video Name</th>
                    <th class="add-user">
                      <a href="/admin/new-video">
                        <svg class="svg-button">
                          <title>Add</title>
                          <${CloudPlusIcon} />
                        </svg>
                      </a>
                    </th>
                  </tr>
                </thead>
                <tbody>
                  ${videos.map(
                    (v) => html`
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
                    `
                  )}
                </tbody>
              </table>
            `}
    <//>
  `;
}

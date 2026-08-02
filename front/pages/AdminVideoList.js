import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { refreshRequested } from '../store/auth.js';
import { selectedVideo, videos, loadVideos, invalidateVideos } from '../store/admin.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';
import { Drawer } from '../component/Drawer.js';
import { ListTable } from '../component/ListTable.js';
import { Loader } from '../component/Loader.js';
import { CloudPlusIcon } from '../svg/CloudPlusIcon.js';
import { CloudArrowDownIcon } from '../svg/CloudArrowDownIcon.js';
import { confirm } from '../component/ConfirmDialog.js';

export default function AdminVideoList() {
  const { route } = useLocation();
  const { isLoading } = useLoader(loadVideos, Array.isArray(videos.value));

  useTitle('Video List');

  function updateVideo(video) {
    selectedVideo.value = video;
    route('/admin/video-settings/' + selectedVideo.value.id);
  }

  async function deleteVideo(id) {
    const name = videos.value?.find((v) => v.id === id)?.title ?? 'this video';
    if (!(await confirm(`Delete ${name} video?`))) return;
    await api.adminDeleteVideo(id);
    invalidateVideos(); // reset stats
    refreshRequested.value = true; // remove this video to the admin video list (current user)
    await loadVideos();
  }

  return html`
    <${AdminNav} />

    <${Content}>
      ${
        isLoading
          ? html`<${Loader} />`
          : html`<${ListTable}
              title="List of videos"
              icon="${html`<title>Add video</title> <${CloudPlusIcon} />`}"
              addLink="/admin/new-video"
              columns="${[null, { label: 'Video Name', key: 'title' }]}"
              items="${videos.value}"
              searchKeys="${['title']}"
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
                  <td><a href=${'/video/' + v.id}>${v.title}</a></td>
                  <td>
                    <div class="pure-g">
                      <div class="table-button pure-u-1 pure-u-lg-1-3">
                        ${
                          (!!v.groups?.length || !!v.users?.length) &&
                          html`
                            <${Drawer}
                              label="Rights"
                              items=${[
                                { label: 'Group Rights', elements: v.groups?.map((g) => g.name) ?? [] },
                                { label: 'User Rights', elements: v.users?.map((u) => u.name) ?? [] },
                              ]}
                            />
                          `
                        }
                      </div>
                      <div class="table-button pure-u-1 pure-u-lg-1-3">
                        <a onClick=${() => updateVideo(v)}>Update</a>
                      </div>
                      <div class="table-button pure-u-1 pure-u-lg-1-3">
                        <a onClick=${() => deleteVideo(v.id)}>Delete</a>
                      </div>
                    </div>
                  </td>
                </tr>
              `}"
            /> `
      }
    <//>
  `;
}

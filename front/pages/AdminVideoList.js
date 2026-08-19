import { html } from 'htm/preact';
import { useLocation } from 'preact-iso';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { useLoader } from '../hook/useLoader.js';
import { refreshRequested } from '../store/auth.js';
import { selectedVideo, videos, loadVideos, invalidateVideos } from '../store/admin.js';
import { Content } from '../component/Content.js';
import { AdminNav } from '../component/UserNav.js';
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
              addContent="${html`<svg class="svg-button"><${CloudPlusIcon} /></svg> New video`}"
              addLink="/admin/new-video"
              columns="${[
                { key: 'title', label: 'Title', sortValue: (v) => v.title, render: (v) => html`<a href=${'/video/' + v.id}>${v.title}</a>` },
                { key: 'date', label: 'Date', sortValue: (v) => v.date, render: (v) => v.date },
                { key: 'location', label: 'Location', sortValue: (v) => v.location?.name, render: (v) => v.location?.name },
              ]}"
              items="${videos.value}"
              searchKeys="${['title', 'date', 'location.name', 'authors.name', 'tags.name']}"
              renderExpanded="${(v) => html`
                <div class="list-expanded-actions">
                  <a href=${api.adminDownloadVideoPath(v.id)} download=${v.title + '.mp4'}>
                    <svg class="svg-button"><${CloudArrowDownIcon} /></svg> Download
                  </a>
                  <a onClick=${() => updateVideo(v)}>Update</a>
                  <a onClick=${() => deleteVideo(v.id)}>Delete</a>
                </div>
                ${
                  (!!v.authors.length || !!v.tags.length || !!v.groups?.length || !!v.users?.length) &&
                  html`
                    <div class="list-expanded-info">
                      ${
                        !!v.authors?.length &&
                        html`
                          <div class="list-expanded-info-row">
                            <h4>Authors</h4>
                            ${v.authors?.map((a) => html`<p>${a.name}</p>`)}
                          </div>
                        `
                      }
                      ${
                        !!v.tags?.length &&
                        html`
                          <div class="list-expanded-info-row">
                            <h4>Tags</h4>
                            ${v.tags?.map((t) => html`<p>${t.name}</p>`)}
                          </div>
                        `
                      }
                      ${
                        !!v.groups?.length &&
                        html`
                          <div class="list-expanded-info-row">
                            <h4>Group Rights</h4>
                            ${v.groups?.map((g) => html`<p>${g.name}</p>`)}
                          </div>
                        `
                      }
                      ${
                        !!v.users?.length &&
                        html`
                          <div class="list-expanded-info-row">
                            <h4>User Rights</h4>
                            ${v.users?.map((u) => html`<p>${u.name}</p>`)}
                          </div>
                        `
                      }
                    </div>
                  `
                }
              `}"
            />`
      }
    <//>
  `;
}

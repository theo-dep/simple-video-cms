import { html } from 'htm/preact';
import { useSearch } from '../hook/useSearch.js';
import { useTitle } from '../hook/useTitle.js';
import { user, refreshed } from '../store/auth.js';
import { Content } from '../component/Content.js';
import { UserNav } from '../component/UserNav.js';
import { SearchInput } from '../component/SearchInput.js';
import { Footer } from '../component/Footer.js';
import { VideoThumbnail } from '../component/VideoThumbnail.js';
import { Loader } from '../component/Loader.js';

export default function Home() {
  const allVideos = user.videos.value;
  const { results, search } = useSearch(allVideos, ['title']);
  const isLoading = !refreshed.value;

  useTitle('Home');

  return html`
    <${UserNav}>
      <${SearchInput} onSearch=${search} />
    <//>

    <${Content} class="pure-g">
      ${
        isLoading
          ? html`<${Loader} />`
          : results.map(
              (v) =>
                html` <div key=${v.id} class="pure-u-1 pure-u-sm-1-2 pure-u-lg-1-3 pure-u-xl-1-4">
                  <a href=${'/video/' + v.id} class="video-card">
                    <div class="video-card-thumb">
                      <${VideoThumbnail} id=${v.id} title=${v.title} />
                    </div>
                    <h4 class="video-title">${v.title}</h4>
                  </a>
                </div>`
            )
      }
    <//>

    <${Footer} />
  `;
}

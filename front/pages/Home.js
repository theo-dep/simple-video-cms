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
      ${isLoading
        ? html`<${Loader} />`
        : results.map(
            (v) => html`
          <div class="pure-u-1 pure-u-sm-2-5" key=${v.id}>
            <a href=${'/video/' + v.id}>
              <${VideoThumbnail} id=${v.id} title=${v.title} //>
            </a>
          </div>
          <div class="pure-u-1 pure-u-sm-3-5" key=${v.id}>
            <h4><a href=${'/video/' + v.id}>${v.title}</a></h4>
          </div>
          <hr class="pure-u-1" />
        `
          )}
    <//>

    <${Footer} />
  `;
}

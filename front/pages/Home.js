import { html } from 'htm/preact';
import { useState } from 'preact/hooks';
import { user } from '../store/auth.js';
import { api } from '../api.js';
import { Content } from '../component/Content.js';
import { UserNav } from '../component/UserNav.js';
import { Footer } from '../component/Footer.js';

export default function Home() {
  const [search, setSearch] = useState('');

  function onSearch(e) {
    e.preventDefault();
    setSearch(e.target.elements['search'].value.trim());
  }

  const videos = user.videos.value.filter((v) => !search || v.title.toLowerCase().includes(search.toLowerCase()));

  return html`
    <${UserNav}>
      <form class="form-search pure-form pure-g" onSubmit=${onSearch}>
        <div class="pure-u-2-3">
          <input type="text" class="pure-input-rounded pure-input-1" name="search" placeholder="Search" />
        </div>
        <div class="pure-u-1-3">
          <button id="search-button" class="button button-search pure-button pure-input-1" type="submit">Search</button>
        </div>
      </form>
    <//>

    <${Content} class="pure-g">
      ${videos.map(
        (v) => html`
          <div class="pure-u-1 pure-u-md-1-3" key=${v.id}>
            <a href=${'/watch-video/' + v.id}>
              <img src=${api.thumbnailPath(v.id)} class="thumbnail pure-img" alt=${v.title} />
            </a>
          </div>
          <div class="pure-u-1 pure-u-md-2-3" key=${v.id}>
            <h4><a href=${'/watch-video/' + v.id}>${v.title}</a></h4>
          </div>
          <hr class="pure-u-1" />
        `
      )}
    <//>

    <${Footer} />
  `;
}

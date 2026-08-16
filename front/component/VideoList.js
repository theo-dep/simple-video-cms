import { html } from 'htm/preact';
import { useMemo, useState } from 'preact/hooks';
import { useSearch } from '../hook/useSearch.js';
import { useTitle } from '../hook/useTitle.js';
import { user, refreshed } from '../store/auth.js';
import { Content } from '../component/Content.js';
import { UserNav } from './UserNav.js';
import { MultiSelectDropDown } from '../component/SelectDropDown.js';
import { SearchInput } from '../component/SearchInput.js';
import { VideoThumbnail } from '../component/VideoThumbnail.js';
import { Loader } from '../component/Loader.js';
import { BookmarkButton } from '../component/BookmarkButton.js';
import { ArrowDownIcon } from '../svg/ArrowDownIcon.js';
import { TagIcon } from '../svg/TagIcon.js';
import { CalendarIcon } from '../svg/CalendarIcon.js';
import { LocationIcon } from '../svg/LocationIcon.js';
import { PencilIcon } from '../svg/PencilIcon.js';

function unique(items) {
  return [...new Set(items)].sort();
}

export function VideoList({ title, filterCondition }) {
  const allVideos = useMemo(() => user.videos.value.filter(filterCondition), [user.videos.value, filterCondition]);

  const { results, search } = useSearch(allVideos, ['title', 'date', 'place', 'authors', 'tags']);

  const [places, setPlaces] = useState([]);
  const [authors, setAuthors] = useState([]);
  const [tags, setTags] = useState([]);
  const [filtersOpen, setFiltersOpen] = useState(false);

  const placeOptions = useMemo(() => unique(allVideos.map((v) => v.place).filter(Boolean)), [allVideos]);
  const authorOptions = useMemo(() => unique(allVideos.flatMap((v) => v.authors ?? [])), [allVideos]);
  const tagOptions = useMemo(() => unique(allVideos.flatMap((v) => v.tags ?? [])), [allVideos]);

  const activeFilterCount = places.length + authors.length + tags.length;

  const filtered = useMemo(
    () =>
      results.filter(
        (v) =>
          (!places.length || places.includes(v.place)) &&
          (!authors.length || v.authors?.some((a) => authors.includes(a))) &&
          (!tags.length || v.tags?.some((t) => tags.includes(t)))
      ),
    [results, places, authors, tags]
  );

  const isLoading = !refreshed.value;

  useTitle(title);

  return html`
    <${UserNav} />

    <${Content} class="pure-g">
      <div class="pure-u-1">
        <div class="pure-g">
          <div class="pure-u-1 pure-u-sm-3-4 pure-u-md-1-2 pure-u-lg-1-3">
            <${SearchInput} onSearch=${search} />
          </div>
          <button
            type="button"
            class="pure-button button filter-toggle ${filtersOpen ? 'is-open' : ''}"
            onClick=${() => setFiltersOpen(!filtersOpen)}
          >
            Filters ${!!activeFilterCount && html`<span class="filter-badge">${activeFilterCount}</span>`}
            <${ArrowDownIcon} class="filter-toggle-arrow" />
          </button>
        </div>

        ${
          filtersOpen &&
          html`
            <div class="search-filters pure-g">
              <div class="search-filter pure-u-1 pure-u-md-1-3 pure-u-lg-1-4">
                <${MultiSelectDropDown} name="place-filter" placeholder="Places" onChange=${setPlaces}>
                  ${placeOptions.map((p) => html`<option key=${p} value=${p}>${p}</option>`)}
                <//>
              </div>
              <div class="search-filter pure-u-1 pure-u-md-1-3 pure-u-lg-1-4">
                <${MultiSelectDropDown} name="author-filter" placeholder="Authors" onChange=${setAuthors}>
                  ${authorOptions.map((a) => html`<option key=${a} value=${a}>${a}</option>`)}
                <//>
              </div>
              <div class="search-filter pure-u-1 pure-u-md-1-3 pure-u-lg-1-4">
                <${MultiSelectDropDown} name="tag-filter" placeholder="Tags" onChange=${setTags}>
                  ${tagOptions.map((t) => html`<option key=${t} value=${t}>${t}</option>`)}
                <//>
              </div>
            </div>
          `
        }
      </div>

      ${
        isLoading
          ? html`<${Loader} />`
          : filtered.map(
              (v) =>
                html` <div key=${v.id} class="pure-u-1 pure-u-sm-1-2 pure-u-lg-1-3 pure-u-xl-1-4">
                  <a href=${'/video/' + v.id} class="video-card">
                    <div class="video-card-thumb">
                      <${VideoThumbnail} id=${v.id} title=${v.title} />
                      ${user.isLogged.value && html`<${BookmarkButton} videoId=${v.id} isBookmarked=${v.bookmarked} location="home" />`}
                    </div>
                    <div class="video-info">
                      <h4 class="video-title">${v.title}</h4>
                      ${
                        (v.date || v.place || !!v.authors?.length || !!v.tags?.length) &&
                        html`
                          <div class="video-meta">
                            ${v.date && html`<span class="meta-item meta-date"><${CalendarIcon} /> ${v.date}</span>`}
                            ${v.place && html`<span class="meta-item meta-place"><${LocationIcon} /> ${v.place}</span>`}
                            ${!!v.authors?.length && html`<span class="meta-item meta-authors"><${PencilIcon} /> ${v.authors.join(', ')}</span>`}
                          </div>
                          <div class="video-meta">
                            ${!!v.tags?.length && v.tags.map((t) => html`<span class="meta-item meta-tag"><${TagIcon} /> ${t}</span>`)}
                          </div>
                        `
                      }
                    </div>
                  </a>
                </div>`
            )
      }
    <//>
  `;
}

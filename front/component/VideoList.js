import { html } from 'htm/preact';
import { useEffect, useMemo, useState } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { useSearch } from '../hook/useSearch.js';
import { useTitle } from '../hook/useTitle.js';
import { user, refreshed } from '../store/auth.js';
import { Content } from '../component/Content.js';
import { UserNav } from './HeaderNav.js';
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
  const { path, query, route } = useLocation();

  const allVideos = useMemo(() => user.videos.value.filter(filterCondition), [user.videos.value, filterCondition]);

  const { results, search } = useSearch(allVideos, ['title', 'date', 'location', 'authors', 'tags']);

  const [locations, setLocations] = useState([]);
  const [authors, setAuthors] = useState([]);
  const [tags, setTags] = useState([]);
  const [filtersOpen, setFiltersOpen] = useState(false);

  useEffect(() => {
    setLocations(query?.locations ? query.locations.split(';') : []);
    setAuthors(query?.authors ? query.authors.split(';') : []);
    setTags(query?.tags ? query.tags.split(';') : []);
  }, [query]);

  function routeSearch(key, value) {
    const params = new URLSearchParams(query);
    value.length ? params.set(key, value.join(';')) : params.delete(key);
    const paramQuery = params.size ? `?${params}` : '';
    route(`${path}${paramQuery}`, /*replace*/ true);
  }

  function onLocationChange(values) {
    setLocations(values);
    routeSearch('locations', values);
  }

  function onAuthorsChange(values) {
    setAuthors(values);
    routeSearch('authors', values);
  }

  function onTagsChange(values) {
    setTags(values);
    routeSearch('tags', values);
  }

  function isLocationSelected(location) {
    return locations.includes(location);
  }

  function isAuthorSelected(author) {
    return authors.includes(author);
  }

  function isTagSelected(tag) {
    return tags.includes(tag);
  }

  const locationOptions = useMemo(() => unique(allVideos.map((v) => v.location).filter(Boolean)), [allVideos]);
  const authorOptions = useMemo(() => unique(allVideos.flatMap((v) => v.authors ?? [])), [allVideos]);
  const tagOptions = useMemo(() => unique(allVideos.flatMap((v) => v.tags ?? [])), [allVideos]);

  const activeFilterCount = locations.length + authors.length + tags.length;

  const filtered = useMemo(
    () =>
      results.filter(
        (v) =>
          (!locations.length || locations.includes(v.location)) &&
          (!authors.length || v.authors?.some((a) => authors.includes(a))) &&
          (!tags.length || v.tags?.some((t) => tags.includes(t)))
      ),
    [results, locations, authors, tags]
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
                <${MultiSelectDropDown} name="location-filter" placeholder="Locations" onChange=${onLocationChange}>
                  ${locationOptions.map((p) => html`<option key=${p} value=${p} selected=${isLocationSelected(p)}>${p}</option>`)}
                <//>
              </div>
              <div class="search-filter pure-u-1 pure-u-md-1-3 pure-u-lg-1-4">
                <${MultiSelectDropDown} name="author-filter" placeholder="Authors" onChange=${onAuthorsChange}>
                  ${authorOptions.map((a) => html`<option key=${a} value=${a} selected=${isAuthorSelected(a)}>${a}</option>`)}
                <//>
              </div>
              <div class="search-filter pure-u-1 pure-u-md-1-3 pure-u-lg-1-4">
                <${MultiSelectDropDown} name="tag-filter" placeholder="Tags" onChange=${onTagsChange}>
                  ${tagOptions.map((t) => html`<option key=${t} value=${t} selected=${isTagSelected(t)}>${t}</option>`)}
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
                        (v.date || v.location || !!v.authors?.length || !!v.tags?.length) &&
                        html`
                          <div class="video-meta">
                            ${v.date && html`<span class="meta-item meta-date"><${CalendarIcon} /> ${v.date}</span>`}
                            ${v.location && html`<span class="meta-item meta-location"><${LocationIcon} /> ${v.location}</span>`}
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

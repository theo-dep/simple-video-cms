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
import { Icon } from '../component/Icon.js';

function unique(items) {
  return [...new Set(items)].sort();
}

export function VideoList({ title, filterCondition }) {
  const { path, query, route } = useLocation();

  const allVideos = useMemo(() => user.videos.value.filter(filterCondition), [user.videos.value, filterCondition]);

  const { results, search } = useSearch(allVideos, ['title', 'date', 'location', 'authors', 'tags']);

  const [titles, setTitles] = useState([]);
  const [locations, setLocations] = useState([]);
  const [authors, setAuthors] = useState([]);
  const [tags, setTags] = useState([]);
  const [filtersOpen, setFiltersOpen] = useState(false);

  const titleOptions = useMemo(() => unique(allVideos.map((v) => v.title).filter(Boolean)), [allVideos]);
  const locationOptions = useMemo(() => unique(allVideos.map((v) => v.location).filter(Boolean)), [allVideos]);
  const authorOptions = useMemo(() => unique(allVideos.flatMap((v) => v.authors ?? [])), [allVideos]);
  const tagOptions = useMemo(() => unique(allVideos.flatMap((v) => v.tags ?? [])), [allVideos]);

  useEffect(() => {
    setTitles(query?.titles ? query.titles.split(';').filter((t) => titleOptions.includes(t)) : []);
    setLocations(query?.locations ? query.locations.split(';').filter((l) => locationOptions.includes(l)) : []);
    setAuthors(query?.authors ? query.authors.split(';').filter((a) => authorOptions.includes(a)) : []);
    setTags(query?.tags ? query.tags.split(';').filter((t) => tagOptions.includes(t)) : []);
  }, [query, titleOptions, locationOptions, authorOptions, tagOptions]);

  function routeSearch(key, value) {
    const params = new URLSearchParams(query);
    value.length ? params.set(key, value.join(';')) : params.delete(key);
    const paramQuery = params.size ? `?${params}` : '';
    route(`${path}${paramQuery}`, /*replace*/ true);
  }

  function onTitleChange(values) {
    setTitles(values);
    routeSearch('titles', values);
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

  function isTitleSelected(title) {
    return titles.includes(title);
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

  const activeFilterCount = titles.length + locations.length + authors.length + tags.length;

  const filtered = useMemo(
    () =>
      results.filter(
        (v) =>
          (!titles.length || titles.includes(v.title)) &&
          (!locations.length || locations.includes(v.location)) &&
          (!authors.length || v.authors?.some((a) => authors.includes(a))) &&
          (!tags.length || v.tags?.some((t) => tags.includes(t)))
      ),
    [results, titles, locations, authors, tags]
  );

  const isLoading = !refreshed.value;

  useTitle(title);

  return html`
    <${UserNav} />

    <${Content}>
      <div class="video-list-search">
        <div class="video-list-search-row">
          <div class="video-list-search-input">
            <${SearchInput} onSearch=${search} />
          </div>
          <button type="button" class="button filter-toggle ${filtersOpen ? 'is-open' : ''}" onClick=${() => setFiltersOpen(!filtersOpen)}>
            Filters ${!!activeFilterCount && html`<span class="filter-badge">(${activeFilterCount})</span>`}
            <${Icon} name="chevron-down" class="filter-toggle-arrow" />
          </button>
        </div>
        ${
          filtersOpen &&
          html`
            <div class="video-list-filters">
              <div class="video-list-filter">
                <${MultiSelectDropDown} name="title-filter" placeholder="Titles" onChange=${onTitleChange}>
                  ${titleOptions.map((t) => html`<option key=${t} value=${t} selected=${isTitleSelected(t)}>${t}</option>`)}
                <//>
              </div>
              <div class="video-list-filter">
                <${MultiSelectDropDown} name="location-filter" placeholder="Locations" onChange=${onLocationChange}>
                  ${locationOptions.map((l) => html`<option key=${l} value=${l} selected=${isLocationSelected(l)}>${l}</option>`)}
                <//>
              </div>
              <div class="video-list-filter">
                <${MultiSelectDropDown} name="author-filter" placeholder="Authors" onChange=${onAuthorsChange}>
                  ${authorOptions.map((a) => html`<option key=${a} value=${a} selected=${isAuthorSelected(a)}>${a}</option>`)}
                <//>
              </div>
              <div class="video-list-filter">
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
          : html`<div class="video-grid">
              ${filtered.map(
                (v, i) => html`
                  <a key=${v.id} href=${'/video/' + v.id} class="video-card">
                    <div class="video-card-thumb">
                      <${VideoThumbnail} id=${v.id} title=${v.title} priority=${i < 4} />
                      ${user.isLogged.value && html`<${BookmarkButton} videoId=${v.id} isBookmarked=${v.bookmarked} location="home" />`}
                    </div>
                    <div class="video-info">
                      <h4 class="video-title">${v.title}</h4>
                      ${
                        (v.date || v.location || !!v.authors?.length || !!v.tags?.length) &&
                        html`
                          <div class="video-meta">
                            ${v.date && html`<span class="meta-item meta-date"><${Icon} name="calendar-date" /> ${v.date}</span>`}
                            ${v.location && html`<span class="meta-item meta-location"><${Icon} name="geo" /> ${v.location}</span>`}
                            ${!!v.authors?.length && html`<span class="meta-item meta-authors"><${Icon} name="pencil-square" /> ${v.authors.join(', ')}</span>`}
                          </div>
                          <div class="video-meta">
                            ${!!v.tags?.length && v.tags.map((t) => html`<span class="meta-item meta-tag"><${Icon} name="tag" /> ${t}</span>`)}
                          </div>
                        `
                      }
                    </div>
                  </a>
                `
              )}
            </div>`
      }
    <//>
  `;
}

import { html } from 'htm/preact';
import { lazy, Suspense } from 'preact/compat';
import { useEffect } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import { useTitle } from '../hook/useTitle.js';
import { refreshed, user } from '../store/auth.js';
import { videoIdRedirected } from '../store/redirect.js';
import { swReady, postToServiceWorker } from '../store/sw.js';
import { InfoContent } from '../component/InfoContent.js';
import { UserNav } from '../component/UserNav.js';
import { Footer } from '../component/Footer.js';
import { Loader } from '../component/Loader.js';
import { ShareButton } from '../component/ShareButton.js';
import { BookmarkButton } from '../component/BookmarkButton.js';

// video.js can be long to load
const Video = lazy(() => import('../component/Video.js'));

export default function WatchVideo({ videoId }) {
  const { route } = useLocation();
  const video = user.videos.value.find((v) => v.id === Number(videoId));
  const isLoading = !refreshed.value;

  useTitle(video ? video.title : 'Watch Video');

  function onLoginClicked() {
    videoIdRedirected.value = videoId;
    route('/login');
  }

  useEffect(() => {
    if (!swReady.value) return;

    if (user.isLogged.value) {
      postToServiceWorker('enableVideoCaching');
    }

    return () => {
      postToServiceWorker('disableVideoCaching');
    };
  }, [swReady.value, user.isLogged.value]);

  return html`
    <${UserNav} videoId=${videoId} />

    <${InfoContent} class="info-video">
      ${
        isLoading
          ? html`<${Loader} />`
          : video
            ? html`
                <${Suspense} fallback=${html`<${Loader} />`}>
                  <h1>${video.title}</h1>
                  <${Video} videoId=${videoId} />
                  <div class="video-footer-content">
                    <${BookmarkButton} videoId=${video.id} isBookmarked=${video.bookmarked} location="video" />
                    <${ShareButton} />
                  </div>
                <//>
              `
            : html`
                <h3>Access Restricted</h3>
                <p>
                  It looks like you don't have permission to view this video.<br />
                  Please make sure you're logged in or have the right access.
                </p>
                <a onClick=${() => onLoginClicked()} style="cursor:pointer" class="back">Log in to continue</a>
              `
      }
    <//>

    <${Footer} />
  `;
}

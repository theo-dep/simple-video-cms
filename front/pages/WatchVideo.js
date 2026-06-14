import { html } from 'htm/preact';
import { useEffect, useRef } from 'preact/hooks';
import { useLocation } from 'preact-iso';
import videojs from 'video.js';
import { api } from '../api.js';
import { useTitle } from '../hook/useTitle.js';
import { user } from '../store/auth.js';
import { videoIdRedirected } from '../store/redirect.js';
import { InfoContent } from '../component/InfoContent.js';
import { UserNav } from '../component/UserNav.js';
import { Footer } from '../component/Footer.js';

import 'videojs-yt-style';
import 'videojs-mobile-ui';

import VideoJSStyleSheet from 'video.js/dist/video-js.css' with { type: 'css' };
import VideoJSMobileUIStyleSheet from 'videojs-mobile-ui/dist/videojs-mobile-ui.css' with { type: 'css' };
import VideoJSYtStyleSheet from 'videojs-yt-style/dist/videojs-yt-style.css' with { type: 'css' };

const additionalVideoJSStyle = new CSSStyleSheet();
additionalVideoJSStyle.replaceSync(`
.video-js .vjs-big-play-button:active,
.video-js .vjs-big-play-button:focus,
.video-js:hover .vjs-big-play-button {
    color: #16A085;
}

.video-js .vjs-progress-control .vjs-progress-holder .vjs-play-progress,
.video-js .vjs-progress-control .vjs-progress-holder .vjs-play-progress:before {
    background-color: #16A085;
}

.video-js .vjs-loading-spinner {
    border-color: #16A085;
}

.video-js .vjs-progress-control {
    top: -1em;
}

.video-js .vjs-play-control:focus-visible {
    outline: none;
}
`);

function waitForServiceWorkerController(timeoutMs = 1500) {
  if (navigator.serviceWorker.controller) {
    return Promise.resolve(true);
  }

  return new Promise((resolve) => {
    let settled = false;

    const resolveOnce = (value) => {
      if (settled) return;
      settled = true;
      navigator.serviceWorker.removeEventListener('controllerchange', onControllerChange);
      clearTimeout(timeoutId);
      resolve(value);
    };

    const onControllerChange = () => resolveOnce(true);
    const timeoutId = setTimeout(() => resolveOnce(false), timeoutMs);

    navigator.serviceWorker.addEventListener('controllerchange', onControllerChange);
  });
}

export default function WatchVideo({ videoId }) {
  const { route } = useLocation();
  const videoRef = useRef(null);
  const playerRef = useRef(null);
  const video = user.videos.value.find((v) => v.id === Number(videoId));

  useTitle(video ? video.title : 'Watch Video');

  useEffect(() => {
    document.adoptedStyleSheets = [VideoJSStyleSheet, VideoJSMobileUIStyleSheet, VideoJSYtStyleSheet, additionalVideoJSStyle];
    return () => {
      document.adoptedStyleSheets = document.adoptedStyleSheets.filter((s) => {
        return s !== VideoJSStyleSheet || s !== VideoJSMobileUIStyleSheet || s !== VideoJSYtStyleSheet || s !== additionalVideoJSStyle;
      });
    };
  }, []);

  function onLoginClicked() {
    videoIdRedirected.value = videoId;
    route('/login');
  }

  useEffect(() => {
    if (!video || !videoRef.current || playerRef.current) return;

    let debounce = null;

    async function waitForServiceWorker() {
      if (!(user.isLogged.value && 'serviceWorker' in navigator)) return;

      try {
        const waitForController = waitForServiceWorkerController();

        // Scope must be "/" for clients.claim() to work without a reload:
        // clients.claim() matches the registration scope against the client's
        // CREATION URL (the initial navigation that loaded the document), not
        // the current SPA route (pushState). Since this app is always loaded
        // from "/", only scope="/" can match and grant immediate control.
        const registration = await navigator.serviceWorker.register('/videoserviceworker.js', { scope: '/' });
        await navigator.serviceWorker.ready;

        const hasController = navigator.serviceWorker.controller ? true : await waitForController;

        const SW_PROMPT_KEY = 'video-sw-prompt-shown';
        if (!hasController && !sessionStorage.getItem(SW_PROMPT_KEY)) {
          sessionStorage.setItem(SW_PROMPT_KEY, '1');
          if (confirm('Do you want to reload this page to enable video caching?')) {
            window.location.reload();
          }
        }

        const sw = registration.installing || registration.waiting || registration.active;
        sw?.postMessage('enableCaching');
      } catch (err) {
        console.error('Video service worker registration failed', err);
      }
    }

    async function init() {
      await waitForServiceWorker();

      const existingPlayer = videojs.getPlayer(videoRef.current);
      if (existingPlayer) {
        playerRef.current = existingPlayer;
        return;
      }

      playerRef.current = videojs(videoRef.current, {
        html5: {
          vhs: {
            overrideNative: true,
            withCredentials: false,
          },
          nativeVideoTracks: false,
          nativeAudioTracks: false,
        },
        fluid: true,
        preload: 'metadata',
        playbackRates: [0.25, 0.5, 0.75, 1, 1.5, 2],
      });

      const player = playerRef.current;

      // player.mobileUi();
      player.ytStyle();

      player.src({
        src: api.videoPlaylistPath(videoId),
        type: 'application/x-mpegURL',
      });

      player.on('ready', async () => {
        await api.addVideoSession(videoId).catch(() => route('/403'));
      });

      let isSessionStarted = false;
      player.on('play', async () => {
        if (!isSessionStarted) {
          isSessionStarted = true;
          await api.startVideoSession(videoId).catch(() => route('/403'));
        }
      });

      // patch video.js to stop fetching a hls segment during seeking
      // this is made to synchronise the reset session api with seeking

      let isSeeking = false;
      let lastBlocked = null;

      let originalVhsXhr = null;

      player.on('xhr-hooks-ready', () => {
        originalVhsXhr = player.tech({ IWillNotUseThisInPlugins: true }).vhs.xhr;

        player.tech({ IWillNotUseThisInPlugins: true }).vhs.xhr = function (options, callback) {
          if (isSeeking && options.uri?.endsWith('.ts')) {
            lastBlocked = { options, callback };
            return {
              abort: () => {},
              addEventListener: () => {},
            };
          }

          return originalVhsXhr(options, callback);
        };
      });

      async function onSeekEnd() {
        await api.resetVideoSession(videoId).catch(() => route('/403'));
        isSeeking = false;

        if (lastBlocked && originalVhsXhr) {
          originalVhsXhr(lastBlocked.options, lastBlocked.callback);
          lastBlocked = null;
        }
      }

      player.on('seeking', () => {
        if (videojs.browser.IS_IOS) api.resetVideoSession(videoId).catch(() => route('/403'));

        isSeeking = true;
        clearTimeout(debounce);
        debounce = setTimeout(onSeekEnd, 300);
      });
    }

    init();

    return () => {
      if (playerRef.current) {
        playerRef.current.dispose();
        playerRef.current = null;
      }
      clearTimeout(debounce);
    };
  }, [video, videoId]);

  return html`
    <${UserNav} videoId=${videoId} />

    <${InfoContent} class="info-video">
      ${video
        ? html`
            <h1>${video.title}</h1>
            <video
              ref=${videoRef}
              onContextMenu=${(e) => e.preventDefault()}
              id="video-player"
              class="video-js vjs-default-skin"
              controls
              playsinline
            >
              <p class="vjs-no-js">
                To view this video please enable JavaScript and upgrade to a browser that
                <a href="https://videojs.com/html5-video-support/" target="_blank">supports HTML5 video</a>.
              </p>
            </video>
          `
        : html`
            <h3>Access Restricted</h3>
            <p>
              It looks like you don't have permission to view this video.<br />
              Please make sure you're logged in or have the right access.
            </p>
            <a onClick=${() => onLoginClicked()} style="cursor:pointer" class="back">Log in to continue</a>
          `}
    <//>

    <${Footer} />
  `;
}

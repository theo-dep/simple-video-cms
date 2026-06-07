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

  useEffect(() => {
    if (user.isLogged.value && 'serviceWorker' in navigator) {
      navigator.serviceWorker.register('/videoserviceworker.js', {
        scope: '/watch-video/',
      });
    }
  }, []);

  function onLoginClicked() {
    videoIdRedirected.value = videoId;
    route('/login');
  }

  useEffect(() => {
    if (!video || !videoRef.current || playerRef.current) return;

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

    let isSessionStarted = false;

    player.on('play', async () => {
      if (!isSessionStarted) {
        isSessionStarted = true;
        await api.startVideoSession(videoId).catch(() => route('/403'));
      }
    });

    player.on('seeking', async () => {
      await api.resetVideoSession(videoId).catch(() => route('/403'));
    });

    return () => {
      if (playerRef.current) {
        playerRef.current.dispose();
        playerRef.current = null;
      }
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

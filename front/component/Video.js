import { html } from 'htm/preact';
import { useEffect, useRef } from 'preact/hooks';
import videojs from 'video.js';
import { api } from '../api.js';
import { useStyleSheet } from '../hook/useStyleSheet.js';
import { Loader } from './Loader.js';

import 'videojs-yt-style';
import 'videojs-mobile-ui';

import VideoJSStyleSheet from 'video.js/dist/video-js.css' with { type: 'css' };
import VideoJSMobileUIStyleSheet from 'videojs-mobile-ui/dist/videojs-mobile-ui.css' with { type: 'css' };
import VideoJSYtStyleSheet from 'videojs-yt-style/dist/videojs-yt-style.css' with { type: 'css' };
import VideoStyleSheet from './Video.css' with { type: 'css' };

export default function Video({ videoId }) {
  const videoRef = useRef(null);
  const playerRef = useRef(null);

  const isAdoptedStyleSheets = useStyleSheet([VideoJSStyleSheet, VideoJSMobileUIStyleSheet, VideoJSYtStyleSheet, VideoStyleSheet]);
  if (!isAdoptedStyleSheets) {
    return html`<${Loader} />`;
  }

  useEffect(() => {
    if (!videoRef.current) return;

    // Dispose any existing player first to prevent memory leaks
    if (playerRef.current) {
      playerRef.current.dispose();
      playerRef.current = null;
    }

    const existingPlayer = videojs.getPlayer(videoRef.current);
    if (existingPlayer) {
      existingPlayer.dispose();
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
      await api.addVideoSession(videoId).catch((err) => console.error(err));
    });

    let isSessionStarted = false;
    player.on('play', async () => {
      if (!isSessionStarted) {
        isSessionStarted = true;
        await api.startVideoSession(videoId).catch((err) => console.error(err));
      }
    });

    // patch video.js to stop fetching a hls segment during seeking
    // this is made to synchronise the reset session api with seeking

    let isSeeking = false;
    let debounce = null;
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
      await api.resetVideoSession(videoId).catch((err) => console.error(err));
      isSeeking = false;

      if (lastBlocked && originalVhsXhr) {
        originalVhsXhr(lastBlocked.options, lastBlocked.callback);
        lastBlocked = null;
      }
    }

    player.on('seeking', () => {
      if (videojs.browser.IS_IOS) api.resetVideoSession(videoId).catch((err) => console.error(err));

      isSeeking = true;
      clearTimeout(debounce);
      debounce = setTimeout(onSeekEnd, 300);
    });

    return () => {
      if (playerRef.current) {
        playerRef.current.dispose();
        playerRef.current = null;
      }
      clearTimeout(debounce);
    };
  }, [videoId]);

  return html`
    <video ref=${videoRef} onContextMenu=${(e) => e.preventDefault()} id="video-player" class="video-js vjs-default-skin" controls playsinline>
      <p class="vjs-no-js">
        To view this video please enable JavaScript and upgrade to a browser that
        <a href="https://videojs.com/html5-video-support/" target="_blank">supports HTML5 video</a>.
      </p>
    </video>
  `;
}

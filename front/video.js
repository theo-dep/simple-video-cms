import videojs from 'video.js';
window.videojs = videojs;

await import('videojs-yt-style');
await import('videojs-mobile-ui');

delete window.videojs;

export default videojs;

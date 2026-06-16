export default async function disableSW() {
  if ('serviceWorker' in navigator) {
    const registration = await navigator.serviceWorker.getRegistration();
    const sw = registration?.installing || registration?.waiting || registration?.active;
    sw?.postMessage('disableVideoCaching');
  }
}

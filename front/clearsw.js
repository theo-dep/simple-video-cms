export default async function clearSW() {
  if ('serviceWorker' in navigator) {
    const registrations = await navigator.serviceWorker.getRegistrations();
    for (const registration of registrations) {
      if (registration?.active) {
        registration.active.postMessage('disableCaching');
      }
      await registration?.unregister();
    }
  }
}

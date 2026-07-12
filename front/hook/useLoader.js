import { useEffect } from 'preact/hooks';

export function useLoader(apiCall, isStored, deps = []) {
  function apiCallIfNotStored() {
    if (!isStored) {
      apiCall();
    }
  }
  useEffect(() => {
    apiCallIfNotStored();
    addEventListener('retry-fetches', apiCallIfNotStored);
    return () => removeEventListener('retry-fetches', apiCallIfNotStored);
  }, deps);

  return { isLoading: !isStored };
}

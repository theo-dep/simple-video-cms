import { useEffect } from 'preact/hooks';

export function useLoader(apiCall, isStored, deps = []) {
  useEffect(() => {
    if (!isStored) {
      apiCall();
    }
  }, deps);

  return { isLoading: !isStored };
}

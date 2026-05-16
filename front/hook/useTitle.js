import { useEffect } from 'preact/hooks';
import { websiteName } from '../store/auth.js';

export function useTitle(title) {
  useEffect(() => {
    const prev = document.title;
    document.title = (title ? `${title} - ` : '') + websiteName.value;
    return () => {
      document.title = prev;
    };
  }, [title]);
}

import { useState, useMemo } from 'preact/hooks';
import Fuse from 'fuse.js';

export function useSearch(items, keys) {
  const [results, setResults] = useState(items);

  const fuse = useMemo(() => {
    return new Fuse(items, {
      keys,
      useTokenSearch: true,
      includeScore: true,
      threshold: 0.35,
      ignoreLocation: true,
      ignoreDiacritics: true,
      findAllMatches: true,
      minMatchCharLength: 2,
    });
  }, [items]);

  function search(query) {
    setResults(query.trim() ? fuse.search(query).map((r) => r.item) : items);
  }

  return { results, search };
}

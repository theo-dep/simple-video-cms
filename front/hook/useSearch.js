import { useState, useMemo } from 'preact/hooks';
import Fuse from 'fuse.js';

export function useSearch(items, keys) {
  const [query, setQuery] = useState('');

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

  // don't break the reactivity chain from items to results
  const results = useMemo(() => {
    return query.trim() ? fuse.search(query).map((r) => r.item) : items;
  }, [fuse, query, items]);

  function search(query) {
    setQuery(query);
  }

  return { results, search };
}

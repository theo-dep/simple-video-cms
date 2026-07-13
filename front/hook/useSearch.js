import { useState, useMemo } from 'preact/hooks';
import Fuse from 'fuse.js';

export function useSearch(items, keys) {
  const [query, setQuery] = useState('');

  const fuse = useMemo(() => {
    if (!items || items.length === 0) return null;
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
  }, [items, keys]);

  // don't break the reactivity chain from items to results
  const results = useMemo(() => {
    if (!fuse) return items || [];
    const trimmedQuery = query.trim();
    return trimmedQuery ? fuse.search(trimmedQuery).map((r) => r.item) : items;
  }, [fuse, query]);

  function search(query) {
    setQuery(query);
  }

  return { results, search };
}

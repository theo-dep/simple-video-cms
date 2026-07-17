import { act, renderHook } from '@testing-library/preact';
import { describe, expect, it } from 'vitest';

import { useSearch } from '../hook/useSearch.js';

describe('useSearch', () => {
  it('returns full list when query empty', () => {
    const items = [{ title: 'Alpha video' }, { title: 'Beta clip' }];

    const { result } = renderHook(() => useSearch(items, ['title']));

    expect(result.current.results).toEqual(items);
  });

  it('filters list when query set', () => {
    const items = [{ title: 'Alpha video' }, { title: 'Beta clip' }, { title: 'Gamma stream' }];

    const { result } = renderHook(() => useSearch(items, ['title']));

    act(() => {
      result.current.search('alp');
    });

    expect(result.current.results).toEqual([{ title: 'Alpha video' }]);
  });

  it('trims query and returns full list for blank input', () => {
    const items = [{ title: 'Alpha video' }, { title: 'Beta clip' }];

    const { result } = renderHook(() => useSearch(items, ['title']));

    act(() => {
      result.current.search('   ');
    });

    expect(result.current.results).toEqual(items);
  });
});

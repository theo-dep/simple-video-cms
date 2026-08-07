import { useEffect, useState } from 'preact/hooks';

export function useStyleSheet(sheets) {
  const [isAdoptedStyleSheets, setAdoptedStyleSheets] = useState(false);

  useEffect(
    () => {
      const list = Array.isArray(sheets) ? sheets : [sheets];

      for (const sheet of list) {
        if (document.adoptedStyleSheets.includes(sheet)) continue;

        // append for Video.js
        document.adoptedStyleSheets = [...document.adoptedStyleSheets, sheet];
      }

      setAdoptedStyleSheets(true);
    },
    Array.isArray(sheets) ? sheets : [sheets]
  );

  return isAdoptedStyleSheets;
}

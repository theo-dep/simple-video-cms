import { videojsEntries } from './rollup.shared.js';

export default videojsEntries.map((entry) => ({
  ...entry,
  output: { dir: 'build/', format: 'es' },
}));

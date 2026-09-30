import { videojsEntries, onwarn } from './rollup.shared.js';

export default videojsEntries.map((entry) => ({
  ...entry,
  onwarn,
  output: { dir: 'build/', format: 'es' },
}));

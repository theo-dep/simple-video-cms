// Format bytes size
export function formatBytes(bytes) {
  if (bytes === 0 || !bytes) return '0 bytes';
  const k = 1024;
  const sizes = ['bytes', 'KB', 'MB', 'GB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

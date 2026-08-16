export function formatVideo(v) {
  const date = v.date ? ' - ' + v.date : '';
  const location = v.location ? ' - ' + v.location.name : '';
  const authors = v.authors.length ? ' - ' + v.authors.map((a) => a.name).join(', ') : '';
  return v.title + date + location + authors;
}

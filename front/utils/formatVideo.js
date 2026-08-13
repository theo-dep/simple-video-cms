export function formatVideo(v) {
  const date = v.date ? ' - ' + v.date : '';
  const place = v.place ? ' - ' + v.place.name : '';
  const authors = v.authors.length ? ' - ' + v.authors.map((a) => a.name).join(', ') : '';
  return v.title + date + place + authors;
}

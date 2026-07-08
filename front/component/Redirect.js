import { useEffect } from 'preact/hooks';
import { useLocation } from 'preact-iso';

export function Redirect({ to, ...props }) {
  const { route } = useLocation();

  useEffect(() => {
    const pathTo = to.replace(/\/:(\w+)/g, (match, key) => `/${props[key] || match}`);
    route(pathTo);
  }, [to, props]);

  return null;
}

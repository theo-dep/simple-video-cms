const uxAllowlist = /^[\p{L}\p{N}\p{M}\s.,!?'"()\-_:;]*$/u;

export function validateField(value) {
  if (uxAllowlist.test(value)) return;
  throw new Error('Unauthorized character set');
}

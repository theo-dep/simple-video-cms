const uxAllowlist = /^[\p{L}\p{N}\s.,!?'"()\-:;]*$/u;

export function validateField(value) {
  if (uxAllowlist.test(value)) return;
  throw new Error('Unauthorized character set');
}

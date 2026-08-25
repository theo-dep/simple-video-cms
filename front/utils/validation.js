const textAllowlist = /^[\p{L}\p{N}\p{M}\s.,!?'"()\-_:;]*$/u;
const dateAllowlist = /^[0-9/-]*$/;

export function validateText(value) {
  if (textAllowlist.test(value)) return;
  throw new Error('Special characters not allowed except: . , ! ? \' " ( ) - _ : ;');
}

export function validateDate(value) {
  if (dateAllowlist.test(value)) return;
  throw new Error('Date format invalid: only numbers, / and - allowed');
}

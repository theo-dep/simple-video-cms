// form.requestSubmit() only exists from Safari 16 / iOS 16 onward.
// Fallback: dispatch a submit event, which triggers the onSubmit
// handlers (the pages call e.preventDefault and invoke the API).
// form.submit() is not suitable: it triggers neither validation nor submit.
export function requestFormSubmit(form) {
  if (typeof form.requestSubmit === 'function') {
    form.requestSubmit();
    return;
  }
  form.dispatchEvent(new Event('submit', { bubbles: true, cancelable: true }));
}

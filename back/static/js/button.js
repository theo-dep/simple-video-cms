for (let i = 0; i < document.forms.length; i++) {
    document.forms[i].addEventListener('submit', () => {
        // on submit form, disable all buttons and inputs in the page to prevent multi-clicks
        for (let button of document.getElementsByClassName('button')) {
            button.disabled = true;
        }
        for (let input of document.getElementsByClassName('input')) {
            input.disabled = true;
        }

        for (let button of document.forms[i].getElementsByClassName('button')) {
            button.textContent = 'Processing...';
        }
    }, false);
}

for (let i = 0; i < document.forms.length; i++) {
    document.forms[i].addEventListener('submit', () => {
        // on submit form, disable all buttons and inputs in the page to prevent multi-clicks
        for (let button of document.getElementsByClassName('button')) {
            button.disabled = true;
        }
        for (let input of document.getElementsByClassName('input')) {
            input.disabled = true;
        }

        // replace the button form text by a spinner
        for (let button of document.forms[i].getElementsByClassName('button')) {
            button.innerHTML = `
            <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="spinner">
                <circle cx="12" cy="12" r="10" stroke-dasharray="31.4" stroke-dashoffset="0">
                    <animate attributeName="stroke-dashoffset" from="0" to="62.8" dur="1s" repeatCount="indefinite" />
                </circle>
            </svg>
        `;
        }
    }, false);
}

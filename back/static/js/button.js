// from https://github.com/n3r4zzurr0/svg-spinners/blob/main/svg-css/blocks-shuffle-3.svg
const spinner = `
    <svg fill="currentColor" class="spinner" width="24" height="24" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
        <style>
            .spinner_9y7u {
                animation: spinner_fUkk 4.8s linear infinite;
                animation-delay: -4.8s;
            }
            .spinner_DF2s {
                animation-delay: -3.2s;
            }
            .spinner_q27e {
                animation-delay: -1.6s;
            }
            @keyframes spinner_fUkk {
                8.33% { x: 13px; y: 1px; }
                25% { x: 13px; y: 1px; }
                33.3% { x: 13px; y: 13px; }
                50% { x: 13px; y: 13px; }
                58.33% { x: 1px; y: 13px; }
                75% { x: 1px; y: 13px; }
                83.33% { x: 1px; y: 1px; }
            }
        </style>
        <rect class="spinner_9y7u" x="1" y="1" rx="1" width="10" height="10"/>
        <rect class="spinner_9y7u spinner_DF2s" x="1" y="1" rx="1" width="10" height="10"/>
        <rect class="spinner_9y7u spinner_q27e" x="1" y="1" rx="1" width="10" height="10"/>
    </svg>
`;

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
            button.innerHTML = spinner;
            button.classList.toggle('button-spinner');
        }
    }, false);
}

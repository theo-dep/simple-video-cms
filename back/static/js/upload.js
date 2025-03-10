document.addEventListener('DOMContentLoaded', () => {
    const fileInput = document.querySelector('.file-input');
    const dropArea = document.querySelector('.file-drop-area');

    // highlight drag area
    fileInput.addEventListener('dragenter', () => {
        dropArea.classList.add('is-active');
    });
    fileInput.addEventListener('focus', () => {
        dropArea.classList.add('is-active');
    });
    fileInput.addEventListener('click', () => {
        dropArea.classList.add('is-active');
    });

    // back to normal state
    fileInput.addEventListener('dragleave', () => {
        dropArea.classList.remove('is-active');
    });
    fileInput.addEventListener('blur', () => {
        dropArea.classList.remove('is-active');
    });
    fileInput.addEventListener('drop', () => {
        dropArea.classList.remove('is-active');
    });

    // change inner text
    fileInput.addEventListener('change', function () {
        const filesCount = this.files.length;
        const textContainer = this.previousElementSibling;

        const fileName = this.value.split('\\').pop();
        textContainer.textContent = fileName;

        // If filename has an extension, remove it, otherwise keep the filename
        const basename = fileName ? fileName.slice(0, fileName.lastIndexOf('.')) || fileName : '';
        document.getElementById('title').value = basename;
    });
});
